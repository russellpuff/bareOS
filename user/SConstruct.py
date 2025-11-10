import re
import shutil
import subprocess
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Set

from SCons.Script import COMMAND_LINE_TARGETS, Exit, GetOption

USER_DIR = Path(Dir(".").abspath)
ROOT_DIR = USER_DIR.parent
BUILD_ROOT = USER_DIR / "build"
LOAD_DIR = ROOT_DIR / "load"
LIB_DIR = ROOT_DIR / "library"
LIB_INCLUDE_DIR = LIB_DIR / "include"
LINKER_SCRIPT = USER_DIR / "linker.ld"
START_SOURCE = USER_DIR / "start.s"

# Map tells the compiler what source files come with what header
# Prevents us from including unnecessary stuff in a user program
LIBRARY_HEADER_MAP: Dict[str, List[str]] = {
	"barelib.h": [],
    "util/string.h": ["src/util/string.c"],
    "util/limits.h": [],
    "dev/printf.h": ["src/dev/printf.c"],
    "dev/printf_iface.h": ["src/dev/io.c", "src/dev/printf.c"],
    "dev/ecall.h": ["src/dev/ecall.c"],
    "dev/io.h": ["src/dev/io.c", "src/dev/printf.c", "src/util/string.c", "src/dev/ecall.c"],
    "dev/time.h": ["src/dev/time.c", "src/dev/io.c", "src/util/string.c", "src/dev/ecall.c", "src/dev/printf.c"],
}

INCLUDE_PATTERN = re.compile(r'^\s*#include\s*([<"])([^">]+)[">]')

def _fail(message: str) -> None:
	print(f"[error] {message}")
	Exit(1)

# Clean build artifacts and .elf files copied to load/
# If the user put their own .elf file there manually, it's gone
# No way to verify, just don't do it, you can't run it anyways?
def _clean() -> None:
	if BUILD_ROOT.exists():
		shutil.rmtree(BUILD_ROOT)
	if LOAD_DIR.exists():
		for elf_path in LOAD_DIR.glob("*.elf"):
			elf_path.unlink()

# Detect if a usable compiler exists
def _detect_compiler() -> str:
	for prefix in (
		"riscv64-unknown-elf",
		"riscv64-unknown-linux-gnu",
		"riscv64-linux-gnu",
	):
		compiler = f"{prefix}-gcc"
		if shutil.which(compiler):
			return compiler
	_fail("no suitable RISC-V cross compiler found on PATH")
	raise AssertionError

# Search user source files for what headers they need from the shared library folder
def _parse_includes(paths: Iterable[Path]) -> Set[str]:
	headers: Set[str] = set()
	for path in paths:
		for line in path.read_text().splitlines():
			match = INCLUDE_PATTERN.match(line)
			if match and match.group(1) == "<":
				headers.add(match.group(2).strip())
	return headers

# Gets the sources in the mini project folder for building the user program
def _collect_sources(program_dir: Path) -> List[Path]:
	sources = sorted(program_dir.glob("*.c"))
	if not sources:
		_fail(f"no C sources found in {program_dir}")
	return sources

# Gets the sources for shared library files after determining which ones we need
def _resolve_library_sources(headers: Sequence[str], header_map: Dict[str, List[str]]) -> List[Path]:
	missing = [header for header in headers if header not in header_map]
	if missing:
		_fail(f"no library provides header(s): {", ".join(missing)}")
	resolved: Set[Path] = set()
	for header in headers:
		for source in header_map[header]:
			resolved.add(LIB_DIR / source)
	return sorted(resolved)

# Compile all the objects we've decided to include in this build.
# Skips duplicate objects, namely when two libraries share the same parent.
def _compile_objects(compiler: str, flags: Sequence[str], sources: Sequence[Path], output_dir: Path) -> List[Path]:
	objects: List[Path] = []
	seen: Set[Path] = set()
	for src in sources:
		if src in seen:
			continue
		obj_path = output_dir / (src.stem + ".o")
		command = [compiler, *flags, "-c", str(src), "-o", str(obj_path)]
		print(" ".join(command))
		subprocess.check_call(command)
		objects.append(obj_path)
		seen.add(src)
	return list(objects)


def _link(compiler: str, flags: Sequence[str], objects: Sequence[Path], output: Path, map_path: Path) -> None:
	command = [compiler, *flags, f"-Wl,-Map,{map_path}", "-o", str(output), *map(str, objects)]
	print(" ".join(command))
	subprocess.check_call(command)


if GetOption("clean"):
	_clean()
	Exit(0)

if not COMMAND_LINE_TARGETS or COMMAND_LINE_TARGETS[0] != "build":
	print("usage: scons build <program>")
	Exit(1)
if len(COMMAND_LINE_TARGETS) < 2:
	_fail("program name required (example: scons build shell)")
program_name = COMMAND_LINE_TARGETS[1]
program_dir = USER_DIR / program_name # programs can only be built if they have a subdir named after them
if not program_dir.is_dir():
	_fail(f"unknown program '{program_name}'")

if not START_SOURCE.is_file():
	_fail(f"start source missing: {START_SOURCE}")

if not LINKER_SCRIPT.is_file():
	_fail(f"linker script missing: {LINKER_SCRIPT}")

compiler = _detect_compiler()

sources = _collect_sources(program_dir)
angle_headers = sorted(_parse_includes([*sources, *program_dir.glob("*.h")]))
library_sources = _resolve_library_sources(angle_headers, LIBRARY_HEADER_MAP) if angle_headers else []

build_dir = BUILD_ROOT / program_name
if build_dir.exists():
	shutil.rmtree(build_dir)
build_dir.mkdir(parents=True, exist_ok=True)

common_flags = [
	"-std=gnu2x",
	"-Wall",
	"-Werror",
	"-fno-builtin",
	"-nostdlib",
	"-nostdinc",
	"-march=rv64imac_zicsr",
	"-mabi=lp64",
	"-mcmodel=medany",
	"-O0",
	"-g",
	f"-I{LIB_INCLUDE_DIR}",
	f"-I{program_dir}",
]

object_paths = _compile_objects(compiler, common_flags, sources, build_dir)
lib_objects = _compile_objects(compiler, common_flags, library_sources, build_dir)

start_obj = build_dir / "start.o"
start_cmd = [compiler, *common_flags, "-c", str(START_SOURCE), "-o", str(start_obj)]
print(" ".join(start_cmd))
subprocess.check_call(start_cmd)

all_objects = [start_obj, *object_paths, *lib_objects]

link_flags = [
	"-nostdlib",
	"-nostartfiles",
	f"-Wl,-T,{LINKER_SCRIPT}",
]

elf_path = build_dir / f"{program_name}.elf"
map_path = build_dir / f"{program_name}.map"
_link(compiler, link_flags, all_objects, elf_path, map_path)

LOAD_DIR.mkdir(parents=True, exist_ok=True)
shutil.copy2(elf_path, LOAD_DIR / f"{program_name}.elf") # copy compiled elf to the load directory

print(f"[done] built {program_name} -> {elf_path}")
