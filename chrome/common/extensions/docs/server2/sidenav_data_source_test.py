#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from compiled_file_system import CompiledFileSystem
from object_store_creator import ObjectStoreCreator
from servlet import Request
from sidenav_data_source import (
    SidenavDataSource, _AddLevels, _AddSelected, _QualifyHrefs)
from test_file_system import TestFileSystem
from test_util import CaptureLogging


class FakeServerInstance(object):
  def __init__(self, file_system):
    self.compiled_host_fs_factory = CompiledFileSystem.Factory(
        file_system, ObjectStoreCreator.ForTest())
    self.sidenav_json_base_path = ''


class SamplesDataSourceTest(unittest.TestCase):
  def testAddLevels(self):
    sidenav_json = [{
      'title': 'H2',
      'items': [{
        'title': 'H3',
        'items': [{ 'title': 'X1' }]
      }]
    }]

    expected = [{
      'level': 1,
      'title': 'H2',
      'items': [{
        'level': 2,
        'title': 'H3',
        'items': [{ 'level': 3, 'title': 'X1' }]
      }]
    }]

    _AddLevels(sidenav_json, 1)
    self.assertEqual(expected, sidenav_json)

  def testAddSelected(self):
    sidenav_json = [
      { 'href': '/AH2.html' },
      {
        'href': '/H2.html',
        'items': [{
          'href': '/H3.html'
        }]
      }
    ]

    expected = [
      { 'href': '/AH2.html' },
      {
        'child_selected': True,
        'href': '/H2.html',
        'items': [{
          'href': '/H3.html',
          'selected': True
        }]
      }
    ]

    _AddSelected(sidenav_json, 'H3.html')
    self.assertEqual(expected, sidenav_json)

  def testQualifyHrefs(self):
    sidenav_json = [
      { 'href': '/qualified/H1.html' },
      { 'href': 'https://qualified/X1.html' },
      {
        'href': 'H2.html',
        'items': [{
          'href': 'H3.html'
        }]
      }
    ]

    expected = [
      { 'href': '/qualified/H1.html' },
      { 'href': 'https://qualified/X1.html' },
      {
        'href': '/H2.html',
        'items': [{
          'href': '/H3.html'
        }]
      }
    ]

    log_output = CaptureLogging(lambda: _QualifyHrefs(sidenav_json))

    self.assertEqual(expected, sidenav_json)
    self.assertEqual(2, len(log_output))

  def testSidenavDataSource(self):
    file_system = TestFileSystem({
      'apps_sidenav.json': json.dumps([{
        'title': 'H1',
        'href': 'H1.html',
        'items': [{
          'title': 'H2',
          'href': '/H2.html'
        }]
      }])
    })

    expected = [{
      'level': 2,
      'child_selected': True,
      'title': 'H1',
      'href': '/H1.html',
      'items': [{
        'level': 3,
        'selected': True,
        'title': 'H2',
        'href': '/H2.html'
      }]
    }]

    sidenav_data_source = SidenavDataSource(
        FakeServerInstance(file_system), Request.ForTest('/H2.html'))

    log_output = CaptureLogging(
        lambda: self.assertEqual(expected, sidenav_data_source.get('apps')))

    self.assertEqual(1, len(log_output))
    self.assertTrue(
        log_output[0].msg.startswith('Paths in sidenav must be qualified.'))

  def testCron(self):
    file_system = TestFileSystem({
      'apps_sidenav.json': '[{ "title": "H1" }]' ,
      'extensions_sidenav.json': '[{ "title": "H2" }]'
    })

    # Ensure Cron doesn't rely on request.
    sidenav_data_source = SidenavDataSource(
        FakeServerInstance(file_system), request=None)
    sidenav_data_source.Cron()

    # If Cron fails, apps_sidenav.json will not be cached, and the _cache_data
    # access will fail.
    # TODO(jshumway): Make a non hack version of this check.
    sidenav_data_source._cache._file_object_store.Get(
        '/apps_sidenav.json').Get()._cache_data


if __name__ == '__main__':
  unittest.main()
