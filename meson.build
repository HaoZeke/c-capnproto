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

# MSVC shared builds only emit capnp_c.lib when something is dllexport.
# CAPNP_C_BUILDING marks library objects; CAPNP_C_STATIC keeps the header
# from dllimport when the archive is static.
capnp_c_consumer_args = []
if get_option('default_library') == 'static'
  capnp_c_consumer_args += '-DCAPNP_C_STATIC'
endif

# Library name libcapnp_c matches historical pc and avoids clashing with C++ libcapnp.
libcapnp_c = library(
  'capnp_c',
  libcapnp_src,
  c_args: common_c_args + libcapnp_c_args + capnp_c_consumer_args,
  c_shared_args: ['-DCAPNP_C_BUILDING'],
  c_static_args: ['-DCAPNP_C_STATIC'],
  include_directories: include_directories(['lib']),
  implicit_include_directories: false,
  install: true,
)

# Generated user headers #include "c.capnp.h"; ship it next to capnp_c.h.
install_headers(
  'lib' / 'capnp_c.h',
  'compiler' / 'c.capnp.h',
)

libcapnp_dep = declare_dependency(
  link_with: libcapnp_c,
  include_directories: include_directories(['lib']),
  compile_args: capnp_c_consumer_args,
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
  gtest_main_dep = gtest_proj.get_variable('gtest_main_dep')

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
    'tests' / 'fortran-golden-test.cpp',
    'tests' / 'extraheader-test.cpp',
    'tests' / 'fieldgetset-test.cpp',
    'tests' / 'codegen-defaults-test.cpp',
    'tests' / 'bounds-test.cpp',
    'tests' / 'interface-test.cpp',
    'tests' / 'addressbook.capnp.c',
    'tests' / 'extraheader.capnp.c',
    'tests' / 'fieldgetset.capnp.c',
    'tests' / 'codegen-defaults.capnp.c',
    'compiler' / 'test.capnp.c',
    'compiler' / 'schema-test.cpp',
    'compiler' / 'schema.capnp.c',
  ]

  extraheader_h = meson.project_source_root() / 'tests' / 'extraheader.capnp.h'
  codegen_defaults_c = meson.project_source_root() / 'tests' / 'codegen-defaults.capnp.c'
  codegen_defaults_h = meson.project_source_root() / 'tests' / 'codegen-defaults.capnp.h'
  fortran_golden_dir = meson.project_source_root() / 'tests' / 'fixtures'

  exe = executable(
    'capn-test',
    capn_test_src,
    include_directories: include_directories(['lib', 'tests', 'compiler']),
    dependencies: test_dependencies,
    c_args: common_c_args,
    cpp_args: common_cpp_args + [
      '-DEXTRAHEADER_GENERATED_H="' + extraheader_h + '"',
      '-DCODEGEN_DEFAULTS_GENERATED_C="' + codegen_defaults_c + '"',
      '-DCODEGEN_DEFAULTS_GENERATED_H="' + codegen_defaults_h + '"',
      '-DFORTRAN_GOLDEN_DIR="' + fortran_golden_dir + '"',
    ],
    install: false,
    implicit_include_directories: false,
  )
  test('capn-test', exe)

  create_min_sz_test = executable(
    'create-min-sz-test',
    [
      'tests' / 'create-min-sz-test.cpp',
      'lib' / 'capn.c',
      'lib' / 'capn-malloc.c',
      'lib' / 'capn-stream.c',
    ],
    include_directories: include_directories(['lib']),
    dependencies: [gtest_dep, gtest_main_dep, thread_dep],
    c_args: common_c_args + ['-DCAPN_CREATE_MIN_SZ=512'],
    cpp_args: common_cpp_args,
    install: false,
    implicit_include_directories: false,
  )
  test('create-min-sz-test', create_min_sz_test)

  capnp = find_program('capnp', required: false)
  if capnp.found()
    codec_c_args = common_c_args + ['-D_POSIX_C_SOURCE=200809L']

    codec_roundtrip_gen = custom_target(
      'codec-roundtrip-capnp',
      input: 'tests' / 'codec-roundtrip.capnp',
      output: ['codec-roundtrip.capnp.c', 'codec-roundtrip.capnp.h'],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c],
    )

    codec_roundtrip_test = executable(
      'codec-roundtrip-test',
      'tests' / 'codec-roundtrip-test.cpp',
      codec_roundtrip_gen,
      include_directories: include_directories(['lib', 'tests', 'compiler']),
      dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
      c_args: codec_c_args + ['-include', meson.current_source_dir() / 'tests' / 'codec-roundtrip.h'],
      cpp_args: common_cpp_args,
      install: false,
    )
    test('codec-roundtrip-test', codec_roundtrip_test)

    group_in_union_gen = custom_target(
      'group-in-union-capnp',
      input: 'tests' / 'group-in-union.capnp',
      output: ['group-in-union.capnp.c', 'group-in-union.capnp.h'],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c],
    )

    group_in_union_test = executable(
      'group-in-union-test',
      'tests' / 'group-in-union-test.cpp',
      group_in_union_gen,
      include_directories: include_directories(['lib', 'tests', 'compiler']),
      dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
      c_args: codec_c_args + ['-include', meson.current_source_dir() / 'tests' / 'group-in-union.h'],
      cpp_args: common_cpp_args,
      install: false,
    )
    test('group-in-union-test', group_in_union_test)

    list_decode_srcdest_gen = custom_target(
      'list-decode-srcdest-capnp',
      input: 'tests' / 'list-decode-srcdest.capnp',
      output: ['list-decode-srcdest.capnp.c', 'list-decode-srcdest.capnp.h'],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c],
    )

    list_decode_srcdest_test = executable(
      'list-decode-srcdest-test',
      'tests' / 'list-decode-srcdest-test.cpp',
      list_decode_srcdest_gen,
      include_directories: include_directories(['lib', 'tests', 'compiler']),
      dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
      c_args: codec_c_args + ['-include', meson.current_source_dir() / 'tests' / 'list-decode-srcdest.h'],
      cpp_args: common_cpp_args,
      install: false,
    )
    test('list-decode-srcdest-test', list_decode_srcdest_test)

    interface_field_gen = custom_target(
      'interface-field-capnp',
      input: 'tests' / 'interface-field.capnp',
      output: ['interface-field.capnp.c', 'interface-field.capnp.h'],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c],
    )

    interface_field_test = executable(
      'interface-field-test',
      'tests' / 'interface-field-test.cpp',
      interface_field_gen,
      include_directories: include_directories(['lib', 'tests', 'compiler']),
      dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
      c_args: codec_c_args,
      cpp_args: common_cpp_args,
      install: false,
    )
    test('interface-field-test', interface_field_test)

    foreach t : ['data-codec', 'empty-list', 'null-text']
      gen = custom_target(
        t + '-capnp',
        input: 'tests' / (t + '.capnp'),
        output: [t + '.capnp.c', t + '.capnp.h'],
        command: [
          capnp, 'compile',
          '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
          '-I', meson.current_source_dir() / 'compiler',
          '--src-prefix', meson.current_source_dir() / 'tests',
          '@INPUT@',
        ],
        depends: [capnpc_c],
      )
      t_exe = executable(
        t + '-test',
        'tests' / (t + '-test.cpp'),
        gen,
        include_directories: include_directories(['lib', 'tests', 'compiler']),
        dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
        c_args: codec_c_args + ['-include', meson.current_source_dir() / 'tests' / (t + '.h')],
        cpp_args: common_cpp_args,
        install: false,
      )
      test(t + '-test', t_exe)
    endforeach
  endif
endif

if get_option('fuzz')
  fuzz_inc = include_directories(['lib', 'tests', 'compiler'])
  # read_mem.c uses lstat; strict c99 hides POSIX declarations.
  executable(
    'fuzz-mem',
    ['fuzz' / 'read_mem.c', 'tests' / 'addressbook.capnp.c'],
    include_directories: fuzz_inc,
    dependencies: [libcapnp_dep],
    c_args: common_c_args,
    override_options: ['c_std=gnu99'],
    install: false,
    implicit_include_directories: false,
  )
  executable(
    'fuzz-fp',
    ['fuzz' / 'read_fp.c', 'tests' / 'addressbook.capnp.c'],
    include_directories: fuzz_inc,
    dependencies: [libcapnp_dep],
    c_args: common_c_args,
    override_options: ['c_std=gnu99'],
    install: false,
    implicit_include_directories: false,
  )
endif
