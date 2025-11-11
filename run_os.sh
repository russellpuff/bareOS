#!/bin/sh
# Rebuild bareOS and launch QEMU using the generated loader flags.
# Spinner shows only in --silent mode. It starts before the first scons call and
# is erased immediately before the final exec to QEMU.
set -eu

usage() {
	cat <<'USAGE'
Usage: run_os.sh [--debug] [--silent] [--help] [--with <target ...>]

Options:
	--debug    Build with BAREOS_QEMU_DEBUG=1 so QEMU starts with a GDB stub.
	--silent   Suppress build output from scons and show a tiny spinner.
	--with     Treat all the following arguments as "scons build <arg>" targets.
	--help     Show this help and exit.
USAGE
}

DEBUG_MODE=0
SILENT_MODE=0
WITH_TARGETS=""
LOG_FILE=""
LOG_TMP=""

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
	clear
}

# Append messages to the build log if available.
log_append() {
	[ -z "$1" ] && return 0
	if [ -n "$LOG_TMP" ]; then
		 printf '%s\n' "$1" >>"$LOG_TMP"
	elif [ -n "$LOG_FILE" ]; then
		 printf '%s\n' "$1" >>"$LOG_FILE"
	fi
}

# Ensure spinner is stopped on any exit path and finalize logs safely.
cleanup() {
	stop_spinner
	if [ -n "$LOG_TMP" ] && [ -f "$LOG_TMP" ]; then
		mv -f "$LOG_TMP" "$LOG_FILE"
		LOG_TMP=""
	fi
}

# scons wrapper that respects --silent.
run_scons() {
	if [ "${SILENT_MODE}" -eq 1 ]; then
		scons "$@" >>"$LOG_TMP" 2>&1
		return $?
	fi

	pipe_tmp=$(mktemp "${TMPDIR:-/tmp}/run_os.pipe.XXXXXX")
	rm -f "$pipe_tmp"
	if ! mkfifo "$pipe_tmp"; then
		printf 'Failed to create logging pipe at %s\n' "$pipe_tmp" >&2
		return 1
	fi

	tee -a "$LOG_TMP" <"$pipe_tmp" &
	tee_pid=$!

	if scons "$@" >"$pipe_tmp" 2>&1; then
		status=0
	else
		status=$?
	fi

	wait "$tee_pid" 2>/dev/null || true
	rm -f "$pipe_tmp"

	return $status
}

# Print a fatal build error, mirror it to the log, and exit with the supplied status.
fatal() {
	status=$1
	shift
	message=$*
	[ -n "$message" ] || message="Build failed"
	log_append "$message"
	cleanup
	printf '%s\n' "$message" >&2
	exit "$status"
}

# Record a non-fatal optional build failure while allowing subsequent targets to proceed.
report_optional_failure() {
	target=$1
	msg="Failed to build optional target: $target"
	log_append "$msg"
	if [ "${SILENT_MODE}" -ne 1 ]; then
			printf '%s\n' "$msg" >&2
	fi
}

trap 'cleanup' EXIT
trap 'handle_signal INT' INT
trap 'handle_signal TERM' TERM

# Parse args.
while [ "$#" -gt 0 ]; do
	case "$1" in
		--debug|-d)  DEBUG_MODE=1 ; shift ;;
		--silent|-s) SILENT_MODE=1 ; shift ;;
		--help|-h)   usage ; exit 0 ;;
		--with)
			shift
			WITH_TARGETS="$*"
			break
			;;
		*) echo "Unknown option: $1" >&2 ; usage ; exit 2 ;;
	esac
done

# Project root assumed to be the directory containing this script.
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

LOG_FILE="$ROOT_DIR/build.log"
# Always start from a clean log.
rm -f "$LOG_FILE"
LOG_TMP=$(mktemp "$ROOT_DIR/build.log.XXXXXX")

# Start spinner before the first run_scons call if in silent mode.
start_spinner

# Start with userland stuff
cd user
if ! run_scons -c; then # Mandatory clean
	fatal 1 "Failed to clean user build artifacts"
fi

if ! run_scons build shell; then # Mandatory shell
	fatal 1 "Failed to build shell target"
fi

if [ -n "$WITH_TARGETS" ]; then # Optional build targets, skipped if fail
	for target in $WITH_TARGETS; do
		if ! run_scons build "$target"; then
			report_optional_failure "$target"
		fi
	done
fi

cd "$ROOT_DIR"

# Full build. DEBUG toggles QEMU flag generation via env.
if [ "${DEBUG_MODE}" -eq 1 ]; then
	if ! BAREOS_QEMU_DEBUG=1 run_scons build; then
		fatal 1 "Failed to build kernel (debug mode)"
	fi
else
	if ! run_scons build; then
		fatal 1 "Failed to build kernel"
	fi
fi

FLAGS_FILE="$ROOT_DIR/.build/qemu_flags.txt"
if [ ! -f "$FLAGS_FILE" ]; then
	fatal 1 "Failed to locate generated QEMU flags at $FLAGS_FILE"
fi

FLAGS=$(cat "$FLAGS_FILE")

KERNEL_IMAGE="$ROOT_DIR/bareOS.elf"
if [ ! -f "$KERNEL_IMAGE" ]; then
	fatal 1 "Missing kernel image: $KERNEL_IMAGE"
fi

# Stop spinner and finalize logs before handing control to QEMU.
stop_spinner
cleanup

trap - EXIT INT TERM

# Launch QEMU with the generated flags.
set -- $FLAGS
exec qemu-system-riscv64 -kernel "$KERNEL_IMAGE" "$@"