# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Extracts a Windows VS2013 toolchain from various downloadable pieces.


from toolchain import *


def GetIsoUrl(pro):
  """Get the .iso path."""
  prefix = 'http://download.microsoft.com/download/'
  if pro:
    return (prefix +
        'A/F/1/AF128362-A6A8-4DB3-A39A-C348086472CC/VS2013_RTM_PRO_ENU.iso')
  else:
    return (prefix +
        '7/2/E/72E0F986-D247-4289-B9DC-C4FB07374894/VS2013_RTM_DskExp_ENU.iso')


def DownloadMainIso(url):
  temp_dir = TempDir()
  target_path = os.path.join(temp_dir, os.path.basename(url))
  Download(url, target_path)
  return target_path


def GetSourceImage(local_dir, pro):
  url = GetIsoUrl(pro)
  if local_dir:
    return os.path.join(local_dir, os.path.basename(url))
  else:
    return DownloadMainIso(url)


def ExtractMsiList(iso_dir, packages):
  results = []
  for (package, skippable) in packages:
    path_to_package = os.path.join(iso_dir, 'packages', package)
    if not os.path.exists(path_to_package) and skippable:
      sys.stdout.write('Pro-only %s skipped.\n' % package)
      continue
    results.append(ExtractMsi(path_to_package))
  return results


def ExtractComponents(image):
  extracted_iso = ExtractIso(image)
  results = ExtractMsiList(extracted_iso, [
      (r'vcRuntimeAdditional_amd64\vc_runtimeAdditional_x64.msi', False),
      (r'vcRuntimeAdditional_x86\vc_runtimeAdditional_x86.msi', False),
      (r'vcRuntimeDebug_amd64\vc_runtimeDebug_x64.msi', False),
      (r'vcRuntimeDebug_x86\vc_runtimeDebug_x86.msi', False),
      (r'vcRuntimeMinimum_amd64\vc_runtimeMinimum_x64.msi', False),
      (r'vcRuntimeMinimum_x86\vc_runtimeMinimum_x86.msi', False),
      (r'vc_compilerCore86\vc_compilerCore86.msi', False),
      (r'vc_compilerCore86res\vc_compilerCore86res.msi', False),
      (r'vc_compilerx64nat\vc_compilerx64nat.msi', True),
      (r'vc_compilerx64natres\vc_compilerx64natres.msi', True),
      (r'vc_compilerx64x86\vc_compilerx64x86.msi', True),
      (r'vc_compilerx64x86res\vc_compilerx64x86res.msi', True),
      (r'vc_librarycore86\vc_librarycore86.msi', False),
      (r'vc_libraryDesktop\x64\vc_LibraryDesktopX64.msi', False),
      (r'vc_libraryDesktop\x86\vc_LibraryDesktopX86.msi', False),
      (r'vc_libraryextended\vc_libraryextended.msi', True),
      (r'Windows_SDK\Windows Software Development Kit-x86_en-us.msi', False),
      ('Windows_SDK\\'
       r'Windows Software Development Kit for Metro style Apps-x86_en-us.msi',
          False),
    ])
  return results


def CopyToFinalLocation(extracted_dirs, target_dir):
  sys.stdout.write('Copying to final location...\n')
  mappings = {
      'Program Files\\Microsoft Visual Studio 12.0\\': '.\\',
      'Windows Kits\\8.0\\': 'win8sdk\\',
      'System64\\': 'sys64\\',
      'System\\': 'sys32\\',
  }
  matches = []
  for extracted_dir in extracted_dirs:
    for root, dirnames, filenames in os.walk(extracted_dir):
      for filename in filenames:
        matches.append((extracted_dir, os.path.join(root, filename)))

  copies = []
  for prefix, full_path in matches:
    partial_path = full_path[len(prefix) + 1:]  # +1 for trailing \
    #print 'partial_path', partial_path
    for map_from, map_to in mappings.iteritems():
      #print 'map_from:', map_from, ', map_to:', map_to
      if partial_path.startswith(map_from):
        target_path = os.path.join(map_to, partial_path[len(map_from):])
        copies.append((full_path, os.path.join(target_dir, target_path)))

  for full_source, full_target in copies:
    target_dir = os.path.dirname(full_target)
    if not os.path.isdir(target_dir):
      os.makedirs(target_dir)
    shutil.copy2(full_source, full_target)


def GenerateSetEnvCmd(target_dir, pro):
  """Generate a batch file that gyp expects to exist to set up the compiler
  environment. This is normally generated by a full install of the SDK, but we
  do it here manually since we do not do a full install."""
  with open(os.path.join(
        target_dir, r'win8sdk\bin\SetEnv.cmd'), 'w') as file:
    file.write('@echo off\n')
    file.write(':: Generated by tools\\win\\toolchain\\toolchain2013.py.\n')
    # Common to x86 and x64
    file.write('set PATH=%~dp0..\\..\\Common7\\IDE;%PATH%\n')
    file.write('set INCLUDE=%~dp0..\\..\\win8sdk\\Include\\um;'
               '%~dp0..\\..\\win8sdk\\Include\\shared;'
               '%~dp0..\\..\\VC\\include;'
               '%~dp0..\\..\\VC\\atlmfc\\include\n')
    file.write('if "%1"=="/x64" goto x64\n')

    # x86. If we're Pro, then use the amd64_x86 cross (we don't support x86
    # host at all).
    if pro:
      file.write('set PATH=%~dp0..\\..\\win8sdk\\bin\\x86;'
                '%~dp0..\\..\\VC\\bin\\amd64_x86;'
                '%~dp0..\\..\\VC\\bin\\amd64;'  # Needed for mspdb120.dll.
                '%PATH%\n')
    else:
      file.write('set PATH=%~dp0..\\..\\win8sdk\\bin\\x86;'
                '%~dp0..\\..\\VC\\bin;%PATH%\n')
    file.write('set LIB=%~dp0..\\..\\VC\\lib;'
               '%~dp0..\\..\\win8sdk\\Lib\\win8\\um\\x86;'
               '%~dp0..\\..\\VC\\atlmfc\\lib\n')
    file.write('goto done\n')

    # Express does not include a native 64 bit compiler, so we have to use
    # the x86->x64 cross.
    if not pro:
      # x86->x64 cross.
      file.write(':x64\n')
      file.write('set PATH=%~dp0..\\..\\win8sdk\\bin\\x64;'
                 '%~dp0..\\..\\VC\\bin\\x86_amd64;'
                 '%PATH%\n')
    else:
      # x64 native.
      file.write(':x64\n')
      file.write('set PATH=%~dp0..\\..\\win8sdk\\bin\\x64;'
                 '%~dp0..\\..\\VC\\bin\\amd64;'
                 '%PATH%\n')
    file.write('set LIB=%~dp0..\\..\\VC\\lib\\amd64;'
               '%~dp0..\\..\\win8sdk\\Lib\\win8\\um\\x64;'
               '%~dp0..\\..\\VC\\atlmfc\\lib\\amd64\n')
    file.write(':done\n')


def GenerateTopLevelEnv(target_dir, pro):
  """Generate a batch file that sets up various environment variables that let
  the Chromium build files and gyp find SDKs and tools."""
  with open(os.path.join(target_dir, r'env.bat'), 'w') as file:
    file.write('@echo off\n')
    file.write(':: Generated by tools\\win\\toolchain\\toolchain2013.py.\n')
    file.write('set GYP_DEFINES=windows_sdk_path="%~dp0win8sdk" '
               'component=shared_library\n')
    file.write('set GYP_MSVS_VERSION=2013%s\n' % '' if pro else 'e')
    file.write('set GYP_MSVS_OVERRIDE_PATH=%~dp0\n')
    file.write('set GYP_GENERATORS=ninja\n')
    file.write('set WindowsSDKDir=%~dp0win8sdk\n')
    paths = [
        r'Debug_NonRedist\x64\Microsoft.VC120.DebugCRT',
        r'Debug_NonRedist\x86\Microsoft.VC120.DebugCRT',
        r'x64\Microsoft.VC120.CRT',
        r'x86\Microsoft.VC120.CRT',
      ]
    additions = ';'.join(('%~dp0' + x) for x in paths)
    file.write('set PATH=%s;%%PATH%%\n' % additions)
    file.write('echo Environment set for toolchain in %~dp0.\n')
    file.write('cd /d %~dp0..\n')


def main():
  parser = OptionParser()
  parser.add_option('--targetdir', metavar='DIR',
                    help='put toolchain into DIR',
                    default=os.path.abspath('win_toolchain_2013'))
  parser.add_option('--noclean', action='store_false', dest='clean',
                    help='do not remove temp files',
                    default=True)
  parser.add_option('--local', metavar='DIR',
                    help='use downloaded files from DIR')
  parser.add_option('--express', metavar='EXPRESS',
                    help='use VS Express instead of Pro', action='store_true')
  options, args = parser.parse_args()
  try:
    target_dir = os.path.abspath(options.targetdir)
    if os.path.exists(target_dir):
      sys.stderr.write('%s already exists. Please [re]move it or use '
                       '--targetdir to select a different target.\n' %
                       target_dir)
      return 1
    pro = not options.express
    # Set the working directory to 7z subdirectory. 7-zip doesn't find its
    # codec dll very well, so this is the simplest way to make sure it runs
    # correctly, as we don't otherwise care about working directory.
    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '7z'))
    image = GetSourceImage(options.local, pro)
    extracted = ExtractComponents(image)
    CopyToFinalLocation(extracted, target_dir)

    GenerateSetEnvCmd(target_dir, pro)
    GenerateTopLevelEnv(target_dir, pro)
  finally:
    if options.clean:
      DeleteAllTempDirs()

  sys.stdout.write(
      '\nIn a (clean) cmd shell, you can now run\n\n'
      '  %s\\env.bat\n\n'
      'then\n\n'
      "  gclient runhooks (or gclient sync if you haven't pulled deps yet)\n"
      '  ninja -C out\Debug chrome\n\n'
      'Note that this script intentionally does not modify any global\n'
      'settings like the registry, or system environment variables, so you\n'
      'will need to run the above env.bat whenever you start a new\n'
      'shell.\n\n' % target_dir)


if __name__ == '__main__':
  main()
