# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from layout_package import json_results_generator
from layout_package import path_utils
from layout_package import test_expectations
from layout_package import test_failures


class JSONLayoutResultsGenerator(json_results_generator.JSONResultsGenerator):
    """A JSON results generator for layout tests."""

    LAYOUT_TESTS_PATH = "LayoutTests"

    # Additional JSON fields.
    WONTFIX = "wontfixCounts"
    DEFERRED = "deferredCounts"

    def __init__(self, builder_name, build_name, build_number,
        results_file_base_path, builder_base_url,
        test_timings, expectations, result_summary, all_tests):
        """Modifies the results.json file. Grabs it off the archive directory
        if it is not found locally.

        Args:
          result_summary: ResultsSummary object storing the summary of the test
              results.
          (see the comment of JSONResultsGenerator.__init__ for other Args)
        """

        self._builder_name = builder_name
        self._build_name = build_name
        self._build_number = build_number
        self._builder_base_url = builder_base_url
        self._results_file_path = os.path.join(results_file_base_path,
            self.RESULTS_FILENAME)
        self._expectations = expectations

        # We don't use self._skipped_tests and self._passed_tests as we
        # override _InsertFailureSummaries.

        # We want relative paths to LayoutTest root for JSON output.
        path_to_name = self._GetPathRelativeToLayoutTestRoot
        self._result_summary = result_summary
        self._failures = dict(
            (path_to_name(test), test_failures.DetermineResultType(failures))
            for (test, failures) in result_summary.failures.iteritems())
        self._all_tests = [path_to_name(test) for test in all_tests]
        self._test_timings = dict(
            (path_to_name(test_tuple.filename), test_tuple.test_run_time)
            for test_tuple in test_timings)

        self._GenerateJSONOutput()

    def _GetPathRelativeToLayoutTestRoot(self, test):
        """Returns the path of the test relative to the layout test root.
        For example, for:
          src/third_party/WebKit/LayoutTests/fast/forms/foo.html
        We would return
          fast/forms/foo.html
        """
        index = test.find(self.LAYOUT_TESTS_PATH)
        if index is not -1:
            index += len(self.LAYOUT_TESTS_PATH)

        if index is -1:
            # Already a relative path.
            relativePath = test
        else:
            relativePath = test[index + 1:]

        # Make sure all paths are unix-style.
        return relativePath.replace('\\', '/')

    # override
    def _ConvertJSONToCurrentVersion(self, results_json):
        archive_version = None
        if self.VERSION_KEY in results_json:
            archive_version = results_json[self.VERSION_KEY]

        super(JSONLayoutResultsGenerator, self)._ConvertJSONToCurrentVersion(
            results_json)

        # version 2->3
        if archive_version == 2:
            for results_for_builder in results_json.itervalues():
                try:
                    test_results = results_for_builder[self.TESTS]
                except:
                    continue

            for test in test_results:
                # Make sure all paths are relative
                test_path = self._GetPathRelativeToLayoutTestRoot(test)
                if test_path != test:
                    test_results[test_path] = test_results[test]
                    del test_results[test]

    # override
    def _InsertFailureSummaries(self, results_for_builder):
        summary = self._result_summary

        self._InsertItemIntoRawList(results_for_builder,
            len((set(summary.failures.keys()) |
                summary.tests_by_expectation[test_expectations.SKIP]) &
                summary.tests_by_timeline[test_expectations.NOW]),
            self.FIXABLE_COUNT)
        self._InsertItemIntoRawList(results_for_builder,
            self._GetFailureSummaryEntry(test_expectations.NOW),
            self.FIXABLE)
        self._InsertItemIntoRawList(results_for_builder,
            len(self._expectations.GetTestsWithTimeline(
                test_expectations.NOW)), self.ALL_FIXABLE_COUNT)
        self._InsertItemIntoRawList(results_for_builder,
            self._GetFailureSummaryEntry(test_expectations.DEFER),
            self.DEFERRED)
        self._InsertItemIntoRawList(results_for_builder,
            self._GetFailureSummaryEntry(test_expectations.WONTFIX),
            self.WONTFIX)

    # override
    def _NormalizeResultsJSON(self, test, test_name, tests):
        super(JSONLayoutResultsGenerator, self)._NormalizeResultsJSON(
            test, test_name, tests)

        # Remove tests that don't exist anymore.
        full_path = os.path.join(path_utils.LayoutTestsDir(), test_name)
        full_path = os.path.normpath(full_path)
        if not os.path.exists(full_path):
            del tests[test_name]

    def _GetFailureSummaryEntry(self, timeline):
        """Creates a summary object to insert into the JSON.

        Args:
          summary   ResultSummary object with test results
          timeline  current test_expectations timeline to build entry for
                    (e.g., test_expectations.NOW, etc.)
        """
        entry = {}
        summary = self._result_summary
        timeline_tests = summary.tests_by_timeline[timeline]
        entry[self.SKIP_RESULT] = len(
            summary.tests_by_expectation[test_expectations.SKIP] &
            timeline_tests)
        entry[self.PASS_RESULT] = len(
            summary.tests_by_expectation[test_expectations.PASS] &
            timeline_tests)
        for failure_type in summary.tests_by_expectation.keys():
            if failure_type not in self.FAILURE_TO_CHAR:
                continue
            count = len(summary.tests_by_expectation[failure_type] &
                        timeline_tests)
            entry[self.FAILURE_TO_CHAR[failure_type]] = count
        return entry
