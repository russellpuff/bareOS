#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCHES=(
  "riscv64-unknown-elf"
  "riscv64-unknown-linux-gnu"
  "riscv64-linux-gnu"
)

if [[ ${#ARCHES[@]} -eq 0 ]]; then
  echo "[error] internal arch list is empty" >&2
  exit 1
fi

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

BUILD_DIR="${ROOT_DIR}/user/build/io"
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

SRCS=(
  "kernel/lib/io.c"
  "kernel/lib/printf.c"
  "kernel/lib/string.c"
  "kernel/lib/ecall.c"
)

OBJECTS=()
for src in "${SRCS[@]}"; do
  base="$(basename "${src}")"
  obj="${BUILD_DIR}/${base%.c}.o"
  "${CC}" "${CFLAGS[@]}" -c "${ROOT_DIR}/${src}" -o "${obj}"
  OBJECTS+=("${obj}")
  echo "built ${obj}" >&2
done

LIB_PATH="${BUILD_DIR}/libio.a"
"${AR}" rcs "${LIB_PATH}" "${OBJECTS[@]}"
echo "archive created at ${LIB_PATH}" >&2