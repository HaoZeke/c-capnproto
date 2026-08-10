# Linux kernel builds (optional)

The runtime sources under `lib/` can be compiled into a kernel module
when `__KERNEL__` is defined (kbuild does this automatically). Userspace
is the default: meson, autotools, and CMake never enable a kernel build.

This is not a supported in-tree kbuild product. The sample under
`examples/kernel/` is examples-only.

## What the ifdefs change

When `__KERNEL__` is set:

- `stdio.h` / `stdlib.h` / `unistd.h` / `errno.h` are not included
- `linux/types.h`, `linux/string.h`, `linux/slab.h` are used instead
- `calloc` / `malloc` / `free` in `capn-malloc.c` map to
  `kcalloc` / `kmalloc` / `kfree` with `GFP_KERNEL`
- `FILE*` inflate/read paths (`fread`) are compiled out;
  `capn_init_mem` still works
- `EAGAIN` / `EINTR` retries in `capn_write_fd` are compiled out
- float converters (`capn_to_f32` and friends) are omitted

Userspace layouts, `ALIGNED_(8)` field attributes, `capn_size`,
`capn_write_mem`'s `int64_t` return, and the MSVC/FreeBSD headers are
unchanged.

`printk` is not used in the library; the sample module uses it.

Some runtime functions keep 4k stack buffers (`init_fp`, `capn_write_fd`).
kbuild may warn `-Wframe-larger-than=2048` on those paths. The sample
heap-allocates its encode buffer; a production module should do the same
or raise the frame limit.

## Sample module

`examples/kernel/` encodes the addressbook example at `insmod` time.

```sh
# needs a built capnpc-c only if you regenerate tests/addressbook.capnp.c
cd examples/kernel
make            # uses /lib/modules/$(uname -r)/build
sudo insmod capnp_addressbook.ko
dmesg | tail
sudo rmmod capnp_addressbook
```

`capn_init_fp` is userspace-only. Kernel consumers should use
`capn_init_malloc` + `capn_write_mem` / `capn_init_mem`.

## Out-of-tree kbuild

Point `ccflags-y` at `lib/` and list `capn.c`, `capn-malloc.c`,
`capn-stream.c` in your module. Do not copy those files; include the
tree. Generated `*.capnp.c` from `capnpc-c` can be compiled into the
same module if they only use the runtime API (no `stdio`).
