#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import csv
import logging
import os
import sys

from telemetry.core import browser_finder
from telemetry.core import browser_options
from telemetry.page import block_page_benchmark_results
from telemetry.page import csv_page_benchmark_results
from telemetry.page import page_benchmark
from telemetry.page import page_runner
from telemetry.page import page_set
from telemetry.test import discover

def Main(benchmark_dir):
  """Turns a PageBenchmark into a command-line program.

  Args:
    benchmark_dir: Path to directory containing PageBenchmarks.
  """
  benchmarks = discover.DiscoverClasses(benchmark_dir,
                                        os.path.join(benchmark_dir, '..'),
                                        page_benchmark.PageBenchmark)

  # Naively find the benchmark. If we use the browser options parser, we run
  # the risk of failing to parse if we use a benchmark-specific parameter.
  benchmark_name = None
  for arg in sys.argv:
    if arg in benchmarks:
      benchmark_name = arg

  options = browser_options.BrowserOptions()
  parser = options.CreateParser('%prog [options] <benchmark> <page_set>')

  page_runner.PageRunner.AddCommandLineOptions(parser)
  parser.add_option('--output-format',
                    dest='output_format',
                    default='csv',
                    help='Output format. Can be "csv" or "block". '
                    'Defaults to "%default".')
  parser.add_option('-o', '--output',
                    dest='output_file',
                    help='Redirects output to a file. Defaults to stdout.')
  parser.add_option('--output-trace-tag',
                    dest='output_trace_tag',
                    help='Append a tag to the key of each result trace.')

  benchmark = None
  if benchmark_name is not None:
    benchmark = benchmarks[benchmark_name]()
    benchmark.AddCommandLineOptions(parser)

  _, args = parser.parse_args()

  if benchmark is None or len(args) != 2:
    parser.print_usage()
    import page_sets # pylint: disable=F0401
    print >> sys.stderr, 'Available benchmarks:\n%s\n' % ',\n'.join(
        sorted(benchmarks.keys()))
    print >> sys.stderr, 'Available page_sets:\n%s\n' % ',\n'.join(
        sorted([os.path.relpath(f)
                for f in page_sets.GetAllPageSetFilenames()]))
    sys.exit(1)

  ps = page_set.PageSet.FromFile(args[1])

  benchmark.CustomizeBrowserOptions(options)
  possible_browser = browser_finder.FindBrowser(options)
  if not possible_browser:
    print >> sys.stderr, """No browser found.\n
Use --browser=list to figure out which are available.\n"""
    sys.exit(1)

  if not options.output_file:
    output_file = sys.stdout
  elif options.output_file == '-':
    output_file = sys.stdout
  else:
    output_file = open(os.path.expanduser(options.output_file), 'w')

  if options.output_format == 'csv':
    results = csv_page_benchmark_results.CsvPageBenchmarkResults(
      csv.writer(output_file),
      benchmark.results_are_the_same_on_every_page)
  elif options.output_format in ('block', 'terminal-block'):
    results = block_page_benchmark_results.BlockPageBenchmarkResults(
      output_file)
  else:
    raise Exception('Invalid --output-format value: "%s". Valid values are '
                    '"csv" and "block".'
                    % options.output_format)

  with page_runner.PageRunner(ps) as runner:
    runner.Run(options, possible_browser, benchmark, results)

  output_trace_tag = ''
  if options.output_trace_tag:
    output_trace_tag = options.output_trace_tag
  elif options.browser_executable:
    # When using an exact executable, assume it is a reference build for the
    # purpose of outputting the perf results.
    # TODO(tonyg): Remove this branch once the perfbots use --output-trace-tag.
    output_trace_tag = '_ref'
  results.PrintSummary(output_trace_tag)

  if len(results.page_failures):
    logging.warning('Failed pages: %s', '\n'.join(
        [failure['page'].url for failure in results.page_failures]))

  if len(results.skipped_pages):
    logging.warning('Skipped pages: %s', '\n'.join(
        [skipped['page'].url for skipped in results.skipped_pages]))
  return min(255, len(results.page_failures))
