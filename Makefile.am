ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign subdir-objects

lib_LTLIBRARIES =
bin_PROGRAMS =
check_PROGRAMS =
EXTRA_DIST =
noinst_HEADERS =
include_HEADERS =


EXTRA_DIST += README.md KERNEL.md CMakeLists.txt CHANGELOG.md MAINTAINING.md SECURITY.md COPYING \
	VERSION.txt meson.build meson_options.txt subprojects/gtest.wrap \
	cmake/c_capnprotoConfig.cmake.in cmake/consumer_smoke/CMakeLists.txt \
	cmake/consumer_smoke/smoke.c
EXTRA_DIST += \
	examples/kernel/GNUmakefile \
	examples/kernel/Kbuild \
	examples/kernel/main.c \
	examples/kernel/runtime.c

EXTRA_DIST += c-capnproto.pc.in
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = c-capnproto.pc

EXTRA_DIST += compiler/c.capnp compiler/capnp/stream.capnp
capnp_DATA = compiler/c.capnp
AM_CPPFLAGS = \
	-I${srcdir}/compiler \
	-I${srcdir}/lib

lib_LTLIBRARIES += libcapnp_c.la
libcapnp_c_la_LDFLAGS = -version-info 1:0:0
libcapnp_c_la_CPPFLAGS = $(AM_CPPFLAGS) -DCAPNP_C_BUILDING
libcapnp_c_la_SOURCES = \
	lib/capn-malloc.c \
	lib/capn-stream.c \
	lib/capn.c \
	lib/capn-rpc.c \
	lib/capn-dyn.c \
	compiler/rpc.capnp.c \
	compiler/rpc-threeparty.capnp.c
EXTRA_DIST += \
	lib/capn-list.inc

bin_PROGRAMS += capnpc-c
capnpc_c_SOURCES = \
	compiler/capnpc-c.c \
	compiler/codecgen.c \
	compiler/reflectgen.c \
	compiler/schema.capnp.c \
	compiler/str.c
capnpc_c_LDADD = libcapnp_c.la
include_HEADERS += \
	lib/capnp_c.h \
	lib/capn-rpc.h \
	lib/capn-dyn.h \
	compiler/c.capnp.h

noinst_HEADERS += \
	lib/capnp_priv.h \
	compiler/str.h \
	compiler/codecgen.h \
	compiler/reflectgen.h \
	compiler/schema.capnp.h \
	compiler/c++.capnp.h \
	compiler/rpc.capnp.h \
	compiler/rpc-threeparty.capnp.h

# Don't try to generate any *.capnp files. Otherwise make wants to compile them
# from *.capnp.c and fails.
%.capnp: ;

# googletest is optional. @gtest_dist_subdirs@ is set only when the
# suite is configured, so automake does not require the directory.
DIST_SUBDIRS = @gtest_dist_subdirs@

if WITH_GTEST
# A release tarball has no submodule, so use the system library there.
# An in-tree checkout keeps building the bundled copy.
if BUNDLED_GTEST
GTEST_LDADD = gtest/googletest/lib/libgtest.la
gtest/googletest/lib/libgtest.la:
	make -C gtest/googletest lib/libgtest.la
GTEST_CPPFLAGS = -I${srcdir}/gtest/googletest/include
else
GTEST_LDADD = -lgtest
GTEST_CPPFLAGS =
endif

# gtest distributes fused sources and rebuilds them in the source tree.
# Create any missing files, then write a marker that is not older than
# those inputs so the rebuild does not run. A read-only distcheck tree
# already contains the files, so this only writes the marker.
BUILT_SOURCES = gtest-fused-prepare
DISTCLEANFILES = gtest/googletest/fused-gtest
gtest-fused-prepare:
	@if test ! -f $(srcdir)/gtest/googletest/fused-src/gtest/gtest-all.cc \
	    || test ! -f $(srcdir)/gtest/googletest/fused-src/gtest/gtest.h \
	    || test ! -f $(srcdir)/gtest/googletest/fused-src/gtest/gtest_main.cc; then \
	  $(MKDIR_P) $(srcdir)/gtest/googletest/fused-src && \
	  python3 $(srcdir)/gtest/googletest/scripts/fuse_gtest_files.py $(srcdir)/gtest/googletest/fused-src && \
	  cp -f $(srcdir)/gtest/googletest/src/gtest_main.cc $(srcdir)/gtest/googletest/fused-src/gtest/; \
	fi
	@$(MKDIR_P) gtest/googletest
	@touch -r $(srcdir)/gtest/googletest/fused-src/gtest/gtest-all.cc gtest/googletest/fused-gtest

# Tests
check_PROGRAMS += \
	capn-test
capn_test_SOURCES = \
	tests/capn-test.cpp \
	tests/capn-stream-test.cpp \
	tests/example-test.cpp \
	tests/fortran-golden-test.cpp \
	tests/extraheader-test.cpp \
	tests/fieldgetset-test.cpp \
	tests/codegen-defaults-test.cpp \
	tests/bounds-test.cpp \
	tests/interface-test.cpp \
	tests/list-c6-upgrade-test.cpp \
	tests/constants-test.cpp \
	tests/testunion-union1-test.cpp \
	tests/addressbook.capnp.c \
	tests/extraheader.capnp.c \
	tests/fieldgetset.capnp.c \
	tests/codegen-defaults.capnp.c \
	compiler/test.capnp.c \
	compiler/schema-test.cpp \
	compiler/schema.capnp.c
noinst_HEADERS += \
	compiler/test.capnp.h \
	tests/addressbook.capnp.h \
	tests/extraheader.capnp.h \
	tests/extraheader-probe.h \
	tests/fieldgetset.capnp.h \
	tests/codegen-defaults.capnp.h
endif

# Meson builds its tests from a tarball, so the lists below stay
# outside WITH_GTEST.
EXTRA_DIST += \
	tests/null-ptr-write-test.c \
	tests/packed-header-write-test.c \
	.github/workflows/ci.yml

EXTRA_DIST += \
	compiler/c.capnp \
	compiler/capnp/stream.capnp \
	compiler/c++.capnp \
	scripts/check-stream-schema.sh \
	compiler/schema.capnp \
	compiler/test.capnp \
	tests/addressbook.capnp \
	tests/extraheader.capnp \
	tests/codegen-defaults.capnp \
	tests/codec-roundtrip.capnp \
	tests/codec-roundtrip.h \
	tests/codec-roundtrip-test.cpp \
	tests/group-in-union.capnp \
	tests/group-in-union.h \
	tests/group-in-union-test.cpp \
	tests/fieldgetset.capnp \
	tests/create-min-sz-test.cpp \
	tests/fortran-golden.md \
	compiler/adder.capnp \
	compiler/adder.capnp.c \
	compiler/adder.capnp.h \
	compiler/c.capnp.c \
	compiler/c++.capnp.c \
	compiler/rpc.capnp \
	compiler/rpc.capnp.h \
	compiler/rpc-threeparty.capnp \
	compiler/rpc-threeparty.capnp.h \
	compiler/rpc-threeparty-frames.capnp \
	compiler/rpc-twoparty.capnp \
	compiler/rpc-twoparty.capnp.c \
	compiler/rpc-twoparty.capnp.h \
	compiler/update-notes.md \
	interop/rpc_client.c \
	interop/rpc_peer_server.c++ \
	interop/run_rpc_interop.sh \
	scripts/gen-rpc-frames.sh \
	scripts/packaging-smoke.sh \
	scripts/regen-generated.sh \
	scripts/smoke-m32.sh \
	tests/compiler-str-test.c \
	tests/data-codec.capnp \
	tests/data-codec.h \
	tests/data-codec-test.cpp \
	tests/empty-list.capnp \
	tests/empty-list.h \
	tests/empty-list-test.cpp \
	tests/list-decode-srcdest.capnp \
	tests/list-decode-srcdest.h \
	tests/list-decode-srcdest-test.cpp \
	tests/null-text.capnp \
	tests/null-text.h \
	tests/null-text-test.cpp \
	tests/rpc-client-test.cpp \
	tests/rpc-handoff-test.cpp \
	tests/rpc-join-test.cpp \
	tests/fixtures/rpc-accept.bin \
	tests/fixtures/rpc-introduce.bin \
	tests/fixtures/rpc-provide.bin \
	tests/fixtures/addressbook.bin \
	tests/fixtures/addressbook.packed.bin \
	tests/fixtures/addressbook.canonical.bin \
	tests/fixtures/addressbook.txt \
	tests/fixtures/testalltypes.txt \
	tests/capnp-cli-interop-test.cpp \
	tests/capnp-cli.h \
	tests/reflect-test.cpp \
	tests/reflect.capnp \
	tests/rpcstubs.capnp \
	tests/rpcstubs-test.cpp \
	tests/reflect-other.capnp \
	tests/interface-field.capnp \
	tests/interface-field-test.cpp \
	fuzz/Person.txt \
	fuzz/in/Person.bin \
	fuzz/read_mem.c \
	fuzz/read_fp.c \
	fuzz/read_message.c \
	fuzz/rpc_frame.c \
	fuzz/dyn_text.c \
	fuzz/in/cap-index-frame.bin \
	fuzz/in/dyn-cap-16.bin \
	scripts/fuzz-smoke.sh \
	interop/rpc_server.c \
	interop/rpc_peer_client.c++

if WITH_GTEST
capn_test_CPPFLAGS = $(AM_CPPFLAGS) $(GTEST_CPPFLAGS) \
	-DEXTRAHEADER_GENERATED_H=\"$(abs_srcdir)/tests/extraheader.capnp.h\" \
	-DCODEGEN_DEFAULTS_GENERATED_C=\"$(abs_srcdir)/tests/codegen-defaults.capnp.c\" \
	-DCODEGEN_DEFAULTS_GENERATED_H=\"$(abs_srcdir)/tests/codegen-defaults.capnp.h\" \
	-DTEST_CAPNP_GENERATED_C=\"$(abs_srcdir)/compiler/test.capnp.c\" \
	-DFORTRAN_GOLDEN_DIR=\"$(abs_srcdir)/tests/fixtures\"
capn_test_CXXFLAGS = -std=gnu++14 -pthread
capn_test_LDADD = libcapnp_c.la $(GTEST_LDADD)
capn_test_LDFLAGS = -pthread
TESTS = capn-test
endif

CAPNP_SCHEMA_FILES := $(shell find . -type f -name \*.capnp)

CAPNP ?= capnp
.PHONY: capnp-compile fuzz-mem fuzz-fp gtest-fused-prepare
capnp-compile:
	$(CAPNP) compile \
		--output=./capnpc-c \
		-Icompiler \
		$(CAPNP_SCHEMA_FILES)

# fuzz
AFLCC ?= afl-clang
AFL-FUZZ ?= afl-fuzz

fuzz/in/Person.bin: fuzz/Person.txt
	mkdir -p fuzz/in
	$(CAPNP) encode tests/addressbook.capnp Person < $< > $@

fuzz/read-mem: fuzz/read_mem.c tests/addressbook.capnp.c lib/capn-malloc.c lib/capn-stream.c lib/capn.c
	AFL_USE_ASAN=1 $(AFLCC) $(CFLAGS) -Ilib -Itests -Icompiler  $^ -o $@

fuzz/read-fp: fuzz/read_fp.c tests/addressbook.capnp.c lib/capn-malloc.c lib/capn-stream.c lib/capn.c
	AFL_USE_ASAN=1 $(AFLCC) $(CFLAGS) -Ilib -Itests -Icompiler  $^ -o $@

fuzz-mem: fuzz/read-mem fuzz/in/Person.bin
	$(AFL-FUZZ) -i fuzz/in -o fuzz/out ./fuzz/read-mem @@

fuzz-fp: fuzz/read-fp fuzz/in/Person.bin
	$(AFL-FUZZ) -i fuzz/in -o fuzz/out ./fuzz/read-fp @@
