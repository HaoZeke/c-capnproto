project(
  'c-capnproto',
  ['c', 'cpp'],
  version: '0.1.0',
  meson_version: '>=1.0.0',
  default_options: ['c_std=c99', 'cpp_std=c++14', 'warning_level=1'],
)

cc = meson.get_compiler('c')

common_c_args = []
common_cpp_args = []
libcapnp_c_args = []

libcapnp_src = [
  'lib' / 'capn-malloc.c',
  'lib' / 'capn-stream.c',
  'lib' / 'capn.c',
]

# Library name libcapnp_c matches historical pc and avoids clashing with C++ libcapnp.
libcapnp_c = library(
  'capnp_c',
  libcapnp_src,
  c_args: common_c_args + libcapnp_c_args,
  include_directories: include_directories(['lib']),
  implicit_include_directories: false,
  install: true,
)

install_headers('lib' / 'capnp_c.h')

libcapnp_dep = declare_dependency(
  link_with: libcapnp_c,
  include_directories: include_directories(['lib']),
)

pkg = import('pkgconfig')
pkg.generate(
  libcapnp_c,
  name: 'c-capnproto',
  description: 'Cap\'n Proto pure-C runtime (HaoZeke fork)',
  filebase: 'c-capnproto',
  url: 'https://github.com/HaoZeke/c-capnproto',
)

capnpc_src = [
  'compiler' / 'capnpc-c.c',
  'compiler' / 'codecgen.c',
  'compiler' / 'schema.capnp.c',
  'compiler' / 'str.c',
]

capnpc_c = executable(
  'capnpc-c',
  capnpc_src,
  include_directories: include_directories(['lib', 'compiler']),
  dependencies: [libcapnp_dep],
  c_args: common_c_args,
  install: true,
)

# Schema annotations used by generators / fieldgetset.
install_data(
  'compiler' / 'c.capnp',
  install_dir: get_option('datadir') / 'c-capnproto',
)

if get_option('enable_tests') and not meson.is_subproject()
  thread_dep = dependency('threads')
  gtest_proj = subproject('gtest')
  gtest_dep = gtest_proj.get_variable('gtest_dep')
  gmock_dep = gtest_proj.get_variable('gmock_dep')

  test_dependencies = [libcapnp_dep, gtest_dep, gmock_dep, thread_dep]

  if get_option('b_sanitize').contains('address')
    test_dependencies += cc.find_library('asan', required: false)
  endif
  if get_option('b_sanitize').contains('undefined')
    test_dependencies += cc.find_library('ubsan', required: false)
  endif

  capn_test_src = [
    'tests' / 'capn-test.cpp',
    'tests' / 'capn-stream-test.cpp',
    'tests' / 'example-test.cpp',
    'tests' / 'extraheader-test.cpp',
    'tests' / 'addressbook.capnp.c',
    'tests' / 'extraheader.capnp.c',
    'compiler' / 'test.capnp.c',
    'compiler' / 'schema-test.cpp',
    'compiler' / 'schema.capnp.c',
  ]

  extraheader_h = meson.project_source_root() / 'tests' / 'extraheader.capnp.h'

  exe = executable(
    'capn-test',
    capn_test_src,
    include_directories: include_directories(['lib', 'tests', 'compiler']),
    dependencies: test_dependencies,
    c_args: common_c_args,
    cpp_args: common_cpp_args + [
      '-DEXTRAHEADER_GENERATED_H="' + extraheader_h + '"',
    ],
    install: false,
    implicit_include_directories: false,
  )
  test('capn-test', exe)
endif
