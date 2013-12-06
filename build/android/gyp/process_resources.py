#!/usr/bin/env python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Process Android library resources to generate R.java and crunched images."""

import optparse
import os
import shlex
import shutil

from util import build_utils

def ParseArgs():
  """Parses command line options.

  Returns:
    An options object as from optparse.OptionsParser.parse_args()
  """
  parser = optparse.OptionParser()
  parser.add_option('--android-sdk', help='path to the Android SDK folder')
  parser.add_option('--android-sdk-tools',
                    help='path to the Android SDK build tools folder')
  parser.add_option('--R-dir', help='directory to hold generated R.java')
  parser.add_option('--res-dirs',
                    help='directories containing resources to be packaged')
  parser.add_option('--crunch-input-dir',
                    help='directory containing images to be crunched')
  parser.add_option('--crunch-output-dir',
                    help='directory to hold crunched resources')
  parser.add_option('--non-constant-id', action='store_true')
  parser.add_option('--custom-package', help='Java package for R.java')
  parser.add_option('--android-manifest', help='AndroidManifest.xml path')
  parser.add_option('--stamp', help='File to touch on success')

  # This is part of a temporary fix for crbug.com/177552.
  # TODO(newt): remove this once crbug.com/177552 is fixed in ninja.
  parser.add_option('--ignore', help='this argument is ignored')
  (options, args) = parser.parse_args()

  if args:
    parser.error('No positional arguments should be given.')

  # Check that required options have been provided.
  required_options = ('android_sdk', 'android_sdk_tools', 'R_dir',
                      'res_dirs', 'crunch_input_dir', 'crunch_output_dir')
  build_utils.CheckOptions(options, parser, required=required_options)

  return options


def MoveImagesToNonMdpiFolders(res_root):
  """Move images from drawable-*-mdpi-* folders to drawable-* folders.

  Why? http://crbug.com/289843
  """
  for src_dir_name in os.listdir(res_root):
    src_components = src_dir_name.split('-')
    if src_components[0] != 'drawable' or 'mdpi' not in src_components:
      continue
    src_dir = os.path.join(res_root, src_dir_name)
    if not os.path.isdir(src_dir):
      continue
    dst_components = [c for c in src_components if c != 'mdpi']
    assert dst_components != src_components
    dst_dir_name = '-'.join(dst_components)
    dst_dir = os.path.join(res_root, dst_dir_name)
    build_utils.MakeDirectory(dst_dir)
    for src_file_name in os.listdir(src_dir):
      if not src_file_name.endswith('.png'):
        continue
      src_file = os.path.join(src_dir, src_file_name)
      dst_file = os.path.join(dst_dir, src_file_name)
      assert not os.path.lexists(dst_file)
      shutil.move(src_file, dst_file)


def main():
  options = ParseArgs()
  android_jar = os.path.join(options.android_sdk, 'android.jar')
  aapt = os.path.join(options.android_sdk_tools, 'aapt')

  build_utils.MakeDirectory(options.R_dir)

  # Generate R.java. This R.java contains non-final constants and is used only
  # while compiling the library jar (e.g. chromium_content.jar). When building
  # an apk, a new R.java file with the correct resource -> ID mappings will be
  # generated by merging the resources from all libraries and the main apk
  # project.
  package_command = [aapt,
                     'package',
                     '-m',
                     '-M', options.android_manifest,
                     '--auto-add-overlay',
                     '-I', android_jar,
                     '--output-text-symbols', options.R_dir,
                     '-J', options.R_dir]
  res_dirs = shlex.split(options.res_dirs)
  for res_dir in res_dirs:
    package_command += ['-S', res_dir]
  if options.non_constant_id:
    package_command.append('--non-constant-id')
  if options.custom_package:
    package_command += ['--custom-package', options.custom_package]
  build_utils.CheckOutput(package_command)

  # Crunch image resources. This shrinks png files and is necessary for 9-patch
  # images to display correctly.
  build_utils.MakeDirectory(options.crunch_output_dir)
  aapt_cmd = [aapt,
              'crunch',
              '-S', options.crunch_input_dir,
              '-C', options.crunch_output_dir]
  build_utils.CheckOutput(aapt_cmd, fail_if_stderr=True)

  MoveImagesToNonMdpiFolders(options.crunch_output_dir)

  if options.stamp:
    build_utils.Touch(options.stamp)


if __name__ == '__main__':
  main()
