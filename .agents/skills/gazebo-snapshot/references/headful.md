# Gazebo Headful GUI Snapshots — Details

"Headful" means the Gazebo GUI (the 3D viewport window titled **Gazebo Sim**)
is actually rendered. A screenshot is a capture of that window, taken by an X
client. Two ways to give the GUI a display in Docker:

1. **Xvfb inside the container** (no real monitor needed) — the project's own
   `docker/test.sh` already starts `Xvfb :99`.
2. **X11 forwarding from the host** — the host's X server hosts the window
   (the repo's `docker/service_gazebo.yml` wires up `/tmp/.X11-unix` and
   `~/.Xauthority` for exactly this).

The screenshot approach is the same for both; only `DISPLAY` and where the
capture script runs differ.

## Launching the GUI (headful)

Without `-s`, `gz sim <world>.sdf` launches the full GUI on `$DISPLAY`.

### Headful via Xvfb in the container

```bash
docker exec <container> bash -c '
  # start a virtual X server if one is not already up
  pgrep -x Xvfb >/dev/null || /usr/bin/Xvfb :99 -screen 0 1280x1024x24 &
  sleep 3
  cd <dir>
  DISPLAY=:99 nohup gz sim <world>.sdf > /snap/gui.log 2>&1 &
  echo started'
sleep 12
```

Confirm the window exists:

```bash
docker exec <container> bash -c '
  DISPLAY=:99 xdotool search --name "^Gazebo Sim$" | head'
```

### Headful via host X11 forwarding

Run the container with the X socket shared (as `docker/service_gazebo.yml`
does). On the host, export the right `DISPLAY` and run `gz sim <world>.sdf`
inside the container; the window appears on the host. The screenshot script
then runs on the **host** with the host's `DISPLAY` (e.g. `:0`).

## Capturing the window

Use the bundled `scripts/snapshot_gui.sh`. It:
- finds the visible window whose title is exactly `Gazebo Sim` (largest match
  ≥ 50x50),
- captures it with ImageMagick `import -window <id>`,
- retries (X can return "Resource temporarily unavailable" while the window is
  still being mapped),
- falls back to the X root window if no titled window is found.

```bash
# headful in the container (Xvfb :99)
docker exec <container> bash -c '
  apt-get install -y xdotool imagemagick >/dev/null 2>&1 || true
  DISPLAY=:99 bash /snap/snapshot_gui.sh /snap/gui.png'
docker cp <container>:/snap/gui.png ./gui.png

# headful on the host (X11 forwarding)
DISPLAY=:0 bash scripts/snapshot_gui.sh gui.png
```

### Requirements

- `xdotool` and `imagemagick` (`import`) must be installed in the place the
  capture script runs.
- The capture must run on the **same X server** as the GUI:
  - Xvfb-in-container: run the script inside the container with `DISPLAY=:99`.
  - Host X11 forwarding: run the script on the host with the host `DISPLAY`.

### Alternative capture methods (both verified working)

If `import` is unavailable, `xwd` works:

```bash
DISPLAY=:99 xwd -id "$(xdotool search --name '^Gazebo Sim$' | head -1)" -o /tmp/w.xwd
DISPLAY=:99 convert /tmp/w.xwd /snap/gui.png
```

or grab the whole X screen:

```bash
DISPLAY=:99 import -window root /snap/gui.png
```

## Window-title / geometry pitfalls

- The Gazebo window title is exactly `Gazebo Sim`. Use an anchored regex
  (`"^Gazebo Sim$"`) to avoid matching other windows that merely contain the
  substring.
- Prefer `xdotool getwindowgeometry --shell` (emits `WIDTH=..` / `HEIGHT=..`)
  over parsing the human-readable `Geometry:` field; it is more robust and
  avoids the classic bug of falling back to the root window.
- If multiple Gazebo windows exist, capture the largest (the main viewport).

## Verifying a headful capture is valid

- The PNG size should match the window geometry (e.g. ~1000x845 for the
  default 1280x1024 Xvfb screen with the default window).
- The image should contain the 3D scene, not a blank screen. A blank/all-same
  color capture usually means the wrong window was captured or the GUI had
  not finished drawing; retry after the sim has stepped.

## Troubleshooting quick table

| Symptom | Likely cause | Fix |
|---|---|---|
| `xdotool search` finds nothing | GUI not rendered, wrong `DISPLAY`, or title differs | Confirm `gz sim` (no `-s`) is running and `DISPLAY` is set; check `xdotool search --name "Gazebo"`. |
| Captured image is blank / wrong size | Captured the wrong window or root fallback | Anchor the title regex; verify geometry with `xdotool getwindowgeometry`. |
| `import: couldn't open display` | `DISPLAY` unset or X auth missing | Export `DISPLAY`; for host forwarding ensure `/tmp/.X11-unix` and `Xauthority` are shared. |
| `import: Resource temporarily unavailable` | Window still mapping | The helper retries automatically; increase wait before first attempt. |
