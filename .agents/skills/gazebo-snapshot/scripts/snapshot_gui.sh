#!/usr/bin/env bash
# Capture the Gazebo Sim GUI window to a PNG.
#
# Usage: snapshot_gui.sh [OUT_PNG]
#   OUT_PNG defaults to gui_snapshot.png in the current directory.
#
# Works when the Gazebo GUI is rendered on an X server reachable from where
# this script runs. Two typical setups:
#   - "headfull in Docker": the Gazebo container runs Xvfb and the GUI
#     (DISPLAY=:99 set in-container). Run this script inside the container.
#   - "headfull on host": the GUI is X11-forwarded to the host (DISPLAY=:0).
#     Run this script on the host.
#
# Requires (in the place where the script runs): xdotool and imagemagick
# (import). Install via:
#   apt-get install -y xdotool imagemagick
#
# Finds the visible "Gazebo Sim" window (exact title match, largest one) and
# captures it with ImageMagick's `import`. Retries a few times because X may
# return "Resource temporarily unavailable" while the window is still being
# mapped. Falls back to the X root window if no titled window is found.

set -uo pipefail

OUT="${1:-gui_snapshot.png}"
export DISPLAY="${DISPLAY:-:0}"

# Collect candidate window ids, then pick the largest visible one (>=50x50)
# whose title is exactly "Gazebo Sim".
pick_wid() {
  local id wd ht
  while read -r id; do
    # `--shell` prints "WIDTH=1000" / "HEIGHT=845" lines (more robust than
    # parsing the human-readable `Geometry:` field).
    wd="$(xdotool getwindowgeometry --shell "$id" 2>/dev/null | awk -F= '$1=="WIDTH"{print $2}')"
    ht="$(xdotool getwindowgeometry --shell "$id" 2>/dev/null | awk -F= '$1=="HEIGHT"{print $2}')"
    [[ -n "${wd:-}" && -n "${ht:-}" ]] || continue
    [[ "$wd" -gt 50 && "$ht" -gt 50 ]] 2>/dev/null || continue
    printf '%s\t%s\n' "$id" "$((wd * ht))"
  done < <(xdotool search --name "^Gazebo Sim$" 2>/dev/null)
}

WID="$(pick_wid | sort -t$'\t' -k2 -n | tail -1 | cut -f1)"

for attempt in 1 2 3 4 5; do
  if [[ -n "${WID:-}" ]]; then
    if import -window "$WID" "$OUT" 2>/dev/null; then
      echo "saved $OUT (Gazebo window $WID)" >&2
      exit 0
    fi
  elif import -window root "$OUT" 2>/dev/null; then
    echo "saved $OUT (root window; no titled Gazebo window found)" >&2
    exit 0
  fi
  sleep 2
done
echo "failed to capture the Gazebo GUI on $DISPLAY after retries" >&2
exit 1
