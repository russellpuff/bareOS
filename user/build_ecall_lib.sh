#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
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
AR="${CROSS_PREFIX}-ar"

BUILD_DIR="${ROOT_DIR}/user/build/ecall"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

CFLAGS=(
  -std=gnu2x
  -Wall -Werror
  -Wno-error=pointer-sign
  -fno-builtin
  -nostdlib -nostdinc
  -march=rv64imac_zicsr
  -mabi=lp64
  -mcmodel=medany
  -O0 -g
  -I"${ROOT_DIR}/kernel/include"
)

SRC="kernel/lib/ecall.c"
OBJ="${BUILD_DIR}/ecall.o"
"${CC}" "${CFLAGS[@]}" -c "${ROOT_DIR}/${SRC}" -o "${OBJ}"
echo "built ${OBJ}" >&2

LIB_PATH="${BUILD_DIR}/libecall.a"
"${AR}" rcs "${LIB_PATH}" "${OBJ}"
echo "archive created at ${LIB_PATH}" >&2