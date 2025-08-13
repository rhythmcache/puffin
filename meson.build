project('puffin', 'cpp',
  version : '1.0.0',
  meson_version : '>=1.1',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
    'optimization=3',
    'debug=true'
  ]
)


# Dependencies
gflags_dep = dependency('gflags')
glog_dep = dependency('glog')
protobuf_lite_dep = dependency('protobuf-lite')
threads_dep = dependency('threads')

# Test dependencies (only if tests are enabled)
if get_option('tests')
  gtest_dep = dependency('gtest')
endif

# Filesystem module for checking file existence
fs = import('fs')

# Include directories
inc_dirs = include_directories('.', 'src/include')

# Source files for the main library (matching Makefile exactly)
puffin_sources = files(
  'src/bit_reader.cc',
  'src/bit_writer.cc',
  'src/extent_stream.cc',
  'src/file_stream.cc',
  'src/huffer.cc',
  'src/huffman_table.cc',
  'src/memory_stream.cc',
  'src/puffer.cc',
  'src/puff_reader.cc',
  'src/puff_writer.cc',
  'src/puffin_stream.cc',
  'src/utils.cc'
)

# Unit test sources (matching Makefile + adding unittest_common.cc for the missing symbols)
if get_option('tests')
  unittest_sources = files(
    'src/bit_io_unittest.cc',
    'src/puff_io_unittest.cc',
    'src/puffin_unittest.cc',
    'src/stream_unittest.cc',
    'src/testrunner.cc',
    'src/unittest_common.cc',
    'src/utils_unittest.cc'
  )
endif

# Shared library
libpuffin = shared_library('puffin',
  puffin_sources,
  include_directories : inc_dirs,
  dependencies : [gflags_dep, glog_dep, protobuf_lite_dep, threads_dep],
  install : true,
  version : meson.project_version(),
  soversion : '1'
)

# Static library (optional)
if get_option('static_lib')
  libpuffin_static = static_library('puffin',
    puffin_sources,
    include_directories : inc_dirs,
    dependencies : [gflags_dep, glog_dep, protobuf_lite_dep, threads_dep],
    install : true
  )
endif

# Unit tests executable (only if tests are enabled)
if get_option('tests')
  puffin_unittests = executable('puffin_unittests',
    unittest_sources,
    include_directories : inc_dirs,
    dependencies : [gflags_dep, glog_dep, protobuf_lite_dep, gtest_dep, threads_dep],
    link_with : libpuffin,
    install : false
  )

  # Test target
  test('puffin_tests', puffin_unittests)
endif

# Declare dependency for other projects
if get_option('static_lib')
  puffin_dep = declare_dependency(
    link_with : [libpuffin, libpuffin_static],
    include_directories : inc_dirs
  )
else
  puffin_dep = declare_dependency(
    link_with : libpuffin,
    include_directories : inc_dirs
  )
endif