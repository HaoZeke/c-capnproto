# C only. The C++ compiler is for the gtest suites and the live capnp-C++
# interop peer, so it is added where those are built: a consumer of the C
# runtime should not need a C++ toolchain to compile it.
project(
  'c-capnproto',
  'c',
  version: '0.4.2',
  # c_shared_args / c_static_args below arrived in 1.3.0.
  meson_version: '>=1.3.0',
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
  'lib' / 'capn-rpc.c',
  'lib' / 'capn-dyn.c',
  # RPC needs the rpc.capnp / rpc-threeparty.capnp accessors; the wrappers
  # push default visibility so generated accessors stay exported under
  # hidden builds.
  'lib' / 'capn-rpc-schema.c',
  'lib' / 'capn-rpc-threeparty-schema.c',
]

# MSVC shared builds only emit capnp_c.lib when something is dllexport.
# CAPNP_C_BUILDING marks library objects; CAPNP_C_STATIC keeps the header
# from dllimport when the archive is static.
capnp_c_consumer_args = []
if get_option('default_library') == 'static'
  capnp_c_consumer_args += '-DCAPNP_C_STATIC'
endif

# Library name libcapnp_c matches historical pc and avoids clashing with C++ libcapnp.
# Sanitizer runtimes are resolved when the executable is linked.
# --no-undefined fails the shared library on those symbols under clang.
lib_override = []
if get_option('b_sanitize') != 'none'
  lib_override += 'b_lundef=false'
endif
libcapnp_c = library(
  'capnp_c',
  libcapnp_src,
  c_args: common_c_args + libcapnp_c_args + capnp_c_consumer_args,
  c_shared_args: ['-DCAPNP_C_BUILDING'],
  c_static_args: ['-DCAPNP_C_STATIC'],
  gnu_symbol_visibility: 'hidden',
  include_directories: include_directories(['lib', 'compiler']),
  implicit_include_directories: false,
  override_options: lib_override,
  soversion: '2',
  install: true,
  version: meson.project_version(),
)

# Generated user headers #include "c.capnp.h" and capn-rpc.h includes
# rpc.capnp.h; install the generated schema headers next to capnp_c.h.
install_headers(
  'lib' / 'capnp_c.h',
  'lib' / 'capn-rpc.h',
  'lib' / 'capn-dyn.h',
  'compiler' / 'c.capnp.h',
  'compiler' / 'rpc.capnp.h',
  'compiler' / 'rpc-threeparty.capnp.h',
)

libcapnp_dep = declare_dependency(
  link_with: libcapnp_c,
  # compiler/ carries c.capnp.h, which every generated header includes.
  include_directories: include_directories(['lib', 'compiler']),
  compile_args: capnp_c_consumer_args,
)

# Consumers reach this through `dependency('c-capnproto')`, which falls back
# to this subproject via a .wrap.
meson.override_dependency('c-capnproto', libcapnp_dep)

# c-capnproto.pc.in is the file autotools and CMake install too.
# Substitute @PACKAGE_VERSION@ and the path variables for this prefix.
pc_prefix = get_option('prefix')
pc_bindir = get_option('bindir')
pc_libdir = get_option('libdir')
pc_includedir = get_option('includedir')
pc_data = configuration_data()
pc_data.set('prefix', pc_prefix)
pc_data.set('exec_prefix', '${prefix}')
if pc_bindir.startswith('/')
  pc_data.set('bindir', pc_bindir)
else
  pc_data.set('bindir', '${exec_prefix}/' + pc_bindir)
endif
if pc_libdir.startswith('/')
  pc_data.set('libdir', pc_libdir)
else
  pc_data.set('libdir', '${exec_prefix}/' + pc_libdir)
endif
if pc_includedir.startswith('/')
  pc_data.set('includedir', pc_includedir)
else
  pc_data.set('includedir', '${prefix}/' + pc_includedir)
endif
pc_data.set('PACKAGE_VERSION', meson.project_version())
configure_file(
  input: 'c-capnproto.pc.in',
  output: 'c-capnproto.pc',
  configuration: pc_data,
  install: true,
  install_dir: get_option('libdir') / 'pkgconfig',
)

capnpc_src = [
  'compiler' / 'capnpc-c.c',
  'compiler' / 'codecgen.c',
  'compiler' / 'schema.capnp.c',
  'compiler' / 'str.c',
  'compiler' / 'reflectgen.c',
]

capnpc_c = executable(
  'capnpc-c',
  capnpc_src,
  include_directories: include_directories(['lib', 'compiler']),
  dependencies: [libcapnp_dep],
  c_args: common_c_args,
  install: true,
  # The plugin links the shared runtime, which installs outside the
  # default loader path on multiarch systems.
  install_rpath: get_option('prefix') / get_option('libdir'),
)

# Schema annotations used by generators / fieldgetset.
install_data(
  'compiler' / 'c.capnp',
  install_dir: get_option('datadir') / 'c-capnproto',
)

if not meson.is_subproject() and get_option('enable_tests').allowed()
  gtest_proj = subproject('gtest', required: get_option('enable_tests'))
  if not gtest_proj.found()
    message('gtest is absent; unit tests are skipped')
  endif
  if gtest_proj.found()
  # The suites are C++; ask for the compiler here rather than at the top.
  add_languages('cpp', native: false)
  thread_dep = dependency('threads')
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
    'tests' / 'testunion-union1-test.cpp',
    'tests' / 'list-c6-upgrade-test.cpp',
    'tests' / 'constants-test.cpp',
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
  test_capnp_c = meson.project_source_root() / 'compiler' / 'test.capnp.c'
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
      '-DTEST_CAPNP_GENERATED_C="' + test_capnp_c + '"',
      '-DFORTRAN_GOLDEN_DIR="' + fortran_golden_dir + '"',
    ],
    install: false,
    implicit_include_directories: false,
  )
  test(
    'capn-test',
    exe,
    env: {
      'CAPNP_SOURCE_ROOT': meson.project_source_root(),
      # A huge list is refused with NULL. ASan aborts that allocation
      # unless it is allowed to return NULL.
      'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1:print_summary=1:allocator_may_return_null=1',
    },
  )

  compiler_str_test = executable(
    'compiler-str-test',
    ['tests' / 'compiler-str-test.c', 'compiler' / 'str.c'],
    include_directories: include_directories(['lib', 'compiler']),
    dependencies: test_dependencies,
    c_args: common_c_args,
    install: false,
    implicit_include_directories: false,
  )
  test('compiler-str-test', compiler_str_test)
  export_sh = find_program('sh', required: false)
  if export_sh.found()
    export_tests = meson.project_source_root() / 'tests'
    test(
      'capn-public-exports',
      export_sh,
      args: [
        export_tests / 'check-dyn-export.sh',
        libcapnp_c.full_path(),
        meson.project_source_root() / 'lib',
      ],
      depends: [libcapnp_c],
    )
    test(
      'capn-public-exports-comment',
      export_sh,
      args: [
        export_tests / 'check-dyn-export-comment.sh',
        libcapnp_c.full_path(),
        export_tests / 'fixtures' / 'dyn-export-comment.h',
      ],
      depends: [libcapnp_c],
    )
  endif

  null_ptr_write_test = executable(
    'null-ptr-write-test',
    'tests' / 'null-ptr-write-test.c',
    include_directories: include_directories(['lib', 'compiler']),
    dependencies: test_dependencies,
    c_args: common_c_args,
    install: false,
    implicit_include_directories: false,
  )
  test('null-ptr-write-test', null_ptr_write_test)

  packed_header_write_test = executable(
    'packed-header-write-test',
    'tests' / 'packed-header-write-test.c',
    include_directories: include_directories(['lib', 'compiler']),
    dependencies: test_dependencies,
    c_args: common_c_args,
    install: false,
    implicit_include_directories: false,
  )
  test('packed-header-write-test', packed_header_write_test)

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

  # The vat tests read checked-in generated sources, so they must not sit
  # behind the `capnp` CLI: gating them on it would make the whole RPC
  # suite disappear on a runner without the compiler installed.
  rpc_schema_export_test = executable(
    'rpc-schema-export-test',
    'tests' / 'rpc-schema-export-test.c',
    dependencies: [libcapnp_dep],
    c_args: common_c_args,
    implicit_include_directories: false,
    install: false,
  )
  test('rpc-schema-export-test', rpc_schema_export_test)

  rpc_join_test = executable(
    'rpc-join-test',
    'tests' / 'rpc-join-test.cpp',
    include_directories: include_directories(['lib', 'tests', 'compiler']),
    dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
    cpp_args: common_cpp_args + [
      '-DCAPNP_SOURCE_ROOT="' + meson.project_source_root() + '"',
    ],
    implicit_include_directories: false,
    install: false,
  )
  test(
    'rpc-join-test',
    rpc_join_test,
    env: {'CAPNP_SOURCE_ROOT': meson.project_source_root()},
  )

  rpc_handoff_test = executable(
    'rpc-handoff-test',
    'tests' / 'rpc-handoff-test.cpp',
    include_directories: include_directories(['lib', 'tests', 'compiler']),
    dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
    cpp_args: common_cpp_args,
    implicit_include_directories: false,
    install: false,
  )
  test('rpc-handoff-test', rpc_handoff_test)

  rpc_client_test = executable(
    'rpc-client-test',
    'tests' / 'rpc-client-test.cpp',
    include_directories: include_directories(['lib', 'tests', 'compiler']),
    dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
    cpp_args: common_cpp_args,
    implicit_include_directories: false,
    install: false,
  )
  test('rpc-client-test', rpc_client_test)

  gate_r5_test = executable(
    'gate-r5-test',
    'tests' / 'gate-r5-test.cpp',
    include_directories: include_directories(['lib', 'tests', 'compiler']),
    dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
    cpp_args: common_cpp_args,
    implicit_include_directories: false,
    install: false,
  )
  test('gate-r5-test', gate_r5_test)

  # A cross build must not run the host capnp against the target
  # capnpc-c plugin. The CLI parity tests stay on native builds.
  if meson.is_cross_build()
    have_host_capnp = false
  else
    capnp = find_program('capnp', native: true, required: false)
    have_host_capnp = capnp.found()
  endif
  wire_cli = []
  if have_host_capnp
    wire_cli = [
      '-DCAPNP_CLI="' + capnp.full_path() + '"',
      '-DCAPNP_SOURCE_ROOT="' + meson.project_source_root() + '"',
    ]
  endif
  wire_parity_test = executable(
    'wire-parity-test',
    'tests' / 'wire-parity-test.cpp',
    include_directories: include_directories(['lib', 'tests', 'compiler']),
    dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
    cpp_args: common_cpp_args + wire_cli,
    implicit_include_directories: false,
    install: false,
  )
  test(
    'wire-parity-test',
    wire_parity_test,
    env: have_host_capnp ? {'CAPNP_CLI': capnp.full_path()} : {},
  )

  posix_socket_test = executable(
    'posix-socket-test',
    'tests' / 'posix-socket-test.c',
    include_directories: include_directories(['interop']),
    c_args: ['-D_POSIX_C_SOURCE=200809L'],
    implicit_include_directories: false,
    install: false,
  )
  test('posix-socket-test', posix_socket_test)
  test(
    'posix-socket-check',
    find_program('sh'),
    args: [
      meson.project_source_root() / 'tests' / 'posix-socket-check.sh',
      cc.cmd_array()[-1],
      meson.project_source_root(),
    ],
  )

  frame_le_test = executable(
    'frame-le-test',
    'tests' / 'frame-le-test.c',
    include_directories: include_directories(['interop']),
    c_args: [
      '-DRPC_CLIENT_SOURCE="' + meson.project_source_root() / 'interop' / 'rpc_client.c' + '"',
      '-DTAIL_CLIENT_SOURCE="' + meson.project_source_root() / 'interop' / 'tail_client.c' + '"',
    ],
    implicit_include_directories: false,
    install: false,
  )
  test('frame-le-test', frame_le_test)

  spell_finish_test = executable(
    'spell-finish-test',
    'tests' / 'spell-finish-test.c',
    c_args: ['-DSOURCE_FILE="' + meson.project_source_root() / 'lib' / 'capn-rpc.c' + '"'],
    implicit_include_directories: false,
    install: false,
  )
  test('spell-finish-test', spell_finish_test)

  # The struct layout grew, so the shared object name moved with it.
  test(
    'soname',
    find_program('sh'),
    args: [
      '-c',
      'readelf -d "' + libcapnp_c.full_path() + '" | grep -q "libcapnp_c.so.2"',
    ],
  )

  if have_host_capnp
    test(
      'regen-generated',
      find_program('bash'),
      args: [
        meson.project_source_root() / 'scripts' / 'regen-generated.sh',
        '--check',
        capnpc_c.full_path(),
      ],
      workdir: meson.project_source_root(),
    )

    codec_c_args = common_c_args + ['-D_POSIX_C_SOURCE=200809L']

    read_defaults_gen = custom_target(
      'read-defaults-capnp',
      input: 'tests' / 'read-defaults.capnp',
      output: ['read-defaults.capnp.c', 'read-defaults.capnp.h'],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c],
    )
    read_defaults_test = executable(
      'read-defaults-test',
      ['tests' / 'read-defaults-test.cpp', read_defaults_gen],
      include_directories: include_directories(['lib', 'tests', 'compiler']),
      dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
      cpp_args: common_cpp_args,
      install: false,
    )
    test('read-defaults-test', read_defaults_test)

    test(
      'stream-schema',
      find_program('sh'),
      args: [meson.project_source_root() / 'scripts' / 'check-stream-schema.sh'],
    )
    test(
      'rpc-listen-wait',
      find_program('bash'),
      args: [meson.project_source_root() / 'scripts' / 'check-rpc-listen-wait.sh'],
      timeout: 30,
    )
    test(
      'const-name',
      find_program('sh'),
      args: [meson.project_source_root() / 'scripts' / 'check-const-name.sh'],
    )

    # Official `capnp` CLI interop. Skip these tests only when the
    # binary is missing (this branch is not taken). Do not GTEST_SKIP
    # when meson found capnp.
    # Schemas compiled with $C.reflect, for the reflection, text and JSON
    # tests that compare capn-dyn output with the official tool.
    reflect_gen = custom_target(
      'reflect-capnp',
      input: ['tests' / 'reflect.capnp', 'tests' / 'reflect-other.capnp',
              'tests' / 'jsonann.capnp'],
      output: [
        'reflect.capnp.c', 'reflect.capnp.h',
        'reflect-other.capnp.c', 'reflect-other.capnp.h',
        'jsonann.capnp.c', 'jsonann.capnp.h',
      ],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c],
    )

    cli_interop_test = executable(
      'capnp-cli-interop-test',
      [
        'tests' / 'capnp-cli-interop-test.cpp',
        'tests' / 'reflect-test.cpp',
        'tests' / 'json-ann-test.cpp',
        'tests' / 'brand-test.cpp',
        'tests' / 'addressbook.capnp.c',
        'compiler' / 'test.capnp.c',
        reflect_gen,
      ],
      include_directories: include_directories(['.', 'lib', 'tests', 'compiler']),
      dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
      cpp_args: common_cpp_args + [
        '-DCAPNP_CLI="' + capnp.full_path() + '"',
        '-DCAPNP_SOURCE_ROOT="' + meson.project_source_root() + '"',
      ],
      install: false,
      implicit_include_directories: false,
    )
    test(
      'capnp-cli-interop-test',
      cli_interop_test,
      env: {
        'CAPNP_CLI': capnp.full_path(),
        'CAPNP_SOURCE_ROOT': meson.project_source_root(),
      },
    )


    # --- live-peer RPC interop: this vat against capnp-C++ -----------
    #
    # Byte equality against our own encoders says nothing about whether
    # the reference implementation will talk to us, so this bootstraps a
    # real EzRpcServer and calls it.
    cpp_ok = add_languages('cpp', required: false, native: false)
    capnp_rpc_dep = dependency('capnp-rpc', required: false)
    capnpc_cpp = find_program('capnpc-c++', required: false)
    bash = find_program('bash', required: false)

    if cpp_ok and capnp_rpc_dep.found() and capnpc_cpp.found() and bash.found()
      adder_cpp = custom_target(
        'adder-capnp-cpp',
        input: 'compiler' / 'adder.capnp',
        output: ['adder.capnp.c++', 'adder.capnp.h'],
        command: [
          capnp, 'compile',
          '--src-prefix=' + meson.current_source_dir() / 'compiler',
          '-I', meson.current_source_dir() / 'compiler',
          '-o' + capnpc_cpp.full_path() + ':@OUTDIR@',
          '@INPUT@',
        ],
      )

      # The generated adder.capnp.h lands in the build directory, so this
      # target keeps the implicit include dirs the others drop.
      rpc_peer_server = executable(
        'rpc_peer_server',
        ['interop' / 'rpc_peer_server.c++', adder_cpp],
        dependencies: capnp_rpc_dep,
        override_options: ['cpp_std=c++17'],
      )

      # The C bindings are checked in beside the other generated schema
      # code, as elsewhere in this repo; regenerate with capnpc-c after
      # changing compiler/adder.capnp.
      rpc_client_c = executable(
        'rpc_client_c',
        ['interop' / 'rpc_client.c', 'compiler' / 'adder.capnp.c'],
        include_directories: include_directories(['lib', 'compiler']),
        dependencies: [libcapnp_dep],
        # usleep is POSIX, and -std=c99 hides it without this.
        c_args: ['-D_POSIX_C_SOURCE=200809L'],
        implicit_include_directories: false,
      )
      frame_wrap_test = executable(
        'frame-wrap-test',
        'interop' / 'frame_wrap_test.c',
        c_args: ['-D_POSIX_C_SOURCE=200809L'],
        implicit_include_directories: false,
      )
      test('frame-wrap', frame_wrap_test, args: [rpc_client_c.full_path()],
           depends: [rpc_client_c], timeout: 10)

      rpc_server_c = executable(
        'rpc_server_c',
        ['interop' / 'rpc_server.c', 'compiler' / 'adder.capnp.c'],
        include_directories: include_directories(['lib', 'compiler']),
        dependencies: [libcapnp_dep],
        c_args: ['-D_POSIX_C_SOURCE=200809L'],
        implicit_include_directories: false,
      )
      rpc_peer_client = executable(
        'rpc_peer_client',
        ['interop' / 'rpc_peer_client.c++', adder_cpp],
        dependencies: capnp_rpc_dep,
        override_options: ['cpp_std=c++17'],
      )

      test(
        'rpc_interop_cpp',
        bash,
        args: [
          meson.current_source_dir() / 'interop' / 'run_rpc_interop.sh',
          rpc_peer_server.full_path(),
          rpc_client_c.full_path(),
          rpc_server_c.full_path(),
          rpc_peer_client.full_path(),
        ],
        depends: [rpc_peer_server, rpc_client_c, rpc_server_c, rpc_peer_client],
        timeout: 60,
      )

      rpc_promise_server = executable(
        'rpc_promise_server',
        ['interop' / 'rpc_promise_server.c', 'compiler' / 'adder.capnp.c'],
        include_directories: include_directories(['lib', 'compiler']),
        dependencies: [libcapnp_dep],
        c_args: ['-D_POSIX_C_SOURCE=200809L'],
        implicit_include_directories: false,
      )

      test(
        'rpc_interop_cpp_promise',
        bash,
        args: [
          meson.current_source_dir() / 'interop' / 'run_rpc_promise.sh',
          rpc_promise_server.full_path(),
          rpc_peer_server.full_path(),
        ],
        depends: [rpc_promise_server, rpc_peer_server],
        timeout: 60,
      )

      tail_cpp = custom_target(
        'tail-capnp-cpp',
        input: 'interop' / 'tail.capnp',
        output: ['tail.capnp.c++', 'tail.capnp.h'],
        command: [
          capnp, 'compile',
          '--src-prefix=' + meson.current_source_dir() / 'interop',
          '-o' + capnpc_cpp.full_path() + ':@OUTDIR@',
          '@INPUT@',
        ],
      )
      tail_server = executable(
        'tail_server',
        ['interop' / 'tail_server.c++', tail_cpp],
        dependencies: capnp_rpc_dep,
        override_options: ['cpp_std=c++17'],
      )
      tail_client = executable(
        'tail_client',
        'interop' / 'tail_client.c',
        include_directories: include_directories(['lib', 'compiler']),
        dependencies: [libcapnp_dep],
        c_args: ['-D_POSIX_C_SOURCE=200809L'],
        implicit_include_directories: false,
      )
      test(
        'rpc_tail_cpp',
        bash,
        args: [
          meson.current_source_dir() / 'interop' / 'run_rpc_interop.sh',
          tail_server.full_path(),
          tail_client.full_path(),
        ],
        depends: [tail_server, tail_client],
        timeout: 60,
      )
    else
      message('interop/rpc: skipped (needs a C++ toolchain, capnp-rpc, and the capnp tools).')
    endif

    # An imported struct whose nested constant is unused still has that
    # constant in the schema. The request omits it. The compiler must
    # accept the file.
    nested_holder_gen = custom_target(
      'nested-const-capnp',
      input: 'tests' / 'nested-const.capnp',
      output: ['nested-const.capnp.c', 'nested-const.capnp.h'],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'tests',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c],
    )
    # Compile the importer by itself. The imported nested constant is
    # not in the request. Passing both files hides that.
    nested_const_gen = custom_target(
      'uses-nested-capnp',
      input: 'tests' / 'uses-nested.capnp',
      output: ['uses-nested.capnp.c', 'uses-nested.capnp.h'],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'tests',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c, nested_holder_gen],
    )
    nested_const_test = executable(
      'nested-const-test',
      'tests' / 'nested-const-test.c',
      nested_const_gen,
      nested_holder_gen,
      include_directories: include_directories(['lib', 'compiler']),
      dependencies: [libcapnp_dep],
    )
    test('nested-const', nested_const_test)

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

    rpcstubs_gen = custom_target(
      'rpcstubs-capnp',
      input: 'tests' / 'rpcstubs.capnp',
      output: ['rpcstubs.capnp.c', 'rpcstubs.capnp.h'],
      command: [
        capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@',
      ],
      depends: [capnpc_c],
    )

    rpcstubs_test = executable(
      'rpcstubs-test',
      'tests' / 'rpcstubs-test.cpp',
      rpcstubs_gen,
      include_directories: include_directories(['lib', 'tests', 'compiler']),
      dependencies: [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep],
      c_args: codec_c_args,
      cpp_args: common_cpp_args,
      install: false,
    )
    test('rpcstubs-test', rpcstubs_test)

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
endif

if get_option('fuzz')
  fuzz_inc = include_directories(['lib', 'tests', 'compiler'])
  fuzz_reader = executable(
    'fuzz-reader',
    'fuzz' / 'read_message.c',
    include_directories: fuzz_inc,
    dependencies: [libcapnp_dep],
    c_args: common_c_args,
    install: false,
    implicit_include_directories: false,
  )
  # read_mem.c uses lstat; strict c99 hides POSIX declarations.
  fuzz_mem = executable(
    'fuzz-mem',
    ['fuzz' / 'read_mem.c', 'tests' / 'addressbook.capnp.c'],
    include_directories: fuzz_inc,
    dependencies: [libcapnp_dep],
    c_args: common_c_args,
    override_options: ['c_std=gnu99'],
    install: false,
    implicit_include_directories: false,
  )
  fuzz_fp = executable(
    'fuzz-fp',
    ['fuzz' / 'read_fp.c', 'tests' / 'addressbook.capnp.c'],
    include_directories: fuzz_inc,
    dependencies: [libcapnp_dep],
    c_args: common_c_args,
    override_options: ['c_std=gnu99'],
    install: false,
    implicit_include_directories: false,
  )
  fuzz_rpc = executable(
    'fuzz-rpc-frame',
    'fuzz' / 'rpc_frame.c',
    include_directories: fuzz_inc,
    dependencies: [libcapnp_dep],
    c_args: common_c_args,
    install: false,
    implicit_include_directories: false,
  )
  fuzz_dyn = executable(
    'fuzz-dyn-text',
    'fuzz' / 'dyn_text.c',
    include_directories: fuzz_inc,
    dependencies: [libcapnp_dep],
    c_args: common_c_args,
    install: false,
    implicit_include_directories: false,
  )

  # One-shot smoke on the seed corpus. Prefer a freshly encoded Person.bin
  # when capnp is on PATH (same as the autotools fuzz/in/Person.bin rule);
  # otherwise use the checked-in seed. Not unbounded AFL.
  fs = import('fs')
  fuzz_seed = meson.project_source_root() / 'fuzz' / 'in' / 'Person.bin'
  if meson.is_cross_build()
    have_fuzz_capnp = false
  else
    capnp_fuzz = find_program('capnp', native: true, required: false)
    have_fuzz_capnp = capnp_fuzz.found()
  endif
  sh = find_program('sh', required: false)
  if have_fuzz_capnp and sh.found()
    fuzz_person_bin = custom_target(
      'fuzz-person-bin',
      input: files('fuzz' / 'Person.txt'),
      output: 'Person.bin',
      command: [
        sh,
        '-c',
        '"$1" encode -I "$2" "$3" Person < "$4" > "$5"',
        '--',
        capnp_fuzz.full_path(),
        meson.project_source_root() / 'compiler',
        meson.project_source_root() / 'tests' / 'addressbook.capnp',
        '@INPUT@',
        '@OUTPUT@',
      ],
    )
    test('fuzz-mem-smoke', fuzz_mem, args: [fuzz_person_bin])
    test('fuzz-fp-smoke', fuzz_fp, args: [fuzz_person_bin])
    test('fuzz-reader-smoke', fuzz_reader, args: [fuzz_person_bin])
    test('fuzz-rpc-frame-smoke', fuzz_rpc,
         args: [meson.project_source_root() / 'tests' / 'fixtures' / 'rpc-accept.bin'])
  elif fs.exists(fuzz_seed)
    test('fuzz-mem-smoke', fuzz_mem, args: [fuzz_seed])
    test('fuzz-fp-smoke', fuzz_fp, args: [fuzz_seed])
    test('fuzz-reader-smoke', fuzz_reader, args: [fuzz_seed])
    test('fuzz-rpc-frame-smoke', fuzz_rpc,
         args: [meson.project_source_root() / 'tests' / 'fixtures' / 'rpc-accept.bin'])
  endif
  if sh.found()
    smoke = meson.project_source_root() / 'scripts' / 'fuzz-smoke.sh'
    person = meson.project_source_root() / 'fuzz' / 'in' / 'Person.bin'
    cap_frame = meson.project_source_root() / 'fuzz' / 'in' / 'cap-index-frame.bin'
    dyn16 = meson.project_source_root() / 'fuzz' / 'in' / 'dyn-cap-16.bin'
    rpc_fix = meson.project_source_root() / 'tests' / 'fixtures' / 'rpc-accept.bin'
    test('fuzz-reader-60s', sh,
         args: [smoke, '60', fuzz_reader, '--mutate', '3016077', '60', person, cap_frame, dyn16],
         timeout: 120)
    test('fuzz-mem-60s', sh,
         args: [smoke, '60', fuzz_mem, '--mutate', '3016077', '60', person],
         timeout: 120)
    test('fuzz-fp-60s', sh,
         args: [smoke, '60', fuzz_fp, '--mutate', '3016077', '60', person],
         timeout: 120)
    test('fuzz-rpc-60s', sh,
         args: [smoke, '60', fuzz_rpc, '--mutate', '3016077', '60', cap_frame, rpc_fix],
         timeout: 120)
    test('fuzz-dyn-60s', sh,
         args: [smoke, '60', fuzz_dyn, '--mutate', '3016077', '60', dyn16, cap_frame],
         timeout: 120)
  endif
endif
