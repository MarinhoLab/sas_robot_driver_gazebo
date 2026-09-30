---
name: gazebo-snapshot
description: This skill should be used when the user asks to "take a snapshot from a Gazebo camera", "capture the Gazebo GUI window", "screenshot the Gazebo simulation", "get a headless camera image", or otherwise wants to capture an image from the GazeboSim simulation running in Docker — either from a camera sensor inside the sim (headless) or from the rendered Gazebo GUI window (headful). All Gazebo execution for the `sas_robot_driver_gazebo` project must go through the project Docker image.
---

# Gazebo Snapshot (Camera + GUI)

Capture images from the GazeboSim simulation used by `sas_robot_driver_gazebo`.
Two independent workflows:

1. **Headless camera snapshot** — read a frame from a Gazebo `camera` sensor's
   image topic while the sim runs server-only (`gz sim -s`). No display needed.
2. **Headful GUI snapshot** — screenshot the rendered **Gazebo Sim** window
   while the sim runs with its GUI. Needs an X server (Xvfb in-container, or
   host X11 forwarding).

All Gazebo commands run inside the project Docker image
`ghcr.io/marinholab/gazebo:jazzy` (see `docker/Dockerfile`); Gazebo is not
installed on the host. Use `docker exec` to run commands in a running container.

## Environment / applicability

These commands and their gotchas were **verified on the project Docker image**,
which is **Ubuntu 24.04 (Noble) + ROS 2 Jazzy + Gazebo Sim v8 ("Harmonic")**,
using the Debian `apt` package manager. The environment-specific parts:

- Python bindings `gz.transport13` and `gz.msgs10` in `scripts/snapshot_camera.py`
  are version-tied to this Gazebo/ROS release. On a different Gazebo version the
  `N` in `gz.transportN` / `gz.msgsN` changes — update the imports.
- The GUI window title `Gazebo Sim` (used by `scripts/snapshot_gui.sh`) is the
  Gazebo v8 title; other Gazebo major versions may title the window differently.
- `xdotool` / `imagemagick` / `Xvfb` are installed via `apt-get install -y …`
  (Debian/Ubuntu). On other distros use the equivalent package manager.
- The exact camera `<model>` block, plugin names, and topic paths assume
  Gazebo v8. The *approach* is the same elsewhere, but re-verify each detail
  after changing OS or Gazebo version.

The two workflows themselves (bake a camera / screenshot the GUI window) are
portable across OSs; only the concrete package names, binding versions, and
window title may need adjusting.

## Prerequisites

- A Gazebo container is running. If none exists, build/start one from
  `docker/` (e.g. `docker compose -f docker/service_gazebo.yml up` or a
  `docker build -f docker/Dockerfile .` + `docker run`), then work in it.
  For a quick isolated test, launch a bare container from the same base image:
  `docker run -dit --name gz_snap ghcr.io/marinholab/gazebo:jazzy bash`.
- Copy the helper script(s) into the container before use, e.g.
  `docker cp .agents/skills/gazebo-snapshot/scripts/snapshot_camera.py <c>:/snap/`.

## Workflow 1 — Headless camera snapshot

### Step 1: Bake a camera into the world SDF

The repo's `sdf/*_world.sdf` files contain **no camera**. Add one by pasting a
`<model>` with a `camera` sensor and the `gz-sim-sensors-system` plugin
**directly inside the `<world>` tag** of the target world file (make an edited
copy, e.g. `/snap/<world>_snapshot.sdf`).

A ready-to-paste model is in `assets/camera_model.sdf`. Key requirements:

- The sensor block must include the plugin:
  ```xml
  <plugin filename="gz-sim-sensors-system" name="gz::sim::systems::Sensors"/>
  ```
  Without it, no image topic is published.
- `<update_interval>` may sit under `<sensor>` or `<camera>`; either works and
  publishes. Placing it under the wrong parent prints a harmless "copying as
  child" warning.
- The world must already load the `SceneBroadcaster` and `UserCommands`
  systems — the repo world files do.

**Critical gotcha:** a camera added via `<include>` or via the runtime
`/world/<w>/create` service does **not** publish an image topic in this Gazebo
build, even with an identical sensor + plugin. Only a camera model defined
**inline** in the world file reliably publishes. Also, `gz sim world.sdf
model.sdf` does not merge the two files — pass a single world file. See
`references/camera.md` for the full comparison and a verified working model.

### Step 2: Launch the sim server-only

```bash
docker exec <c> bash -c 'cd <dir> && DISPLAY=:99 nohup gz sim -s <world>.sdf > /snap/sim.log 2>&1 & echo started'
sleep 12
docker exec <c> bash -c 'gz topic -l | grep -i image'   # confirm topic exists
```

`-s` means server-only (headless). Wait until the image topic appears (~10 s).
The topic follows `/world/<world>/model/<model>/link/<link>/sensor/<sensor>/image`
(e.g. `/world/snapshot_world/model/snapshot_camera/link/link/sensor/camera/image`).

### Step 3: Capture one frame to PNG

```bash
docker exec <c> bash -c 'python3 /snap/snapshot_camera.py <image-topic> /snap/frame.png 20'
docker cp <c>:/snap/frame.png ./frame.png
```

`snapshot_camera.py` subscribes with the in-image `gz.transport13` bindings,
grabs the first `gz.msgs.Image`, decodes it with Pillow, and saves a PNG.

## Workflow 2 — Headful GUI snapshot

### Step 1: Launch with the GUI on an X server

- **Xvfb in the container** (recommended, no monitor): the project's
  `docker/test.sh` already starts `Xvfb :99`.
  ```bash
  docker exec <c> bash -c 'pgrep -x Xvfb >/dev/null || /usr/bin/Xvfb :99 -screen 0 1280x1024x24 & sleep 3; cd <dir> && DISPLAY=:99 nohup gz sim <world>.sdf > /snap/gui.log 2>&1 & echo started'
  sleep 12
  ```
- **Host X11 forwarding:** run `gz sim <world>.sdf` (no `-s`) with the host's
  `DISPLAY`; the window appears on the host. The capture then runs on the host.

Do **not** pass `-s` — that disables the GUI.

### Step 2: Screenshot the Gazebo Sim window

```bash
docker exec <c> bash -c 'DISPLAY=:99 bash /snap/snapshot_gui.sh /snap/gui.png'
docker cp <c>:/snap/gui.png ./gui.png
```

`snapshot_gui.sh` finds the window titled exactly `Gazebo Sim` and captures it
with ImageMagick `import` (needs `xdotool` + `imagemagick`, installable via
`apt-get install -y xdotool imagemagick`). It retries while the window maps and
falls back to the X root window. For host X11 forwarding, run the script on the
host with the host `DISPLAY` instead.

## Gotchas summary

- **Headless:** camera must be baked inline (not included/spawned) and must
  carry the `gz-sim-sensors-system` plugin; wait for the topic; verify the PNG
  is the expected size and not a solid color.
- **Pose semantics:** an identity (unrotated) camera looks along the +X axis,
  so an overhead camera needs **pitch +90°** (`0 0 6 0 1.5708 0`) to look
  straight down; without it the target only peeks in as a band at the frame
  edge. Raise altitude for context; verify the target is actually centered in
  the PNG. See `references/camera.md` for the full verified pose table.
- **Headful:** exact window title `Gazebo Sim`; use
  `xdotool getwindowgeometry --shell`; capture on the same X server as the GUI;
  use an emissive target (or a lit scene) if verifying pixels. See
  `references/headful.md`.

## Additional Resources

### Reference Files
- **`references/camera.md`** — camera topic naming, the three add-a-camera
  methods (and which work), verified camera model, launch + capture steps, pose
  semantics, material/pixel verification, runtime-spawn details, troubleshooting.
- **`references/headful.md`** — Xvfb vs X11-forwarding, GUI launch, window
  capture, alternative `xwd`/root capture, window/geometry pitfalls,
  troubleshooting.

### Scripts
- **`scripts/snapshot_camera.py`** — capture one frame from a camera image
  topic to PNG (run inside the Gazebo container).
- **`scripts/snapshot_gui.sh`** — screenshot the Gazebo GUI window to PNG.

### Assets
- **`assets/camera_model.sdf`** — ready-to-bake camera model for the headless
  workflow.
