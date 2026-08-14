# Optional CMake build. Meson (recommended) and autotools remain the
# default product paths. This file is a small install-path alternative
# (MSVC / IDE / packagers). It does not use the DKML `dk` wrapper.
#
#   include(FetchContent)
#   FetchContent_Declare(
#     c_capnproto
#     GIT_REPOSITORY https://github.com/HaoZeke/c-capnproto.git
#     GIT_TAG        v0.3.0
#   )
#   FetchContent_MakeAvailable(c_capnproto)
#   target_link_libraries(myapp PRIVATE c_capnproto::capnp_c)
#
# An installed copy is found with find_package(c_capnproto) or through
# pkg-config as `c-capnproto`. The package name is c_capnproto so it never
# shadows official Cap'n Proto C++'s CapnProtoConfig.cmake.

cmake_minimum_required(VERSION 3.16)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" C_CAPNPROTO_VERSION)
string(STRIP "${C_CAPNPROTO_VERSION}" C_CAPNPROTO_VERSION)

project(c-capnproto
  VERSION ${C_CAPNPROTO_VERSION}
  DESCRIPTION "Cap'n Proto pure-C runtime and capnpc-c plugin"
  HOMEPAGE_URL "https://github.com/HaoZeke/c-capnproto"
  LANGUAGES C CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(_c_capnproto_is_toplevel ON)
else()
  set(_c_capnproto_is_toplevel OFF)
endif()

option(BUILD_TESTING "Build unit tests when an installed GTest is found" ${_c_capnproto_is_toplevel})
option(C_CAPNPROTO_INSTALL "Install targets and package config" ${_c_capnproto_is_toplevel})

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Same source set as meson.build. The RPC vat needs the readers for
# rpc.capnp and the three-party network layer, which are checked in
# under compiler/ rather than generated at build time.
add_library(capnp_c
  lib/capn.c
  lib/capn-malloc.c
  lib/capn-stream.c
  lib/capn-rpc.c
  compiler/rpc.capnp.c
  compiler/rpc-threeparty.capnp.c
)
set_target_properties(capnp_c PROPERTIES
  OUTPUT_NAME capnp_c
  POSITION_INDEPENDENT_CODE ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
)
add_library(c_capnproto::capnp_c ALIAS capnp_c)
set_target_properties(capnp_c PROPERTIES EXPORT_NAME capnp_c)
target_include_directories(capnp_c
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/compiler>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(capnp_c PRIVATE CAPNP_C_BUILDING)
else()
  target_compile_definitions(capnp_c PUBLIC CAPNP_C_STATIC)
endif()

set(capnpc_c_sources
  compiler/capnpc-c.c
  compiler/schema.capnp.c
  compiler/str.c
)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/compiler/codecgen.c")
  list(APPEND capnpc_c_sources compiler/codecgen.c)
endif()

add_executable(capnpc-c ${capnpc_c_sources})
target_link_libraries(capnpc-c PRIVATE capnp_c)
target_include_directories(capnpc-c PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/lib"
  "${CMAKE_CURRENT_SOURCE_DIR}/compiler"
)

if(C_CAPNPROTO_INSTALL)
install(TARGETS capnp_c capnpc-c
  EXPORT c_capnprotoTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES lib/capnp_c.h lib/capn-rpc.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# Generated user headers #include "c.capnp.h"; ship it beside capnp_c.h.
install(FILES compiler/c.capnp.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES compiler/c.capnp DESTINATION ${CMAKE_INSTALL_DATADIR}/c-capnproto)

install(EXPORT c_capnprotoTargets
  FILE c_capnprotoTargets.cmake
  NAMESPACE c_capnproto::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/c_capnproto
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_capnprotoConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/c_capnprotoConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/c_capnproto
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/c_capnprotoConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/c_capnprotoConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/c_capnprotoConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/c_capnproto
)

# pkg-config, for build systems that do not speak CMake.
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
  set(libdir "${CMAKE_INSTALL_LIBDIR}")
else()
  set(libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_BINDIR}")
  set(bindir "${CMAKE_INSTALL_BINDIR}")
else()
  set(bindir "\${prefix}/${CMAKE_INSTALL_BINDIR}")
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
  set(includedir "${CMAKE_INSTALL_INCLUDEDIR}")
else()
  set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()
set(PACKAGE_VERSION "${PROJECT_VERSION}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/c-capnproto.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/c-capnproto.pc"
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/c-capnproto.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
endif()

if(BUILD_TESTING)
  include(CTest)
  find_package(GTest QUIET)
  if(GTest_FOUND)
    if(TARGET GTest::gtest)
      set(capn_gtest_lib GTest::gtest)
    elseif(TARGET GTest::GTest)
      set(capn_gtest_lib GTest::GTest)
    else()
      set(capn_gtest_lib ${GTEST_LIBRARIES})
    endif()
    find_package(Threads REQUIRED)
    add_executable(capn-test
      tests/capn-test.cpp
      tests/capn-stream-test.cpp
      tests/example-test.cpp
      tests/fortran-golden-test.cpp
      tests/interface-test.cpp
      tests/list-c6-upgrade-test.cpp
      tests/addressbook.capnp.c
      compiler/test.capnp.c
      compiler/schema-test.cpp
      compiler/schema.capnp.c
    )
    target_include_directories(capn-test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/lib"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests"
      "${CMAKE_CURRENT_SOURCE_DIR}/compiler"
    )
    target_compile_definitions(capn-test PRIVATE
      "FORTRAN_GOLDEN_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures\""
    )
    target_link_libraries(capn-test PRIVATE
      capnp_c
      ${capn_gtest_lib}
      Threads::Threads
    )
    add_test(NAME capn-test COMMAND capn-test)
  else()
    message(STATUS "GTest not found; skipping capn-test (install GTest or use meson)")
  endif()
endif()
