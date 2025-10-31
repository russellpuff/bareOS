import os
import shutil
import subprocess
from SCons.Script import COMMAND_LINE_TARGETS, Exit, GetOption

USER_DIR = Dir('.').abspath
ROOT_DIR = Dir('#').abspath
BUILD_ROOT = os.path.join(ROOT_DIR, 'user', 'build')
LOAD_DIR = os.path.join(ROOT_DIR, 'load')
SCRIPT_PATH = os.path.join(USER_DIR, 'build_user_program.sh')

AVAILABLE_PROGRAMS = sorted(
    entry
    for entry in os.listdir(USER_DIR)
    if os.path.isdir(os.path.join(USER_DIR, entry))
    and not entry.startswith('.')
)

def _print_usage():
    print("usage: scons build <program> [<program> ...]")
    if AVAILABLE_PROGRAMS:
        joined = ', '.join(AVAILABLE_PROGRAMS)
        print(f"available programs: {joined}")
    else:
        print("no user programs were found")

if GetOption('clean'):
    if os.path.isdir(BUILD_ROOT):
        print(f"scons: Removing build directory: {BUILD_ROOT}")
        shutil.rmtree(BUILD_ROOT)
    if os.path.isdir(LOAD_DIR):
        removed = False
        for program in AVAILABLE_PROGRAMS:
            elf_path = os.path.join(LOAD_DIR, f"{program}.elf")
            if os.path.exists(elf_path):
                if not removed:
                    print("scons: Removing generated load artifacts")
                    removed = True
                os.remove(elf_path)
                print(f"  removed {elf_path}")
    Exit(0)

if not os.path.isfile(SCRIPT_PATH):
    print(f"[error] build script missing: {SCRIPT_PATH}")
    Exit(1)

if not COMMAND_LINE_TARGETS or COMMAND_LINE_TARGETS[0] != 'build':
    _print_usage()
    Exit(1)

programs = COMMAND_LINE_TARGETS[1:]
if not programs:
    _print_usage()
    Exit(1)

for program in programs:
    if program not in AVAILABLE_PROGRAMS:
        print(f"[error] unknown program '{program}'")
        _print_usage()
        Exit(1)

for program in programs:
    print(f"[info] building user program '{program}'")
    result = subprocess.call([SCRIPT_PATH, program], cwd=USER_DIR)
    if result != 0:
        Exit(result)

Exit(0)