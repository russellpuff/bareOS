# Kernel build graph and runtime helpers for bareOS.
# Focuses on kernel-specific sources, QEMU integration, and loader preparation.

from __future__ import annotations

import os
import re
import subprocess
from pathlib import Path

from SCons.Script import COMMAND_LINE_TARGETS, Copy, Exit, File, Glob

Import("env")

IMG_FILE = "bareOS.elf"
HEAD_DIR = Path(Dir("load").abspath)
KERNEL_ROOT = Path(Dir("#/kernel").abspath)
LIBRARY_ROOT = Path(Dir("#/library").abspath)

# To avoid accidentally compiling library files the kernel shouldn't use,
# we create this list of ok files for the build script to select from.
# Later when these libraries become largely immutable, we'll move to an
# archive format where we just ask for the archive like a dll.
KERNEL_LIBRARY_SOURCES = [
		"src/util/string.c",
		"src/dev/printf.c",
		"src/dev/ecall.c",
		"src/dev/time.c",
]

# Print the provided message then abort the build with the given code.
def error(code: int, msg: str) -> None:
	print(msg)
	Exit(code)

# Return a list of kernel source nodes discovered automatically.
def _gather_kernel_sources() -> list:
	kernel_files = []
	for directory in sorted(KERNEL_ROOT.iterdir()):
		if directory.name == "include" or not directory.is_dir():
			continue
		pattern = os.path.join("kernel", directory.name, "*.[cs]")
		kernel_files.extend(Glob(pattern))
	return kernel_files

# Collect shared library files that the kernel links against.
# Unlike the user side, we manually declare what library files we need. 
# Mainly because it's way too much work to scan the large codebase.
def _kernel_library_sources() -> list:
	sources = []
	sources = []
	for candidate in KERNEL_LIBRARY_SOURCES:
		path = LIBRARY_ROOT / candidate
		if path.exists(): # Safety check in case of library update without scons update
			sources.append(File(os.path.join("library", candidate)))
	return sources

# Build loader metadata and return QEMU flag fragments.
# This runs immediately after build to scan the memmap for the location of kernel heap/stack.
def prepare_generic_loader(env) -> str:
	start_addr = 24  # sizeof(alloc_t)
	try:
		with open(str(env["memmap"])) as mf:
			heap_addr = None
			for line in mf:
				match = re.search(r"(0x[0-9a-fA-F]+)\s+PROVIDE \(mem_start", line)
				if match:
					heap_addr = int(match.group(1), 16)
					break
		if heap_addr is not None:
			print(f"[Info] Predicted heap start (mem_start): 0x{heap_addr:08x}")
			if 0x80200000 > heap_addr:
				start_addr += 0x80200000
			else:
				start_addr += heap_addr
			print(f"[Info] Generic importer start location: 0x{start_addr:08x}")
		else:
			print("[Warning] Could not locate mem_start in linker map; skipping file load.")
	except OSError as exc:
		print(f"[Warning] Failed to read linker map for heap prediction: {exc}")

	filename_len = 56 # from format.h
	max_files = 100 # Arbitrary limit since the ramdisk has no limits
	max_total_payload = int(1.5 * 1024 * 1024) # Abitrary limit to avoid maxing out the ramdisk on boot
	header_size = filename_len + 4

	# Encode the filename into the fixed-width header field.
	def do_name_bytes(filename: str) -> bytes | None:
		encoded = filename.encode("utf-8")
		if len(filename) > filename_len - 1:
			return None
		return encoded[:filename_len].ljust(filename_len, b"\0")

	# Convert the supplied size into the little-endian header payload.
	def do_size_bytes(size: int) -> bytes:
		return int(size).to_bytes(4, "little")

	load_flags = []

	if start_addr > 24:
		current = start_addr + 1
		load_dir = Path(Dir("#/load").abspath)
		load_dir.mkdir(parents=True, exist_ok=True)
		HEAD_DIR.mkdir(parents=True, exist_ok=True)

		files = sorted(
			f for f in load_dir.iterdir() if f.is_file()
		)

		valid_files = 0
		total_payload_bytes = 0

		for path in files:
			if valid_files == max_files:
				break
			name_bytes = do_name_bytes(path.name)
			if name_bytes is None:
				continue

			f_size = path.stat().st_size
			if total_payload_bytes + f_size > max_total_payload:
				continue

			# Create .gih (generic importer header) files so the importer
			# Can write header data in a single burst (opposed to 8-byte
			# direct writes). These get cleaned when running scons -c.
			header_path = HEAD_DIR / f"{path.name}.gih"
			header = name_bytes + do_size_bytes(f_size)
			header_path.write_bytes(header)

			load_flags.append(f"-device loader,file={header_path},addr=0x{current:08x}")
			current += header_size

			load_flags.append(
				f"-device loader,file={path},addr=0x{current:08x},force-raw=on"
			)
			current += f_size

			total_payload_bytes += f_size
			valid_files += 1

		load_flags.append(
			f"-device loader,addr={start_addr:#x},data={valid_files:#x},data-len=1"
		)

	return " ".join(load_flags)


all_kernel_sources = _gather_kernel_sources() + _kernel_library_sources()
objs = env.Object(all_kernel_sources)
elf = env.Program(IMG_FILE, objs)
elf = env.Command(None, elf, Copy(IMG_FILE, "$SOURCE"))
env.Alias("build", elf)


# Append the GDB stub configuration to QEMU if running in debug mode.
def _apply_debug_qemu_flags() -> None:
	if env.get("DEBUG_MODE"):
		env["qflags"] += f" -S -gdb tcp::{env['port']}"


_apply_debug_qemu_flags()


# Write the fully-expanded QEMU argument list to the output file.
# Used by run_os.sh for running without scons surrounding execution.
def write_qemu_flags(target, source, env):
	load_flags = prepare_generic_loader(env)
	flag_string = env["qflags"]
	if load_flags:
		flag_string = f"{flag_string} {load_flags}".strip()
	target_path = Path(str(target[0]))
	target_path.parent.mkdir(parents=True, exist_ok=True)
	target_path.write_text(flag_string + "\n")
	print(flag_string)
	return 0

qemu_flags_file = env.Command("#/.build/qemu_flags.txt", [], write_qemu_flags)
env.Depends(qemu_flags_file, elf)
env.AlwaysBuild(qemu_flags_file)
env.Alias("build", [elf, qemu_flags_file])

# Launch bareOS under QEMU with the prepared loader data.
def run_qemu(target, source, env):
	# Allow the user to run without the shell. Note the system panics if it has no shell. 
	# If "no-shell" is used but the shell was built, it'll get loaded anyways.
	if "no-shell" not in COMMAND_LINE_TARGETS:
		load_dir = Path(Dir("#/load").abspath)
		shell_path = load_dir / "shell.elf"
		if not load_dir.is_dir() or not shell_path.is_file():
			error(
				1,
				"[Error] Missing load/shell.elf; you probably need to build the shell "
				"(try `scons build shell` from the user/ directory).",
			)

		load_flags = prepare_generic_loader(env)
		print(env["qflags"])
		if load_flags:
			env.Append(qflags=" " + load_flags)
			print(load_flags)

		args = [env["QEMU"], "-kernel", str(source[0])] + env["qflags"].split()
		if not env.get("DEBUG_MODE"):
			subprocess.run("clear", shell=True, check=True)
		return subprocess.call(args)


run = env.Command(target="run_cmd", source=IMG_FILE, action=run_qemu)
env.Depends(run, elf)
env.AlwaysBuild(run)
env.Alias("run", run)

# --------------------  Environment Management  ----------------------

env.Clean(elf, env["memmap"])
env.Clean(elf, HEAD_DIR)
env.Clean(elf, f"#{IMG_FILE}")
