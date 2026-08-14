# Smoke: build a tiny program against capnp_c via add_subdirectory, the
# same path FetchContent_MakeAvailable takes.
cmake_minimum_required(VERSION 3.16)
project(c_capnproto_consumer_smoke LANGUAGES C)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(C_CAPNPROTO_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${C_CAPNPROTO_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/c_capnproto-build")

add_executable(smoke smoke.c)
target_link_libraries(smoke PRIVATE c_capnproto::capnp_c)
