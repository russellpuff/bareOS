# bareOS

bareOS is a small RISC-V operating system written in C (gnu2x) that began at Indiana University and now continues as an open research project meant to explore the principles of computer operating systems. Under the guidance of Professor Jeremy Musser, the bootstrap and some of the low-level kernel were developed by him; subsequent features and applications are being added by me.

## Features
- 64-bit RISC-V kernel that runs under QEMU.
- Preemptive threading and basic synchronization primitives.
- Simple RAM-disk file system and a basic shell.
- Basic virtual memory support, user binaries can be imported and run as programs.
- Separation of user space and kernel space, users must ecall to access hardware.

## Getting Started

### Prerequisites
On a Debian/Ubuntu system the following commands install the required toolchain and dependencies. The absolute ideal is to build your own cross compiler tools, but unknown-elf should work to achieve the intended bare-metal approach. 

```sh
sudo apt-get install -y --no-install-recommends qemu-system-riscv64 gcc-riscv64-unknown-elf gdb-multiarch
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

If you are running in an automated testing environment, SCons can cause stdio related fuckery. 
To fix this, just run the provided helper that automates a full rebuild and QEMU launch without SCons.
```sh
./run_os.sh [--debug]
```

Use the `shutdown` command inside bareOS to exit QEMU. Clean build artifacts with:

```sh
scons -c
```

Files placed in the `load/` directory are injected into QEMU at boot and written to bareOS's RAM disk. These loaded files have a name limit of 56 bytes and a total file limit of 100 files and 1.5 MiB combined filesize. ELF binaries compiled in the user/ folder will be automatically placed in the /bin directory while everything else is placed in the /home directory.

## License
MIT. See [LICENSE](LICENSE).
