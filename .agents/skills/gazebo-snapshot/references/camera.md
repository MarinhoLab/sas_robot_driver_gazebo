# Gazebo Headless Camera Snapshots — Details

Everything here is verified against `gz sim` (Gazebo/Jazzy) running in the
project's Docker image `ghcr.io/marinholab/gazebo:jazzy`.

## Camera image topic naming

The sensor publishes on:

```
/world/<world>/model/<model>/link/<link>/sensor/<sensor>/image
```

where `<world>` is the `name` of the `<world>` element, and the
`model` / `link` / `sensor` names are exactly as declared in the SDF.
There is also a companion `<...>/camera_info` topic.

To discover the exact name, list topics:

```bash
docker exec <container> bash -c 'gz topic -l | grep -i image'
```

Example: a camera `<model name="snapshot_camera">` in world `snapshot_world`
publishes on
`/world/snapshot_world/model/snapshot_camera/link/link/sensor/camera/image`
(note the duplicated `link/link` — the model link is named `link`).

## The three ways to add a camera (and which one works)

| Method | Publishes image? | Notes |
|---|---|---|
| **Bake a `<model>` with a `camera` sensor + `gz-sim-sensors-system` plugin directly into the world SDF** | **YES** | The only reliable method. Paste the model block inside `<world>`. |
| `<include>` a standalone camera-model SDF from the world | **NO** (observed) | The model appears in the scene (`/world/<w>/pose/info` lists it) but never publishes. Even with an identical sensor + Sensors plugin in the included file. Do not rely on it. |
| Runtime spawn via the `/world/<w>/create` service (`EntityFactory`) | **NO** (observed) | `n.request(...)` returns `ok=False` in this build. Not a reliable snapshot path. |
| `gz sim world.sdf model.sdf` (two file arguments) | **NO** | A second `.sdf` argument does NOT merge into the world; it replaces it. Use a single world file instead. |

Conclusion: always bake the camera into the world file for headless capture.

### Verified working camera `<model>`

```xml
<model name="snapshot_camera">
  <static>true</static>
  <pose>0 0 2.5 0 0 0</pose>
  <link name="link">
    <sensor name="camera" type="camera">
      <always_on>true</always_on>
      <update_interval>0.033</update_interval>
      <camera name="main">
        <horizontal_fov>1.047</horizontal_fov>
        <image>
          <width>640</width>
          <height>480</height>
          <format>R8G8B8</format>
        </image>
      </camera>
      <plugin filename="gz-sim-sensors-system" name="gz::sim::systems::Sensors"/>
    </sensor>
  </link>
</model>
```

A full ready-to-bake copy is in `assets/camera_model.sdf`.

### Required world plugins

The world must load the `SceneBroadcaster` and `UserCommands` systems (the
repo's `sdf/*_world.sdf` already include them):

```xml
<plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
<plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
```

## Headless launch

`gz sim -s world.sdf` runs the simulation server only (no GUI). No display is
required, but in Docker the container already sets `DISPLAY=:99`; keep it.
The `-s` (server-only) flag is what makes it headless.

```bash
docker exec -it <container> bash -c 'cd <dir> && gz sim -s <world>.sdf'
```

Run it in the background (nohup) so the container shell stays usable, then
capture from a second exec:

```bash
docker exec <container> bash -c 'cd <dir> && DISPLAY=:99 nohup gz sim -s <world>.sdf > /snap/sim.log 2>&1 & echo started'
sleep 12   # let the world load and the sensor start publishing
docker exec <container> bash -c 'gz topic -l | grep -i image'
```

Wait for the image topic to appear before capturing; it can take ~10 s after
launch.

## Capturing a frame

Use the bundled `scripts/snapshot_camera.py`:

```bash
docker exec <container> bash -c '
  python3 /snap/snapshot_camera.py \
    /world/<w>/model/snapshot_camera/link/link/sensor/camera/image \
    /snap/frame.png 20'
```

Copy the PNG out of the container if needed:

```bash
docker cp <container>:/snap/frame.png ./frame.png
```

The gz Python bindings are versioned; on the project image the working import
names are `gz.transport13` and `gz.msgs10.image_pb2`. The subscriber script
already targets those.

## Camera pose semantics (pitfalls)

Gazebo camera model axes: **+Z forward** (out of the lens), **+X right**,
**+Y down**, in the camera frame. The pose is the camera's position +
roll/pitch/yaw relative to its parent frame (here, the world).

Practical, verified pose examples to *see the origin area*:

| Pose | Orientation | Result |
|---|---|---|
| `0 0 2.5 0 0 0` | Straight above origin, 2.5 m up | **Verified reliable** bird's-eye view; the target box appears centered. Best default. |
| `0 -3 1.5 1.5708 0 0` | 3 m behind origin, 1.5 m up, roll +90° | Side view. In this build it produced a near-uniform frame when the target was small — confirm the target actually fills the frame before relying on it. |
| `0 -3 1.5 0 0 0` | 3 m behind, no roll | Camera +Z points +Y, so it looks at the origin but the image is **rolled 90°** (sideways). Add roll `1.5708` to level it. |

Gotchas:
- The overhead pose (`0 0 2.5 0 0 0`) is the most robust default: no roll to
  get wrong and the scene is seen top-down.
- A side pose can look like a uniform-gray frame even when the pipeline works
  (small target, weak lighting, or the target just out of frame). Verify the
  captured PNG actually contains the target (e.g. count target-colored pixels)
  rather than assuming success from the presence of an image topic.
- The camera "up" direction is whatever the world frame gives; there is no
  automatic level. If the image looks rotated, fix the roll in the pose.
- `<clip><near>0.05</near><far>100</far></clip>` (optional) controls the
  view frustum; defaults are fine.

## Materials that are hard to verify with a red-pixel check

If you want to confirm "the camera actually sees the scene" by counting
red-dominant pixels, use a target with a **strong emissive** material so it is
independent of lighting:

```xml
<material>
  <emissive>1 0 0 1</emissive>
  <diffuse>0.8 0.1 0.1 1</diffuse>
</material>
```

Standard `Gazebo/Red` or diffuse-only materials can render near-background
gray under headless rendering if lighting/geometry is not ideal, which looks
like "no capture" even though the pipeline works.

## Verifying a capture is valid

- Confirm PNG dimensions match the requested `<image>` width/height
  (e.g. 640x480).
- Confirm it is not a solid color: check that pixel variance across channels
  is non-trivial (a pure-gray frame is usually a pose/lighting issue, not a
  pipeline failure).
- For a known target, count target-colored pixels; a strong emissive target
  gives a reliable non-zero count.

## Troubleshooting quick table

| Symptom | Likely cause | Fix |
|---|---|---|
| No image topic in `gz topic -l` | Camera not baked into world, or world still loading | Bake the `<model>`; wait ~10 s; re-list. |
| Topic exists but script times out | Topic name typo; world paused | Verify exact topic; ensure sim is running (`gz sim -s`). |
| Image is solid gray / one color | Camera pose pointing at nothing, or top-down no-shading, or diffuse target under weak light | Adjust pose (roll/level); use emissive target; raise light intensity. |
| Image is rotated/sideways | Missing roll in pose | Add roll `1.5708` (or -1.5708) to the pose. |
| `update_interval` "not defined in SDF" warning | Element placed under wrong parent | Keep `update_interval` under `<camera>`; it is tolerated but warn — harmless. |
| `gz.msgs.PixelFormatType` import errors / `AttributeError` | Wrong versioned module name | Use the versioned modules that exist in the image (`gz.msgs10`, `gz.transport13` for the Jazzy image). |

## Runtime-spawn reference (if you must add a camera to a live world)

The `/world/<w>/create` service exists and accepts an `EntityFactory` request
with `sdf_filename` set to a world-relative SDF. The request/response types in
the in-image Python bindings are:

```python
import gz.transport13 as t
import gz.msgs10.entity_factory_pb2 as ef
import gz.msgs10.entity_pb2 as ent

node = t.Node()
req = ef.EntityFactory()
req.sdf_filename = "/abs/path/camera_model.sdf"   # or set req.sdf to the SDF string
ok, resp = node.request("/world/<w>/create", req,
                        ef.EntityFactory, ent.Entity, 5000)
```

Note: in this Gazebo build the spawned camera did not publish an image topic,
so treat this as an advanced/unsupported path for snapshots. Prefer baking the
camera into the world file.
