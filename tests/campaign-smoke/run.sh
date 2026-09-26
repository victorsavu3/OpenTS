#!/bin/bash
# Starts a real single-player campaign mission against the caller's own game data, once for
# GDI and once for Nod, and checks that mouse input reaches the tactical view: it selects
# whatever the mission's Home waypoint centers the camera on and gives it a move order. It
# takes a screenshot at each step for a person to look at afterward; it draws no automated
# conclusion from the pixels themselves, since it has no rendered reference to compare them to.
#
# This needs the caller's own legally obtained Tiberian Sun data (RULES.INI, BATTLE.INI, the
# mission files) and is never run by CI or CTest; CONTRIBUTING.md's rule against proprietary
# assets in automated tests does not apply to it for exactly that reason. Build the
# OPENTS_EXPERIMENTAL_LINUX target first (see docs/BUILDING.md) and run this by hand.
#
# It drives the game the same way CnCNet's launcher does: -SPAWN plus a SPAWN.INI naming the
# mission directly (code/spawner.cpp's Spawner_Setup_Campaign), which reaches the mission
# through the same Read_Scenario_INI path a menu-driven launch does, without needing either of
# the two main-menu implementations (code/newmenu.cpp's graphical menu, or the RmlUi fallback
# list in code/init.cpp's Main_Menu) to be automated.
#
# The default GDI/Nod mission filenames are Tiberian Sun's own conventional first-mission
# names; pass --gdi-scenario/--nod-scenario if the caller's data uses different ones (a mod, a
# different release, or a non-default install layout).
#
# The select/move screen coordinates are a guess: Read_Scenario_INI centers the camera on the
# mission's Home waypoint at start (code/scenario.cpp's Fill_In_Data), which by convention is
# near the player's starting units, but the sidebar's width and the exact unit layout are the
# mission's own content and not something this script can know in advance. Look at the
# "02-mission-loaded" screenshot after a first run and pass --select-x/--select-y and
# --move-x/--move-y to correct them, if the default guess misses.
set -euo pipefail

bin_dir=""
assets=""
userdir=""
out_dir=""
gdi_scenario="SCG01EA.INI"
nod_scenario="SCB01EA.INI"
width=1024
height=768
select_x=""
select_y=""
move_x=""
move_y=""
display_number=97

usage() {
	cat <<'EOF'
usage: run.sh --bin-dir <dir> --assets <dir> [options]

Required:
  --bin-dir DIR       Directory containing GameD (e.g. build/linux/bin)
  --assets DIR        Path to the caller's own Tiberian Sun data (-DATADIR=)

Options:
  --userdir DIR        Where saves/logs go (default: a scratch directory this
                        script creates and removes)
  --out DIR             Where screenshots are written (default: ./campaign-smoke-out)
  --gdi-scenario NAME   GDI mission file name (default: SCG01EA.INI)
  --nod-scenario NAME   Nod mission file name (default: SCB01EA.INI)
  --width N             Virtual display width (default: 1024). The game picks its own
                        window size independently of this; it only has to fit.
  --height N            Virtual display height (default: 768)
  --select-x N          Screen X to left-click to select a unit (default: left of the real
                        window's own center, allowing for a right-hand sidebar)
  --select-y N          Screen Y to left-click to select a unit (default: the real window's
                        own vertical center)
  --move-x N            Screen X to right-click to issue the move order (default: select-x
                        plus a tenth of the real window's width)
  --move-y N            Screen Y to right-click to issue the move order (default: select-y)
  --display N           X display number to use (default: 97)
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		--bin-dir) bin_dir="$2"; shift 2 ;;
		--assets) assets="$2"; shift 2 ;;
		--userdir) userdir="$2"; shift 2 ;;
		--out) out_dir="$2"; shift 2 ;;
		--gdi-scenario) gdi_scenario="$2"; shift 2 ;;
		--nod-scenario) nod_scenario="$2"; shift 2 ;;
		--width) width="$2"; shift 2 ;;
		--height) height="$2"; shift 2 ;;
		--select-x) select_x="$2"; shift 2 ;;
		--select-y) select_y="$2"; shift 2 ;;
		--move-x) move_x="$2"; shift 2 ;;
		--move-y) move_y="$2"; shift 2 ;;
		--display) display_number="$2"; shift 2 ;;
		-h|--help) usage; exit 0 ;;
		*) echo "run.sh: unknown argument: $1" >&2; usage >&2; exit 1 ;;
	esac
done

if [ -z "${bin_dir}" ] || [ -z "${assets}" ]; then
	echo "run.sh: --bin-dir and --assets are required" >&2
	usage >&2
	exit 1
fi

bin_dir="$(cd "${bin_dir}" && pwd)"
assets="$(cd "${assets}" && pwd)"
game_bin="${bin_dir}/GameD"

if [ ! -x "${game_bin}" ]; then
	echo "run.sh: ${game_bin} is missing or not executable" >&2
	exit 1
fi

for tool in Xvfb xdotool import; do
	if ! command -v "${tool}" >/dev/null 2>&1; then
		echo "run.sh: ${tool} is not on PATH (Xvfb, xdotool, ImageMagick's import)" >&2
		exit 1
	fi
done

# The game picks its own window size (its options default, not this script's --width/--height,
# which only sizes the virtual display), so the select/move coordinates are resolved against the
# real window geometry once it exists (see resolve_click_points), not against --width/--height.
explicit_select_x="${select_x}"
explicit_select_y="${select_y}"
explicit_move_x="${move_x}"
explicit_move_y="${move_y}"

if [ -z "${out_dir}" ]; then out_dir="$(pwd)/campaign-smoke-out"; fi
mkdir -p "${out_dir}"

work_dir="$(mktemp -d)"
if [ -z "${userdir}" ]; then
	userdir="${work_dir}/user"
	mkdir -p "${userdir}"
fi

xvfb_pid=""
game_pid=""

cleanup() {
	[ -n "${game_pid}" ] && kill -9 "${game_pid}" >/dev/null 2>&1 || true
	[ -n "${xvfb_pid}" ] && kill -9 "${xvfb_pid}" >/dev/null 2>&1 || true
	rm -f "${bin_dir}/OPENTS.INI" "${bin_dir}/SPAWN.INI"
	rm -rf "${work_dir}"
}
trap cleanup EXIT

export DISPLAY=":${display_number}"
Xvfb "${DISPLAY}" -screen 0 "${width}x${height}x24" >"${work_dir}/xvfb.log" 2>&1 &
xvfb_pid=$!
sleep 1

screenshot() {
	local name="$1"
	import -window root "${out_dir}/${name}.png" 2>/dev/null || true
}

# Sends the keys that skip a briefing or intro movie, and dismiss a "press any key" prompt,
# without knowing which (if either) is on screen at the time.
nudge_past_movies() {
	xdotool key --window "$1" Escape >/dev/null 2>&1 || true
	xdotool key --window "$1" Return >/dev/null 2>&1 || true
}

# SDL3's X11 backend only accepts a button event from a non-master pointer device once the
# window already has both mouse and keyboard focus at the SDL level (SDL_x11xinput2.c), which a
# combined "click" sent straight after a plain mousemove does not reliably give it time to reach;
# xdotool's synthetic clicks are exactly such a non-master-device event. Waiting for the move to
# finish (--sync) before pressing, and holding the button down for a moment before releasing it,
# avoids the race.
click_at() {
	local window_id="$1" x="$2" y="$3" button="$4"
	xdotool mousemove --sync --window "${window_id}" "${x}" "${y}"
	xdotool mousedown "${button}"
	sleep 0.3
	xdotool mouseup "${button}"
}

# A message box the game itself pops up (a missing- or bad-data warning, for instance) is also
# titled "Tiberian Sun" and can outlive its own window ID by the time a caller acts on it, so
# matching by name alone is not reliable. The game picks its own window size (its options
# default, not this script's --width/--height), so the main view can only be told apart from a
# dialog by being the larger of the two, not by matching a size decided in advance.
find_main_window() {
	local candidate geometry w h area best="" best_area=0
	for candidate in $(xdotool search --name "Tiberian Sun" 2>/dev/null || true); do
		geometry="$(xdotool getwindowgeometry --shell "${candidate}" 2>/dev/null || true)"
		w="$(printf '%s\n' "${geometry}" | sed -n 's/^WIDTH=//p')"
		h="$(printf '%s\n' "${geometry}" | sed -n 's/^HEIGHT=//p')"
		if [ -z "${w}" ] || [ -z "${h}" ]; then
			continue
		fi
		area=$((w * h))
		if [ "${area}" -gt "${best_area}" ]; then
			best="${candidate}"
			best_area="${area}"
		fi
	done
	if [ -n "${best}" ]; then
		echo "${best}"
		return 0
	fi
	return 1
}

# Fills in any select/move coordinate the caller did not set explicitly, from the real window's
# own geometry rather than --width/--height, and allowing for a right-hand sidebar the same way
# as the original width-based guess.
resolve_click_points() {
	local window_id="$1" geometry real_width real_height
	geometry="$(xdotool getwindowgeometry --shell "${window_id}" 2>/dev/null || true)"
	real_width="$(printf '%s\n' "${geometry}" | sed -n 's/^WIDTH=//p')"
	real_height="$(printf '%s\n' "${geometry}" | sed -n 's/^HEIGHT=//p')"
	real_width="${real_width:-${width}}"
	real_height="${real_height:-${height}}"

	select_x="${explicit_select_x:-$((real_width / 2 - real_width / 8))}"
	select_y="${explicit_select_y:-$((real_height / 2))}"
	move_x="${explicit_move_x:-$((select_x + real_width / 10))}"
	move_y="${explicit_move_y:-${select_y}}"
}

wait_for_window() {
	local timeout="$1" waited=0 id=""
	while [ "${waited}" -lt "${timeout}" ]; do
		if id="$(find_main_window)"; then
			echo "${id}"
			return 0
		fi
		sleep 1
		waited=$((waited + 1))
	done
	return 1
}

# Runs one mission end to end: writes SPAWN.INI, launches the game, nudges it past any movies,
# waits for the scenario to load, gives an order, and screenshots each step.
run_side() {
	local side_name="$1"
	local scenario="$2"
	local log="${work_dir}/${side_name}.log"

	cd "${bin_dir}"
	printf '[Paths]\nSearchPaths=ui\n' >OPENTS.INI
	cat >SPAWN.INI <<-EOF
	[Settings]
	Scenario=${scenario}
	CampaignID=-1
	GameSpeed=1
	Firestorm=False
	IsSinglePlayer=Yes
	SidebarHack=True
	Side=0
	BuildOffAlly=False
	DifficultyModeHuman=0
	DifficultyModeComputer=2
	EOF

	SDL_VIDEODRIVER=x11 "${game_bin}" -DATADIR="${assets}" -USERDIR="${userdir}" -SPAWN \
		>"${log}" 2>&1 &
	game_pid=$!

	local window_id
	if ! window_id="$(wait_for_window 20)"; then
		echo "campaign-smoke (${side_name}): the game window never appeared" >&2
		echo "=== ${side_name} log ==="
		cat "${log}"
		return 1
	fi

	screenshot "${side_name}-01-window-created"

	# The scenario is read once, near the start of the load; movies (if any) can play before
	# and after that, so this nudges throughout the whole wait rather than only around it.
	local waited=0
	while [ "${waited}" -lt 45 ]; do
		nudge_past_movies "${window_id}"
		if grep -q "Reading scenario:" "${log}" 2>/dev/null; then
			break
		fi
		sleep 1
		waited=$((waited + 1))
	done

	if ! grep -q "Reading scenario:" "${log}" 2>/dev/null; then
		echo "campaign-smoke (${side_name}): the scenario never started loading within 45s" >&2
		echo "=== ${side_name} log ==="
		cat "${log}"
		return 1
	fi

	# The scenario has started loading, not finished; give the rest of Read_Scenario_INI and
	# the first rendered frame time to land, still nudging in case a briefing movie follows.
	local settle=0
	while [ "${settle}" -lt 8 ]; do
		nudge_past_movies "${window_id}"
		sleep 1
		settle=$((settle + 1))
	done

	screenshot "${side_name}-02-mission-loaded"

	resolve_click_points "${window_id}"
	echo "campaign-smoke (${side_name}): selecting at ${select_x},${select_y}, moving to ${move_x},${move_y}"

	click_at "${window_id}" "${select_x}" "${select_y}" 1
	sleep 1
	screenshot "${side_name}-03-after-select"

	click_at "${window_id}" "${move_x}" "${move_y}" 3
	sleep 2
	screenshot "${side_name}-04-after-move-order"

	kill -9 "${game_pid}" >/dev/null 2>&1 || true
	wait "${game_pid}" 2>/dev/null || true
	game_pid=""
	rm -f OPENTS.INI SPAWN.INI

	echo "campaign-smoke (${side_name}): screenshots in ${out_dir}, log kept at ${log}"
	cp "${log}" "${out_dir}/${side_name}.log"
}

run_side "gdi" "${gdi_scenario}"
run_side "nod" "${nod_scenario}"

echo "campaign-smoke: done. Inspect ${out_dir} for the screenshots from both sides."
