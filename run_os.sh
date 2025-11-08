#!/bin/sh
# Helper script to rebuild bareOS and launch QEMU using the generated loader flags.
set -eu

usage() {
	cat <<'USAGE'
Usage: run_qemu.sh [--debug]

Options:
	--debug    Generate QEMU flags for GDB debugging and launch QEMU with the debug stub enabled.
USAGE
}

DEBUG_MODE=0

while [ "$#" -gt 0 ]; do
	case "$1" in
	--debug|-d)
		DEBUG_MODE=1
		;;
	--help|-h)
		usage
		exit 0
		;;
	*)
		echo "Unknown option: $1" >&2
		usage >&2
		exit 1
		;;
	esac
	shift
done

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

cd "$ROOT_DIR"

scons -c

cd user
scons build shell
cd "$ROOT_DIR"

scons build

if [ "$DEBUG_MODE" -eq 1 ]; then
	BAREOS_QEMU_DEBUG=1 scons qemu-flags
else
	scons qemu-flags
fi

FLAGS_FILE="$ROOT_DIR/.build/qemu_flags.txt"
if [ ! -f "$FLAGS_FILE" ]; then
	echo "Failed to locate generated QEMU flags at $FLAGS_FILE" >&2
	exit 1
fi

FLAGS=$(cat "$FLAGS_FILE")

KERNEL_IMAGE="$ROOT_DIR/bareOS.elf"
if [ ! -f "$KERNEL_IMAGE" ]; then
	echo "Missing kernel image: $KERNEL_IMAGE" >&2
	exit 1
fi

set -- $FLAGS
exec qemu-system-riscv64 -kernel "$KERNEL_IMAGE" "$@"
