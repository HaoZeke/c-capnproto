#!/usr/bin/env bash
# Consume c-capnproto the four ways a downstream project can, and fail if
# any of them stops working. Shipping install rules that nothing exercises
# is how a broken Config.cmake or a stale .pc reaches users.
#
#   1. CMake add_subdirectory  (the FetchContent_MakeAvailable path)
#   2. CMake find_package      (against a real staged install)
#   3. pkg-config              (CMake install, then a meson install)
#   4. meson subproject        (the .wrap path)
#   5. meson install          (the plugin must find its runtime, and the
#                              pkg-config file must name the plugin)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Version, libdir, and includedir must be set. @VAR@ markers must be gone.
check_pc() {
  local pc_file=$1
  local label=$2
  local want got libdir_raw inc_raw
  echo "----- ${pc_file} -----"
  cat "$pc_file"
  if grep -n '@[[:alnum:]_]*@' "$pc_file"; then
    echo "${label}: c-capnproto.pc has unsubstituted @VAR@ markers" >&2
    exit 1
  fi
  want=$(tr -d '[:space:]' < "$ROOT/VERSION.txt")
  got=$(sed -n 's/^Version:[[:space:]]*//p' "$pc_file" | head -1 | tr -d '[:space:]')
  if [[ -z "$got" || "$got" != "$want" ]]; then
    echo "${label}: Version [${got}] differs from VERSION.txt [${want}]" >&2
    exit 1
  fi
  desc=$(sed -n 's/^Description:[[:space:]]*//p' "$pc_file" | head -1)
  if [[ "$desc" != "Cap'n Proto C bindings" ]]; then
    echo "${label}: Description [${desc}] is unexpected" >&2
    exit 1
  fi
  if grep -q '^URL:' "$pc_file"; then
    echo "${label}: URL field is present" >&2
    exit 1
  fi
  libdir_raw=$(sed -n 's/^libdir=//p' "$pc_file" | head -1 | tr -d '[:space:]')
  inc_raw=$(sed -n 's/^includedir=//p' "$pc_file" | head -1 | tr -d '[:space:]')
  bindir_raw=$(sed -n 's/^bindir=//p' "$pc_file" | head -1 | tr -d '[:space:]')
  if [[ -z "$libdir_raw" || -z "$inc_raw" || -z "$bindir_raw" ]]; then
    echo "${label}: c-capnproto.pc is missing libdir, includedir, or bindir" >&2
    exit 1
  fi
  case "$libdir_raw" in
    *@*) echo "${label}: libdir still has @ markers" >&2; exit 1 ;;
  esac
  case "$inc_raw" in
    *@*) echo "${label}: includedir still has @ markers" >&2; exit 1 ;;
  esac
  echo "   ${label} Version=${got} libdir=${libdir_raw} includedir=${inc_raw} bindir=${bindir_raw}"
}

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
PREFIX="$WORK/prefix"

echo "== 1. CMake add_subdirectory (FetchContent path)"
cmake -S "$ROOT/cmake/consumer_smoke" -B "$WORK/sub" \
  -DC_CAPNPROTO_SOURCE_DIR="$ROOT" -DCMAKE_BUILD_TYPE=Release >"$WORK/sub.log" 2>&1
cmake --build "$WORK/sub" >>"$WORK/sub.log" 2>&1
"$WORK/sub/smoke"

echo "== 2. CMake find_package against a staged install"
# Packaging consumers do not require the producer's optional GTest setup.
cmake -S "$ROOT" -B "$WORK/build" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" -DBUILD_TESTING=OFF \
  >"$WORK/build.log" 2>&1
cmake --build "$WORK/build" >>"$WORK/build.log" 2>&1
cmake --install "$WORK/build" >>"$WORK/build.log" 2>&1

mkdir -p "$WORK/fp"
cp "$ROOT/cmake/consumer_smoke/smoke.c" "$WORK/fp/"
cat >"$WORK/fp/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.16)
project(c_capnproto_find_package_smoke LANGUAGES C)
find_package(c_capnproto REQUIRED)
add_executable(smoke smoke.c)
target_link_libraries(smoke PRIVATE c_capnproto::capnp_c)
EOF
cmake -S "$WORK/fp" -B "$WORK/fpb" -DCMAKE_PREFIX_PATH="$PREFIX" \
  -DCMAKE_BUILD_TYPE=Release >"$WORK/fp.log" 2>&1
cmake --build "$WORK/fpb" >>"$WORK/fp.log" 2>&1
"$WORK/fpb/smoke"
test -x "$PREFIX/bin/capnpc-c"

echo "== 3. pkg-config"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$PREFIX/lib64/pkgconfig"
pkg-config --exists c-capnproto
echo "   version $(pkg-config --modversion c-capnproto)"
pc_file=$(find "$PREFIX" -name c-capnproto.pc -print -quit)
test -n "$pc_file"
check_pc "$pc_file" cmake
codegen=$(pkg-config --variable=codegen c-capnproto)
echo "   codegen ${codegen}"
test -x "$codegen"
# shellcheck disable=SC2046
cc -o "$WORK/pc-smoke" "$ROOT/cmake/consumer_smoke/smoke.c" \
  $(pkg-config --cflags --libs c-capnproto)
"$WORK/pc-smoke"

echo "== 3b. meson pkg-config"
MESON_PREFIX="$WORK/meson-prefix"
meson setup "$WORK/meson-b" "$ROOT" --prefix="$MESON_PREFIX" -Denable_tests=disabled \
  >"$WORK/meson-b.log" 2>&1 || { tail -40 "$WORK/meson-b.log"; exit 1; }
meson compile -C "$WORK/meson-b" >>"$WORK/meson-b.log" 2>&1 || { tail -40 "$WORK/meson-b.log"; exit 1; }
meson install -C "$WORK/meson-b" >>"$WORK/meson-b.log" 2>&1 || { tail -40 "$WORK/meson-b.log"; exit 1; }
mpc=$(find "$MESON_PREFIX" -name c-capnproto.pc -print -quit)
if [[ -z "$mpc" ]]; then
  echo "meson install produced no c-capnproto.pc" >&2
  exit 1
fi
check_pc "$mpc" meson
pc_dir=$(dirname "$mpc")
export PKG_CONFIG_PATH="$pc_dir"
pkg-config --exists c-capnproto
# codegen is the user-visible meson fix. Check it before path shape.
mcode=$(pkg-config --variable=codegen c-capnproto)
mbin=$(pkg-config --variable=bindir c-capnproto)
echo "   meson codegen ${mcode} bindir ${mbin}"
test -n "$mcode"
test -x "$mcode"
test -n "$mbin"
test -d "$mbin"
mlib=$(pkg-config --variable=libdir c-capnproto)
minc=$(pkg-config --variable=includedir c-capnproto)
test -n "$mlib"
test -n "$minc"
test -d "$mlib"
test -d "$minc"
echo "   meson libdir ${mlib} includedir ${minc}"

echo "== 4. meson subproject (.wrap path)"
mkdir -p "$WORK/mw/subprojects"
# A local wrap; the published one points at the git tag instead.
ln -s "$ROOT" "$WORK/mw/subprojects/c-capnproto"
cp "$ROOT/cmake/consumer_smoke/smoke.c" "$WORK/mw/"
cat >"$WORK/mw/meson.build" <<'EOF'
project('c-capnproto-wrap-smoke', 'c', default_options: ['c_std=c11'])
dep = dependency('c-capnproto', fallback: ['c-capnproto', 'libcapnp_dep'])
executable('smoke', 'smoke.c', dependencies: dep)
EOF
meson setup "$WORK/mwb" "$WORK/mw" >"$WORK/mw.log" 2>&1 || { tail -25 "$WORK/mw.log"; exit 1; }
meson compile -C "$WORK/mwb" >>"$WORK/mw.log" 2>&1 || { tail -25 "$WORK/mw.log"; exit 1; }
"$WORK/mwb/smoke"

echo "== 5. meson install"
MPFX="$WORK/mprefix"
meson setup "$WORK/mi" "$ROOT" --prefix="$MPFX" -Denable_tests=false \
  >"$WORK/mi.log" 2>&1 || { tail -25 "$WORK/mi.log"; exit 1; }
meson install -C "$WORK/mi" >>"$WORK/mi.log" 2>&1 || { tail -25 "$WORK/mi.log"; exit 1; }
# No LD_LIBRARY_PATH: the installed plugin has to resolve its own runtime.
if (unset LD_LIBRARY_PATH; ldd "$MPFX/bin/capnpc-c" | grep 'not found'); then
  echo "installed capnpc-c cannot find libcapnp_c" >&2
  exit 1
fi
mpc=$(find "$MPFX" -name c-capnproto.pc | head -1)
grep -q '^codegen=' "$mpc" || { echo "meson .pc lacks codegen" >&2; exit 1; }

echo "ok packaging-smoke"
