#!/bin/bash
# Runs the experimental Linux GameD build against a headless Weston compositor and checks its
# log for evidence that SDL3's Wayland backend created a window, bgfx initialized a renderer,
# and RmlUi's context came up over that window. See docs/BUILDING.md's "Experimental native
# Linux build" section for what this proves and what it does not: the compositor's weston-test
# input-injection protocol is not packaged for Ubuntu, so this does not inject synthetic input,
# and it needs no game data, so it never reaches a game-data UI screen.
#
# Usage: run.sh <path to the bin directory containing GameD>
set -euo pipefail

if [ $# -ne 1 ]; then
	echo "usage: run.sh <path to the bin directory containing GameD>" >&2
	exit 1
fi

bin_dir="$(cd "$1" && pwd)"
game_bin="${bin_dir}/GameD"

if [ ! -x "${game_bin}" ]; then
	echo "wayland-smoke: ${game_bin} is missing or not executable" >&2
	exit 1
fi

if ! command -v weston >/dev/null 2>&1; then
	echo "wayland-smoke: weston is not on PATH" >&2
	exit 1
fi

work_dir="$(mktemp -d)"
weston_pid=""
game_pid=""

cleanup() {
	[ -n "${game_pid}" ] && kill -9 "${game_pid}" >/dev/null 2>&1 || true
	[ -n "${weston_pid}" ] && kill -9 "${weston_pid}" >/dev/null 2>&1 || true
	rm -f "${bin_dir}/OPENTS.INI"
	rm -rf "${work_dir}"
}
trap cleanup EXIT

export XDG_RUNTIME_DIR="${work_dir}/xdg"
mkdir -p "${XDG_RUNTIME_DIR}"
chmod 700 "${XDG_RUNTIME_DIR}"

weston_socket="wayland-smoke-$$"
weston_log="${work_dir}/weston.log"
game_log="${work_dir}/game.log"

weston --backend=headless-backend.so --socket="${weston_socket}" --width=640 --height=480 \
	>"${weston_log}" 2>&1 &
weston_pid=$!

# Weston needs a moment to create its socket before a client can connect to it.
sleep 2

# UIRmlFileClass opens "Arimo.ttf" through CDFileClass's own search-folder machinery, which
# checks the lowercased default folders (INI/, MIX/, Maps/) but not ui/; SearchPaths= adds it.
printf '[Paths]\nSearchPaths=ui\n' >"${bin_dir}/OPENTS.INI"

(
	cd "${bin_dir}"
	WAYLAND_DISPLAY="${weston_socket}" SDL_VIDEODRIVER=wayland "${game_bin}"
) >"${game_log}" 2>&1 &
game_pid=$!

# The game's render loop does not check for SIGTERM during normal operation, so a bounded wait
# and a forced kill are the only way to end this run deterministically.
sleep 6
kill -9 "${game_pid}" >/dev/null 2>&1 || true
kill -9 "${weston_pid}" >/dev/null 2>&1 || true
wait "${game_pid}" 2>/dev/null || true
wait "${weston_pid}" 2>/dev/null || true
game_pid=""
weston_pid=""

echo "=== weston log ==="
cat "${weston_log}"
echo "=== game log ==="
cat "${game_log}"

failures=0

check_present() {
	if ! grep -qF "$1" "${game_log}"; then
		echo "wayland-smoke: missing expected log line: $1" >&2
		failures=$((failures + 1))
	fi
}

check_absent() {
	if grep -qF "$1" "${game_log}"; then
		echo "wayland-smoke: found unexpected log line matching: $1" >&2
		failures=$((failures + 1))
	fi
}

# The renderer name varies with the software driver a runner has installed, so only the
# message's fixed prefix is checked; see code/video.cpp.
check_present "Video: renderer is"
check_present "BGFX Init complete."
check_present "UI: RmlUi"
check_present "ready over a 640x480 frame"
check_absent "UI error:"

if [ "${failures}" -ne 0 ]; then
	echo "wayland-smoke: ${failures} check(s) failed" >&2
	exit 1
fi

echo "wayland-smoke: all checks passed"
