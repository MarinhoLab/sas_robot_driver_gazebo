# convert/ — agent notes

Detailed mechanics for the robot-model conversion scripts in this folder.
`README.md` covers human-facing usage; this file is the operational contract
for AI agents working on these scripts.

## Two-layer design

- `host_convert_<vendor>.sh` — runs **on the host machine**, from the
  repository root. Builds the converter image, runs a container, and copies
  the result back out.
- `convert_<vendor>.sh` — runs **inside the container** at `/root/sas`
  (the repo is baked into the image by `docker/converter/Dockerfile` via
  `COPY --from=repo . /root/sas/`). Converts a vendor URDF to SDF using
  `gz sdf -p`.

The host script is the only entry point users (and agents) should invoke.
The container script is an implementation detail of the host script.

## Invocation contract (do not break)

1. Host scripts must be run from the **repository root**: they build with
   `--build-context repo=.`, which must resolve to the repo.
2. The container invocation must keep `cd /root/sas` before sourcing the
   container script:
   - `convert_ur.sh` uses relative `vendor/...` paths, so its CWD must be
     `/root/sas`.
   - All container scripts capture `WORKDIR=$(pwd)` and copy the generated
     SDF there; the host script then does
     `docker cp extract_files:/root/sas/<robot>.sdf` — if the CWD changed,
     the `docker cp` source path stops matching.
3. If these scripts are moved again, update the `docker exec ... bash -c`
   lines in all three host scripts (the container path mirrors the repo
   layout).

## Docker specifics

- Image: `srdg_convert`, built from `docker/converter/Dockerfile`
  (base: `ghcr.io/marinholab/gazebo:jazzy`).
- Container: `extract_files`, started detached with a kept-open shell
  (`docker run -t -d ... /bin/bash`), driven via `docker exec`, then
  stopped. Cleanup of stale containers is best-effort (`|| true`).
- The build is `--no-cache`; image builds take several minutes — don't
  rebuild unless `docker/converter/Dockerfile` or the repo root layout
  changed.

## Vendor quirks

- **UR** (`convert_ur.sh`): builds the vendored `ur_description` package with
  `colcon build` first (its `install/setup.bash` is sourced afterwards), then
  renders the xacro directly
  (`xacro ur.urdf.xacro name:="<robot>_1" ur_type="<robot>"`). Robot argument
  is any UR type shipped in
  `Universal_Robots_ROS2_Description` (e.g. `ur3e`, `ur30`, `ur5`).
- **Unitree** (`convert_unitree.sh`): the `<robot>_description` packages are
  ROS 1 (catkin) and cannot be `colcon build`ed on ROS 2. The xacro only
  resolves sibling xacro files via `$(find <robot>_description)`, so the
  script registers the package in a minimal ament stub
  (`/tmp/srdg_ament_stub`, exported as `AMENT_PREFIX_PATH`) and renders
  `robots/<robot>_description/xacro/robot.xacro` directly.
- **AgileX** (`convert_agilex.sh`): the vendor URDF path contains a typo in
  the package folder name (`..._desription` — "description" misspelled in
  the upstream `ugv_gazebo_sim` repo). Keep the typo when editing the path;
  do not "fix" it.

## Output convention

Each container script writes `<robot>.sdf` to `WORKDIR` (`/root/sas`) and
returns there; the host script copies it out to its own CWD (repo root when
used as documented). The generated SDF is a build artifact — it is not
committed by these scripts; `sdf/` holds the models that ship with the
package.
