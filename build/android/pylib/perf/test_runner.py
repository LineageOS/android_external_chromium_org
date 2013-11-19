# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs perf tests.

Our buildbot infrastructure requires each slave to run steps serially.
This is sub-optimal for android, where these steps can run independently on
multiple connected devices.

The buildbots will run this script multiple times per cycle:
- First: all steps listed in --steps in will be executed in parallel using all
connected devices. Step results will be pickled to disk. Each step has a unique
name. The result code will be ignored if the step name is listed in
--flaky-steps.
The buildbot will treat this step as a regular step, and will not process any
graph data.

- Then, with -print-step STEP_NAME: at this stage, we'll simply print the file
with the step results previously saved. The buildbot will then process the graph
data accordingly.


The JSON steps file contains a dictionary in the format:
[
  ["step_name_foo", "script_to_execute foo"],
  ["step_name_bar", "script_to_execute bar"]
]

This preserves the order in which the steps are executed.

The JSON flaky steps file contains a list with step names which results should
be ignored:
[
  "step_name_foo",
  "step_name_bar"
]

Note that script_to_execute necessarily have to take at least the following
option:
  --device: the serial number to be passed to all adb commands.
"""

import datetime
import logging
import os
import pickle
import sys
import threading
import time

from pylib import constants
from pylib import forwarder
from pylib import pexpect
from pylib.base import base_test_result
from pylib.base import base_test_runner


def PrintTestOutput(test_name):
  """Helper method to print the output of previously executed test_name.

  Args:
    test_name: name of the test that has been previously executed.

  Returns:
    exit code generated by the test step.
  """
  file_name = os.path.join(constants.PERF_OUTPUT_DIR, test_name)
  if not os.path.exists(file_name):
    logging.error('File not found %s', file_name)
    return 1

  with file(file_name, 'r') as f:
    persisted_result = pickle.loads(f.read())
  logging.info('*' * 80)
  logging.info('Output from:')
  logging.info(persisted_result['cmd'])
  logging.info('*' * 80)
  print persisted_result['output']

  return persisted_result['exit_code']


class _HeartBeatLogger(object):
  # How often to print the heartbeat on flush().
  _PRINT_INTERVAL = 30.0

  def __init__(self):
    """A file-like class for keeping the buildbot alive."""
    self._len = 0
    self._tick = time.time()
    self._stopped = threading.Event()
    self._timer = threading.Thread(target=self._runner)
    self._timer.start()

  def _runner(self):
    while not self._stopped.is_set():
      self.flush()
      self._stopped.wait(_HeartBeatLogger._PRINT_INTERVAL)

  def write(self, data):
    self._len += len(data)

  def flush(self):
    now = time.time()
    if now - self._tick >= _HeartBeatLogger._PRINT_INTERVAL:
      self._tick = now
      print '--single-step output length %d' % self._len
      sys.stdout.flush()

  def stop(self):
    self._stopped.set()


class TestRunner(base_test_runner.BaseTestRunner):
  def __init__(self, test_options, device, tests, flaky_tests):
    """A TestRunner instance runs a perf test on a single device.

    Args:
      test_options: A PerfOptions object.
      device: Device to run the tests.
      tests: a dict mapping test_name to command.
      flaky_tests: a list of flaky test_name.
    """
    super(TestRunner, self).__init__(device, None, 'Release')
    self._options = test_options
    self._tests = tests
    self._flaky_tests = flaky_tests

  @staticmethod
  def _IsBetter(result):
    if result['actual_exit_code'] == 0:
      return True
    pickled = os.path.join(constants.PERF_OUTPUT_DIR,
                           result['name'])
    if not os.path.exists(pickled):
      return True
    with file(pickled, 'r') as f:
      previous = pickle.loads(f.read())
    return result['actual_exit_code'] < previous['actual_exit_code']

  @staticmethod
  def _SaveResult(result):
    if TestRunner._IsBetter(result):
      with file(os.path.join(constants.PERF_OUTPUT_DIR,
                             result['name']), 'w') as f:
        f.write(pickle.dumps(result))

  def _LaunchPerfTest(self, test_name):
    """Runs a perf test.

    Args:
      test_name: the name of the test to be executed.

    Returns:
      A tuple containing (Output, base_test_result.ResultType)
    """
    try:
      logging.warning('Unmapping device ports')
      forwarder.Forwarder.UnmapAllDevicePorts(self.adb)
      self.adb.RestartAdbdOnDevice()
    except Exception as e:
      logging.error('Exception when tearing down device %s', e)

    cmd = ('%s --device %s' %
           (self._tests[test_name], self.device))
    logging.info('%s : %s', test_name, cmd)
    start_time = datetime.datetime.now()

    timeout = 5400
    if self._options.no_timeout:
      timeout = None
    full_cmd = cmd
    if self._options.dry_run:
      full_cmd = 'echo %s' % cmd

    logfile = sys.stdout
    if self._options.single_step:
      # Just print a heart-beat so that the outer buildbot scripts won't timeout
      # without response.
      logfile = _HeartBeatLogger()
    cwd = os.path.abspath(constants.DIR_SOURCE_ROOT)
    if full_cmd.startswith('src/'):
      cwd = os.path.abspath(os.path.join(constants.DIR_SOURCE_ROOT, os.pardir))
    output, exit_code = pexpect.run(
        full_cmd, cwd=cwd,
        withexitstatus=True, logfile=logfile, timeout=timeout,
        env=os.environ)
    if self._options.single_step:
      # Stop the logger.
      logfile.stop()
    end_time = datetime.datetime.now()
    if exit_code is None:
      exit_code = -1
    logging.info('%s : exit_code=%d in %d secs at %s',
                 test_name, exit_code, (end_time - start_time).seconds,
                 self.device)
    result_type = base_test_result.ResultType.FAIL
    if exit_code == 0:
      result_type = base_test_result.ResultType.PASS
    actual_exit_code = exit_code
    if test_name in self._flaky_tests:
      # The exit_code is used at the second stage when printing the
      # test output. If the test is flaky, force to "0" to get that step green
      # whilst still gathering data to the perf dashboards.
      # The result_type is used by the test_dispatcher to retry the test.
      exit_code = 0

    persisted_result = {
        'name': test_name,
        'output': output,
        'exit_code': exit_code,
        'actual_exit_code': actual_exit_code,
        'result_type': result_type,
        'total_time': (end_time - start_time).seconds,
        'device': self.device,
        'cmd': cmd,
    }
    self._SaveResult(persisted_result)

    return (output, result_type)

  def RunTest(self, test_name):
    """Run a perf test on the device.

    Args:
      test_name: String to use for logging the test result.

    Returns:
      A tuple of (TestRunResults, retry).
    """
    output, result_type = self._LaunchPerfTest(test_name)
    results = base_test_result.TestRunResults()
    results.AddResult(base_test_result.BaseTestResult(test_name, result_type))
    retry = None
    if not results.DidRunPass():
      retry = test_name
    return results, retry
