# bareOS

bareOS is a small RISC-V operating system that began at Indiana University and now continues as an open research project meant to explore the principles of computer operating systems. Under the guidance of Professor Jeremy Musser, the bootstrap and much of the low-level kernel were developed by him; subsequent features and applications are being added by me.

## Features
- 64-bit RISC-V kernel that runs under QEMU.
- Preemptive threading and basic synchronization primitives.
- Simple RAM-disk file system and a minimal shell.

## Getting Started

### Prerequisites
On a Debian/Ubuntu system the following commands install the required toolchain and dependencies:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential python3 python3-pip git ca-certificates \
  qemu-system-riscv64 \
  gcc-riscv64-linux-gnu g++-riscv64-linux-gnu \
  binutils-riscv64-linux-gnu \
  gdb-multiarch

pip install scons
```

The build scripts expect the cross compiler tools to use the `riscv64-unknown-linux-gnu-*` prefix. The snippet below creates the necessary symbolic links and ensures `~/.local/bin` is on your `PATH`:

```sh
mkdir -p /usr/local/bin
for t in gcc g++ ld as ar ranlib objcopy objdump strip nm readelf; do
  if command -v "riscv64-linux-gnu-$t" >/dev/null 2>&1; then
    sudo ln -sf "$(command -v riscv64-linux-gnu-$t)" "/usr/local/bin/riscv64-unknown-linux-gnu-$t"
  fi
done

echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.profile
source ~/.profile
```

### Building and Running

```sh
git clone git@github.com:russellpuff/bareOS.git
cd bareOS
mkdir load                    # optional: add files here to preload them into the ramdisk
scons run                     # build and launch bareOS in QEMU
```

Use the `shutdown` command inside bareOS to exit QEMU. Clean build artifacts with:

```sh
scons -c
```

Files placed in the `load/` directory are injected into QEMU at boot and written to bareOS's RAM disk. The file system can hold up to 16 entries with names up to 15 characters. Each file may occupy at most 12 blocks of 512 bytes, which is 6KiB.

See [`kernel/include/fs.h`](kernel/include/fs.h) for more details about these limits.

## License
MIT. See [LICENSE](LICENSE).
