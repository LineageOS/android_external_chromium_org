{
  'TOOLS': ['newlib', 'glibc', 'pnacl', 'linux'],
  'SEL_LDR': True,

  'TARGETS': [
    {
      'NAME' : 'nacl_io_test',
      'TYPE' : 'main',
      'SOURCES' : [
        'dev_fs_for_testing.h',
        'event_test.cc',
        'fake_core_interface.cc',
        'fake_core_interface.h',
        'fake_pepper_interface_html5_fs.cc',
        'fake_pepper_interface_html5_fs.h',
        'fake_pepper_interface_url_loader.cc',
        'fake_pepper_interface_url_loader.h',
        'fake_resource_manager.cc',
        'fake_resource_manager.h',
        'fake_var_interface.cc',
        'fake_var_interface.h',
        'fifo_test.cc',
        'filesystem_test.cc',
        'fuse_fs_test.cc',
        'html5_fs_test.cc',
        'http_fs_test.cc',
        'kernel_object_test.cc',
        'kernel_proxy_test.cc',
        'kernel_wrap_test.cc',
        'main.cc',
        'mem_fs_node_test.cc',
        'mock_fs.cc',
        'mock_fs.h',
        'mock_kernel_proxy.cc',
        'mock_kernel_proxy.h',
        'mock_node.cc',
        'mock_node.h',
        'mock_util.h',
        'path_test.cc',
        'pepper_interface_mock.cc',
        'pepper_interface_mock.h',
        'socket_test.cc',
        'tty_test.cc',
      ],
      'DEPS': ['ppapi_simple', 'nacl_io'],
      # Order matters here: gtest has a "main" function that will be used if
      # referenced before ppapi.
      'LIBS': ['gmock', 'ppapi_cpp', 'ppapi', 'gtest', 'pthread'],
      'INCLUDES': ['$(NACL_SDK_ROOT)/include/gtest/internal'],
      'CXXFLAGS': ['-Wno-sign-compare'],
    }
  ],
  'DATA': [
    'example.js'
  ],
  'DEST': 'tests',
  'NAME': 'nacl_io_test',
  'TITLE': 'NaCl IO test',
}
