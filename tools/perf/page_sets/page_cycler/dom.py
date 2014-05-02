# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable=W0401,W0614
from telemetry.page.actions.all_page_actions import *
from telemetry.page import page as page_module
from telemetry.page import page_set as page_set_module


class DomPage(page_module.Page):

  def __init__(self, url, page_set):
    super(DomPage, self).__init__(url=url, page_set=page_set)


class DomPageSet(page_set_module.PageSet):

  """ DOM page_cycler benchmark """

  def __init__(self):
    super(DomPageSet, self).__init__(
      # pylint: disable=C0301
      serving_dirs=set(['../../../../data/page_cycler/dom']))

    urls_list = [
      'file://../../../../data/page_cycler/dom/HTMLDocument_write/',
      'file://../../../../data/page_cycler/dom/Document_getElementById/',
      'file://../../../../data/page_cycler/dom/DOMWindow_document/',
      'file://../../../../data/page_cycler/dom/DOMWindow_window/',
      'file://../../../../data/page_cycler/dom/Element_getAttribute/',
      'file://../../../../data/page_cycler/dom/HTMLCollection_length/',
      'file://../../../../data/page_cycler/dom/HTMLElement_className/',
      'file://../../../../data/page_cycler/dom/HTMLElement_id/',
      'file://../../../../data/page_cycler/dom/NodeList_length/'
    ]

    for url in urls_list:
      self.AddPage(DomPage(url, self))
