#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIB_DIR="${ROOT_DIR}/testing/build/uprintf"
LIB_PATH="${LIB_DIR}/libuprintf.a"

if [[ ! -f "${LIB_PATH}" ]]; then
  echo "[error] ${LIB_PATH} not found. Run build_uprintf_lib.sh first." >&2
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

BUILD_DIR="${ROOT_DIR}/testing/build/sample"
mkdir -p "${BUILD_DIR}"

COMMON_FLAGS=(
  -std=gnu2x
  -Wall -Werror
  -fno-builtin
  -nostdlib -nostdinc
  -march=rv64imac_zicsr
  -mabi=lp64
  -mcmodel=medany
  -O0 -g
  -I"${ROOT_DIR}/kernel/include"
)

"${CC}" "${COMMON_FLAGS[@]}" -c "${ROOT_DIR}/testing/sample.c" -o "${BUILD_DIR}/sample.o"
"${CC}" "${COMMON_FLAGS[@]}" -c "${ROOT_DIR}/testing/start.s" -o "${BUILD_DIR}/start.o"

LINK_FLAGS=(
  -nostdlib -nostartfiles
  -Wl,-T,"${ROOT_DIR}/testing/linker.ld"
  -Wl,-Map,"${BUILD_DIR}/sample.map"
)

"${CC}" "${LINK_FLAGS[@]}" -o "${BUILD_DIR}/sample.elf" \
  "${BUILD_DIR}/start.o" \
  "${BUILD_DIR}/sample.o" \
  "${LIB_PATH}"

echo "sample.elf written to ${BUILD_DIR}/sample.elf" >&2