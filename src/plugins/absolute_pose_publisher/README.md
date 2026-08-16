# Absolute Pose Publisher for Gazebo Harmonic

A minimal world-level Gazebo system plugin that publishes world-frame poses for
all models and, optionally, all links and joints.

Each `gz.msgs.Pose` entry contains:

- `id`: Gazebo entity ID
- `name`: Gazebo scoped name, such as `robot::arm_link`
- `position` and `orientation`: pose resolved in the world frame

The world name is deliberately excluded from the scoped name.

## Requirements

Gazebo Harmonic development packages, including:

- `gz-sim8`
- `gz-msgs10`
- `gz-transport13`
- CMake and a C++17 compiler

On Ubuntu with Gazebo Harmonic installed from packages, the required development
packages are normally pulled in by the Gazebo installation.

## Build

```bash
cd absolute_pose_publisher
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

## Run the example

```bash
export GZ_SIM_SYSTEM_PLUGIN_PATH="$PWD/build:${GZ_SIM_SYSTEM_PLUGIN_PATH:-}"
gz sim -r example_world.sdf
```

In another terminal, preserving the same Gazebo environment:

```bash
gz topic -e -t /world/my_world/absolute_pose/info
```

## Add it to an existing world

Place this inside the world's `<world>` element:

```xml
<plugin
    filename="AbsolutePosePublisher"
    name="absolute_pose_publisher::AbsolutePosePublisher">
  <topic>/world/my_world/absolute_pose/info</topic>
  <update_rate>50</update_rate>
  <publish_links>true</publish_links>
  <publish_joints>true</publish_joints>
</plugin>
```

Then ensure the build directory is discoverable:

```bash
export GZ_SIM_SYSTEM_PLUGIN_PATH="/absolute/path/to/absolute_pose_publisher/build:${GZ_SIM_SYSTEM_PLUGIN_PATH:-}"
```

## Configuration

- `topic`: output topic; default is `/absolute_pose/info`
- `update_rate`: publication rate in Hz; a non-positive value publishes every
  unpaused simulation iteration
- `publish_links`: whether link poses are included; default is `true`
- `publish_joints`: whether joint poses are included; default is `false`

Models are always published.
