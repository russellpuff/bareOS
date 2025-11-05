# bareOS

bareOS is a small RISC-V operating system written in C (gnu2x) that began at Indiana University and now continues as an open research project meant to explore the principles of computer operating systems. Under the guidance of Professor Jeremy Musser, the bootstrap and some of the low-level kernel were developed by him; subsequent features and applications are being added by me.

## Features
- 64-bit RISC-V kernel that runs under QEMU.
- Preemptive threading and basic synchronization primitives.
- Simple RAM-disk file system and a minimal shell.
- Basic virtual memory support, user binaries can be imported and run as programs.

## Getting Started

### Prerequisites
On a Debian/Ubuntu system the following commands install the required toolchain and dependencies. The absolute ideal is to build your own cross compiler tools, but unknown-elf should work to achieve the intended bare-metal approach. 

```sh
sudo apt-get install -y --no-install-recommends \
     qemu-system-riscv64 \
     gcc-riscv64-unknown-elf \
     gdb-multiarch
     
pip install scons
```

The build scripts automatically locate an available RISC-V cross compiler, if you installed the unknown-elf toolchain earlier, then that will be used by default. If multiple toolchains are installed you can select one explicitly with `scons arch=<prefix>` (for example, `scons arch=riscv64-linux-gnu`).

### Building and Running

```sh
git clone git@github.com:russellpuff/bareOS.git
cd bareOS/user/
scons build lib               # build user libraries
scons build shell             # build the shell (mandatory)
cd ..
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
