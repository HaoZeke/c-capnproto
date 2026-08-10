# Optional CMake build. Meson (recommended) and autotools remain the
# default product paths. This file is a small install-path alternative
# (MSVC / IDE / packagers). It does not use the DKML `dk` wrapper.

cmake_minimum_required(VERSION 3.16)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" C_CAPNPROTO_VERSION)
string(STRIP "${C_CAPNPROTO_VERSION}" C_CAPNPROTO_VERSION)

project(c-capnproto
  VERSION ${C_CAPNPROTO_VERSION}
  DESCRIPTION "Cap'n Proto pure-C runtime and capnpc-c plugin"
  HOMEPAGE_URL "https://github.com/HaoZeke/c-capnproto"
  LANGUAGES C CXX
)

include(GNUInstallDirs)

option(BUILD_TESTING "Build unit tests when an installed GTest is found" ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(capnp_c
  lib/capn.c
  lib/capn-malloc.c
  lib/capn-stream.c
)
set_target_properties(capnp_c PROPERTIES
  OUTPUT_NAME capnp_c
  POSITION_INDEPENDENT_CODE ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
)
target_include_directories(capnp_c
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
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

install(TARGETS capnp_c capnpc-c
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES lib/capnp_c.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES compiler/c.capnp DESTINATION ${CMAKE_INSTALL_DATADIR}/c-capnproto)

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
