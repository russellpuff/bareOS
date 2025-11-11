#!/bin/sh
# Rebuild bareOS and launch QEMU using the generated loader flags.
# Spinner shows only in --silent mode. It starts before the first scons call and
# is erased immediately before the final exec to QEMU.
set -eu

usage() {
	cat <<'USAGE'
Usage: run_os.sh [--debug] [--silent] [--help]

Options:
	--debug    Build with BAREOS_QEMU_DEBUG=1 so QEMU starts with a GDB stub.
	--silent   Suppress build output from scons and show a tiny spinner.
	--help     Show this help and exit.
USAGE
}

DEBUG_MODE=0
SILENT_MODE=0

# Minimal spinner. Runs in background until stop_spinner is called.
start_spinner() {
	[ "${SILENT_MODE}" -eq 1 ] || return 0
	# Avoid multiple spinners.
	[ -n "${SPINNER_PID-}" ] && kill "${SPINNER_PID}" 2>/dev/null || true
	(
		while :; do
			for c in '|' '/' '-' '\'; do
				# shellcheck disable=SC2059
				printf "\r%s" "$c"
				sleep 0.1
			done
		done
	) & SPINNER_PID=$!
}

stop_spinner() {
	[ -n "${SPINNER_PID-}" ] || return 0
	kill "${SPINNER_PID}" 2>/dev/null || true
	wait "${SPINNER_PID}" 2>/dev/null || true
	unset SPINNER_PID
	# Clear the spinner character from the line.
	printf "\r \r"
}

# Ensure spinner is stopped on any exit path.
trap 'stop_spinner' EXIT INT TERM

# scons wrapper that respects --silent.
run_scons() {
	if [ "${SILENT_MODE}" -eq 1 ]; then
		scons "$@" >/dev/null
	else
		scons "$@"
	fi
}

# Parse args.
while [ "$#" -gt 0 ]; do
	case "$1" in
		--debug|-d)  DEBUG_MODE=1 ; shift ;;
		--silent|-s) SILENT_MODE=1 ; shift ;;
		--help|-h)   usage ; exit 0 ;;
		*) echo "Unknown option: $1" >&2 ; usage ; exit 2 ;;
	esac
done

# Project root assumed to be the directory containing this script.
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

# Start spinner before the first run_scons call if in silent mode.
start_spinner

# Full build. DEBUG toggles QEMU flag generation via env.
if [ "${DEBUG_MODE}" -eq 1 ]; then
	BAREOS_QEMU_DEBUG=1 run_scons build
else
	run_scons build
fi

FLAGS_FILE="$ROOT_DIR/.build/qemu_flags.txt"
if [ ! -f "$FLAGS_FILE" ]; then
	stop_spinner
	echo "Failed to locate generated QEMU flags at $FLAGS_FILE" >&2
	exit 1
fi

FLAGS=$(cat "$FLAGS_FILE")

KERNEL_IMAGE="$ROOT_DIR/bareOS.elf"
if [ ! -f "$KERNEL_IMAGE" ]; then
	stop_spinner
	echo "Missing kernel image: $KERNEL_IMAGE" >&2
	exit 1
fi

# Stop and erase spinner before handing control to QEMU.
stop_spinner

# Launch QEMU with the generated flags.
set -- $FLAGS
exec qemu-system-riscv64 -kernel "$KERNEL_IMAGE" "$@"
