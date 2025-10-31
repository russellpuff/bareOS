#!/bin/bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <program-name>" >&2
  exit 1
fi

PROGRAM="$1"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
USER_DIR="${ROOT_DIR}/user"
PROGRAM_DIR="${USER_DIR}/${PROGRAM}"

if [[ ! -d "${PROGRAM_DIR}" ]]; then
  echo "[error] program directory not found: ${PROGRAM_DIR}" >&2
  exit 1
fi

LIB_MAP_PATH="${USER_DIR}/libmap.json"
if [[ ! -f "${LIB_MAP_PATH}" ]]; then
  echo "[error] library map missing: ${LIB_MAP_PATH}" >&2
  exit 1
fi

ARCHES=(
  "riscv64-unknown-elf"
  "riscv64-unknown-linux-gnu"
  "riscv64-linux-gnu"
)
for candidate in "${ARCHES[@]}"; do
  if command -v "${candidate}-gcc" >/dev/null 2>&1; then
    CROSS_PREFIX="${candidate}"
    break
  fi
done
if [[ -z ${CROSS_PREFIX:-} ]]; then
  echo "[error] no suitable RISC-V cross compiler found on PATH" >&2
  exit 1
fi

CC="${CROSS_PREFIX}-gcc"

BUILD_DIR="${ROOT_DIR}/user/build/${PROGRAM}"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

mapfile -t SCAN_OUTPUT < <(python3 - <<'PY' "${PROGRAM_DIR}" "${LIB_MAP_PATH}"
import json
import os
import re
import sys
from pathlib import Path

program_dir = Path(sys.argv[1]).resolve()
lib_map_path = Path(sys.argv[2]).resolve()

if not program_dir.is_dir():
    print(f"[error] {program_dir} is not a directory", file=sys.stderr)
    sys.exit(1)

sources = sorted(p for p in program_dir.glob('*.c') if p.is_file())
if not sources:
    print(f"[error] no C sources found in {program_dir}", file=sys.stderr)
    sys.exit(1)

headers = sorted(p for p in program_dir.glob('*.h') if p.is_file())
files_to_scan = sources + headers

include_re = re.compile(r'^\s*#include\s*([<"])([^">]+)[">]')
angle_headers = set()
local_headers = set()

for path in files_to_scan:
    try:
        text = path.read_text()
    except OSError as exc:
        print(f"[error] failed to read {path}: {exc}", file=sys.stderr)
        sys.exit(1)
    for line in text.splitlines():
        m = include_re.match(line)
        if not m:
            continue
        delim, header = m.group(1), m.group(2).strip()
        if delim == '<':
            angle_headers.add(header)
        else:
            local_headers.add(header)

missing_locals = [h for h in sorted(local_headers) if not (program_dir / h).is_file()]
if missing_locals:
    for header in missing_locals:
        print(f"[error] missing local header: {header}", file=sys.stderr)
    sys.exit(1)

try:
    data = json.loads(lib_map_path.read_text())
except Exception as exc:
    print(f"[error] failed to parse {lib_map_path}: {exc}", file=sys.stderr)
    sys.exit(1)

libraries = data.get('libraries', [])
if not isinstance(libraries, list):
    print(f"[error] invalid library map format: 'libraries' should be a list", file=sys.stderr)
    sys.exit(1)

for lib in libraries:
    if not all(k in lib for k in ('name', 'script', 'artifact', 'headers')):
        print(f"[error] malformed library entry: {lib}", file=sys.stderr)
        sys.exit(1)

header_to_libs = {}
for lib in libraries:
    provided = lib.get('headers', [])
    if not isinstance(provided, list):
        print(f"[error] library headers must be a list: {lib}", file=sys.stderr)
        sys.exit(1)
    for header in provided:
        header_to_libs.setdefault(header, []).append(lib)

required_headers = sorted(angle_headers)
if required_headers:
    for header in required_headers:
        if header not in header_to_libs:
            print(f"[error] no library provides header: {header}", file=sys.stderr)
            sys.exit(1)

selected_libs = []
covered = set()
uncovered = set(required_headers)
available_libs = libraries[:]

while uncovered:
    best_lib = None
    best_covers = set()
    for lib in available_libs:
        covers = set(lib['headers']) & uncovered
        if len(covers) > len(best_covers):
            best_lib = lib
            best_covers = covers
    if best_lib is None:
        print(f"[error] could not resolve libraries for headers: {sorted(uncovered)}", file=sys.stderr)
        sys.exit(1)
    selected_libs.append(best_lib)
    available_libs.remove(best_lib)
    covered.update(best_lib['headers'])
    uncovered = set(required_headers) - covered

for header in sorted(local_headers):
    print(f"LOCAL:{header}")

for lib in selected_libs:
    print(f"LIB:{lib['name']}:{lib['script']}:{lib['artifact']}")

for src in sources:
    print(f"SRC:{src.name}")
PY
)

LOCAL_HEADERS=()
LIB_NAMES=()
LIB_SCRIPTS=()
LIB_ARTIFACTS=()
SRC_FILES=()
for line in "${SCAN_OUTPUT[@]}"; do
  case "${line}" in
    LOCAL:*)
      LOCAL_HEADERS+=("${line#LOCAL:}")
      ;;
    LIB:*)
      IFS=":" read -r _ name script artifact <<< "${line}"
      LIB_NAMES+=("${name}")
      LIB_SCRIPTS+=("${script}")
      LIB_ARTIFACTS+=("${artifact}")
      ;;
    SRC:*)
      SRC_FILES+=("${line#SRC:}")
      ;;
    *)
      if [[ -n "${line}" ]]; then
        echo "${line}" >&2
      fi
      ;;
  esac
done

if [[ ${#SRC_FILES[@]} -eq 0 ]]; then
  echo "[error] no source files discovered for ${PROGRAM}" >&2
  exit 1
fi

INCLUDE_DIRS=(
  "-I${ROOT_DIR}/kernel/include"
  "-I${PROGRAM_DIR}"
)

COMMON_FLAGS=(
  -std=gnu2x
  -Wall -Werror
  -Wno-error=pointer-sign
  -fno-builtin
  -nostdlib -nostdinc
  -march=rv64imac_zicsr
  -mabi=lp64
  -mcmodel=medany
  -O0 -g
  "${INCLUDE_DIRS[@]}"
)

OBJECTS=()
for src in "${SRC_FILES[@]}"; do
  SRC_PATH="${PROGRAM_DIR}/${src}"
  if [[ ! -f "${SRC_PATH}" ]]; then
    echo "[error] declared source not found: ${SRC_PATH}" >&2
    exit 1
  fi
  OBJ_PATH="${BUILD_DIR}/$(basename "${src}" .c).o"
  "${CC}" "${COMMON_FLAGS[@]}" -c "${SRC_PATH}" -o "${OBJ_PATH}"
  OBJECTS+=("${OBJ_PATH}")
  echo "built ${OBJ_PATH}" >&2
done

START_SRC="${USER_DIR}/start.s"
if [[ ! -f "${START_SRC}" ]]; then
  echo "[error] start.s missing at ${START_SRC}" >&2
  exit 1
fi
START_OBJ="${BUILD_DIR}/start.o"
"${CC}" "${COMMON_FLAGS[@]}" -c "${START_SRC}" -o "${START_OBJ}"
OBJECTS=("${START_OBJ}" "${OBJECTS[@]}")

declare -a LIB_PATHS=()
for idx in "${!LIB_NAMES[@]}"; do
  script_rel="${LIB_SCRIPTS[$idx]}"
  artifact_rel="${LIB_ARTIFACTS[$idx]}"

  if [[ -n "${script_rel}" ]]; then
    script="${USER_DIR}/${script_rel}"
    if [[ ! -x "${script}" ]]; then
      echo "[error] library build script missing or not executable: ${script}" >&2
      exit 1
    fi
    "${script}"
  fi

  if [[ -n "${artifact_rel}" ]]; then
    LIB_ABS_PATH="${ROOT_DIR}/${artifact_rel}"
    if [[ ! -f "${LIB_ABS_PATH}" ]]; then
      echo "[error] expected library artifact not found: ${LIB_ABS_PATH}" >&2
      exit 1
    fi
    if [[ ! " ${LIB_PATHS[*]} " =~ " ${LIB_ABS_PATH} " ]]; then
      LIB_PATHS+=("${LIB_ABS_PATH}")
    fi
  fi
done

LINK_FLAGS=(
  -nostdlib -nostartfiles
  -Wl,-T,"${USER_DIR}/linker.ld"
  -Wl,-Map,"${BUILD_DIR}/${PROGRAM}.map"
)

ELF_PATH="${BUILD_DIR}/${PROGRAM}.elf"
"${CC}" "${LINK_FLAGS[@]}" -o "${ELF_PATH}" "${OBJECTS[@]}" "${LIB_PATHS[@]}"
echo "${PROGRAM}.elf written to ${ELF_PATH}" >&2

LOAD_DIR="${ROOT_DIR}/load"
mkdir -p "${LOAD_DIR}"
cp "${ELF_PATH}" "${LOAD_DIR}/${PROGRAM}.elf"
echo "copied ${ELF_PATH} to ${LOAD_DIR}/${PROGRAM}.elf" >&2