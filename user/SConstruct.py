import json
import os
import re
import shutil
import stat
import subprocess
from pathlib import Path
from typing import Dict, List, Set, Tuple

from SCons.Script import ARGUMENTS, COMMAND_LINE_TARGETS, Exit, GetOption

USER_DIR = Path(Dir('.').abspath)
ROOT_DIR = USER_DIR.parent
BUILD_ROOT = ROOT_DIR / 'user' / 'build'
LOAD_DIR = ROOT_DIR / 'load'
KERNEL_INCLUDE = ROOT_DIR / 'kernel' / 'include'
LINKER_SCRIPT = USER_DIR / 'linker.ld'
START_SOURCE = USER_DIR / 'start.s'
LIB_MAP_PATH = USER_DIR / 'libmap.json'

AVAILABLE_PROGRAMS = sorted(
	entry
	for entry in USER_DIR.iterdir()
	if entry.is_dir() and not entry.name.startswith('.')
)


def _print_usage() -> None:
	print("usage: scons build <program> [<program> ...]")
	if AVAILABLE_PROGRAMS:
		joined = ', '.join(entry.name for entry in AVAILABLE_PROGRAMS)
		print(f"available programs: {joined}")
	else:
		print("no user programs were found")


def _safe_remove(path: Path) -> None:
	if path.is_dir():
		print(f"scons: Removing build directory: {path}")
		shutil.rmtree(path)
	elif path.is_file():
		print(f"scons: Removing build artifact: {path}")
		path.unlink()


if GetOption('clean'):
	if BUILD_ROOT.exists():
		_safe_remove(BUILD_ROOT)
	if LOAD_DIR.exists():
		removed = False
		for program in AVAILABLE_PROGRAMS:
			elf_path = LOAD_DIR / f"{program.name}.elf"
			if elf_path.exists():
				if not removed:
					print("scons: Removing generated load artifacts")
					removed = True
				_safe_remove(elf_path)
	Exit(0)


def _detect_arch() -> str:
	arch = ARGUMENTS.get('arch')
	if arch:
		if not shutil.which(f"{arch}-gcc"):
			print(f"[error] requested toolchain '{arch}' not found on PATH")
			Exit(1)
		return arch
	for candidate in (
		'riscv64-unknown-elf',
		'riscv64-unknown-linux-gnu',
		'riscv64-linux-gnu',
	):
		if shutil.which(f"{candidate}-gcc"):
			return candidate
	print("[error] no suitable RISC-V cross compiler found on PATH")
	Exit(1)


def _load_library_map() -> List[Dict[str, object]]:
	if not LIB_MAP_PATH.is_file():
		print(f"[error] library map missing: {LIB_MAP_PATH}")
		Exit(1)
	try:
		data = json.loads(LIB_MAP_PATH.read_text())
	except Exception as exc:  # pragma: no cover - defensive
		print(f"[error] failed to parse {LIB_MAP_PATH}: {exc}")
		Exit(1)
	libraries = data.get('libraries', [])
	if not isinstance(libraries, list):
		print("[error] invalid library map format: 'libraries' should be a list")
		Exit(1)
	for lib in libraries:
		if not all(key in lib for key in ('name', 'script', 'artifact', 'headers')):
			print(f"[error] malformed library entry: {lib}")
			Exit(1)
		headers = lib.get('headers', [])
		if not isinstance(headers, list):
			print(f"[error] library headers must be a list: {lib}")
			Exit(1)
	return libraries


INCLUDE_PATTERN = re.compile(r'^\s*#include\s*([<"])([^">]+)[">]')


def _scan_program(program_dir: Path, libraries: List[Dict[str, object]]) -> Tuple[List[Path], List[Dict[str, object]]]:
	sources = sorted(program_dir.glob('*.c'))
	if not sources:
		print(f"[error] no C sources found in {program_dir}")
		Exit(1)

	headers = sorted(program_dir.glob('*.h'))
	files_to_scan = sources + headers

	local_headers: Set[str] = set()
	angle_headers: Set[str] = set()

	for path in files_to_scan:
		try:
			text = path.read_text()
		except OSError as exc:
			print(f"[error] failed to read {path}: {exc}")
			Exit(1)
		for line in text.splitlines():
			match = INCLUDE_PATTERN.match(line)
			if not match:
				continue
			delimiter, header = match.group(1), match.group(2).strip()
			if delimiter == '<':
				angle_headers.add(header)
			else:
				local_headers.add(header)

	missing_locals = sorted(header for header in local_headers if not (program_dir / header).is_file())
	if missing_locals:
		for header in missing_locals:
			print(f"[error] missing local header: {header}")
		Exit(1)

	header_to_libs: Dict[str, List[Dict[str, object]]] = {}
	for lib in libraries:
		for header in lib.get('headers', []):
			header_to_libs.setdefault(header, []).append(lib)

	required_headers = sorted(angle_headers)
	if required_headers:
		for header in required_headers:
			if header not in header_to_libs:
				print(f"[error] no library provides header: {header}")
				Exit(1)

	selected_libs: List[Dict[str, object]] = []
	uncovered = set(required_headers)
	available_libs = libraries[:]

	while uncovered:
		best_lib = None
		best_cover: Set[str] = set()
		for lib in available_libs:
			covers = set(lib.get('headers', [])) & uncovered
			if len(covers) > len(best_cover):
				best_lib = lib
				best_cover = covers
		if best_lib is None:
			print(f"[error] could not resolve libraries for headers: {sorted(uncovered)}")
			Exit(1)
		selected_libs.append(best_lib)
		available_libs.remove(best_lib)
		uncovered -= set(best_lib.get('headers', []))

	return sources, selected_libs
def _detect_compiler() -> str:
	arch = _detect_arch()
	return f"{arch}-gcc"


def _run_command(command: List[str]) -> None:
	print(' '.join(command))
	subprocess.check_call(command)


def _prepare_script(script_path: Path) -> None:
	try:
		data = script_path.read_bytes()
	except OSError as exc:
		print(f"[error] failed to read script {script_path}: {exc}")
		Exit(1)

	if b"\r\n" in data:
		try:
			script_path.write_bytes(data.replace(b"\r\n", b"\n"))
		except OSError as exc:
			print(f"[error] failed to normalize line endings for {script_path}: {exc}")
			Exit(1)

	try:
		mode = script_path.stat().st_mode
	except OSError as exc:
		print(f"[error] failed to stat script {script_path}: {exc}")
		Exit(1)

	desired_mode = mode | stat.S_IXUSR
	if desired_mode != mode:
		try:
			script_path.chmod(desired_mode)
		except OSError as exc:
			print(f"[error] failed to set execute permissions on {script_path}: {exc}")
			Exit(1)


def _run_library_script(script_path: Path) -> None:
	if not script_path.exists():
		print(f"[error] library build script missing: {script_path}")
		Exit(1)

	_prepare_script(script_path)

	if os.access(script_path, os.X_OK):
		command = [str(script_path)]
	else:
		command = ['bash', str(script_path)]

	print(f"[info] running library script: {script_path}")
	_run_command(command)


def _run_all_library_scripts() -> None:
	scripts = sorted(USER_DIR.glob('build_*_lib.sh'))
	if not scripts:
		print('[info] no library build scripts found to execute')
		Exit(0)

	for script_path in scripts:
		_run_library_script(script_path)

	Exit(0)


if not COMMAND_LINE_TARGETS or COMMAND_LINE_TARGETS[0] != 'build':
	_print_usage()
	Exit(1)

program_names = COMMAND_LINE_TARGETS[1:]
if not program_names:
	_print_usage()
	Exit(1)

if program_names == ['lib']:
	_run_all_library_scripts()

available_program_names = {entry.name for entry in AVAILABLE_PROGRAMS}
for program in program_names:
	if program not in available_program_names:
		print(f"[error] unknown program '{program}'")
		_print_usage()
		Exit(1)

if not START_SOURCE.is_file():
	print(f"[error] start source missing: {START_SOURCE}")
	Exit(1)

if not LINKER_SCRIPT.is_file():
	print(f"[error] linker script missing: {LINKER_SCRIPT}")
	Exit(1)

cc = _detect_compiler()
libraries = _load_library_map()

base_compile_flags = [
	'-std=gnu2x',
	'-Wall',
	'-Werror',
	'-fno-builtin',
	'-nostdlib',
	'-nostdinc',
	'-march=rv64imac_zicsr',
	'-mabi=lp64',
	'-mcmodel=medany',
	'-O0',
	'-g',
	f'-I{KERNEL_INCLUDE}',
]

link_flags = [
	'-nostdlib',
	'-nostartfiles',
	f'-Wl,-T,{LINKER_SCRIPT}',
]

executed_scripts: Set[Path] = set()

for program in program_names:
	program_dir = USER_DIR / program
	print(f"[info] building user program '{program}'")
	sources, selected_libs = _scan_program(program_dir, libraries)

	build_dir = BUILD_ROOT / program
	if build_dir.exists():
		shutil.rmtree(build_dir)
	build_dir.mkdir(parents=True, exist_ok=True)

	include_flags = [*base_compile_flags, f'-I{program_dir}']

	object_paths: List[Path] = []
	for src in sources:
		obj_path = build_dir / (src.stem + '.o')
		compile_cmd = [cc, *include_flags, '-c', str(src), '-o', str(obj_path)]
		_run_command(compile_cmd)
		object_paths.append(obj_path)
		print(f"built {obj_path}")

	start_obj = build_dir / 'start.o'
	start_cmd = [cc, *include_flags, '-c', str(START_SOURCE), '-o', str(start_obj)]
	_run_command(start_cmd)
	object_paths.insert(0, start_obj)

	lib_paths: List[Path] = []
	for lib in selected_libs:
		script_rel = lib.get('script', '') or ''
		artifact_rel = lib.get('artifact', '') or ''

		if script_rel:
			script_path = USER_DIR / script_rel
			if script_path not in executed_scripts:
				_run_library_script(script_path)
				executed_scripts.add(script_path)

		if artifact_rel:
			artifact_path = ROOT_DIR / artifact_rel
			if not artifact_path.exists():
				print(f"[error] expected library artifact not found: {artifact_path}")
				Exit(1)
			if artifact_path not in lib_paths:
				lib_paths.append(artifact_path)

	map_path = build_dir / f'{program}.map'
	elf_path = build_dir / f'{program}.elf'
	link_cmd = [cc, *link_flags, f'-Wl,-Map,{map_path}', '-o', str(elf_path), *map(str, object_paths), *map(str, lib_paths)]
	_run_command(link_cmd)
	print(f"{program}.elf written to {elf_path}")

	LOAD_DIR.mkdir(parents=True, exist_ok=True)
	dest = LOAD_DIR / f'{program}.elf'
	shutil.copy2(elf_path, dest)
	print(f"copied {elf_path} to {dest}")

Exit(0)
