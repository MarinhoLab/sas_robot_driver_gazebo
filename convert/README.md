# Convert scripts

Convert vendor robot models (UR, Unitree, AgileX) into Gazebo SDF files.

## Prerequisites

- Docker (the conversion runs inside the `srdg_convert` image built from
  `docker/converter/`).
- The vendor models, which are not shared in this repository. Inside the
  container they live under `/root/sas/vendor` (see `scripts/setup_vendor.sh`
  and the root `README.md`).

## Usage

Run the `host_convert_*` scripts from the repository root, passing the robot
name as the first argument:

```console
# Universal Robots (e.g. ur3e, ur30, ur5)
./convert/host_convert_ur.sh ur3e

# Unitree (e.g. b2, go2, a1)
./convert/host_convert_unitree.sh b2

# AgileX (e.g. r820)
./convert/host_convert_agilex.sh r820
```

## Output

The script writes `<robot>.sdf` to your current working directory. Move the
result into `sdf/` (and register it with the Gazebo resource path) if you want
it to ship with the package.

See `AGENTS.md` in this folder for the detailed mechanics (host vs. container
scripts, invocation contract, per-vendor quirks).
