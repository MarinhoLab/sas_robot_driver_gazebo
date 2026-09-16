# sas_robot_driver_gazebo

> [!TIP]
> Repository for this module: https://github.com/MarinhoLab/sas_robot_driver_gazebo <br/>
> More information about SmartArmStack is available in https://smartarmstack.github.io/.

## Licenses

Check any licenses in repositories in `vendor` if you decide to use them. This repository's owner holds no
liability for misuse. 

## Folder structure

The two folders below have special meanings, the others follow usual naming.

| Folder    | Meaning                                                                                                    |
|-----------|------------------------------------------------------------------------------------------------------------|
| `sdf`     | Base Gazebo scene files.                                                                                   |
| `vendor`  | Vendor models. Check the licenses if you decide to use them in your work. They are not shared in this repo |

## Technical Overview

Each relevant joint to be controlled should have a plugin specification similar to the following, using `gz::sim::systems::JointPositionController`.

## Joint Position Control

```xml
<plugin
filename="gz-sim-joint-position-controller-system"
name="gz::sim::systems::JointPositionController">
    <joint_name>shoulder_pan_joint</joint_name>
    <use_velocity_commands>True</use_velocity_commands>
    <cmd_max>0.25</cmd_max>
</plugin>
```

## Joint State Reading

Joints are read from a default `gz::sim::systems::JointStatePublisher`.

```xml
<plugin
    filename="gz-sim-joint-state-publisher-system"
    name="gz::sim::systems::JointStatePublisher">
</plugin>
```

## Joint State Reading

```xml
<plugin filename="gz-sim-scene-broadcaster-system"  name="gz::sim::systems::SceneBroadcaster"/>
```

## Joint State Reading

```xml
<plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
```

## ROS 2 Nodes & Parameters

Each node loads its parameters from a YAML configuration file. The default is
`config/config.yaml` in this package; pass a different file with the
`config_file:=` launch argument of the corresponding launch file.

### Node: `sas_robot_driver_ros_gazebo`

| Property | Value |
|---|---|
| **Executable** | `sas_robot_driver_ros_gazebo.py` |
| **ROS node name** | `ur3e_1` (set by the `name` launch argument of `robot_driver_server_launch.py`) |
| **Description** | Bridges ROS and Gazebo. Subscribes to Gazebo joint states and publishes target joint positions, running the `RobotDriverROS` control loop. |

#### Parameters

| Parameter | Type | Mandatory / Optional | Default | Purpose |
|---|---|---|---|---|
| `joint_names` | array of strings | **Mandatory** | none — must be provided | Names of the Gazebo joints to control |
| `joint_positions_topic_prefix` | string | **Mandatory** | none — must be provided | Gazebo topic prefix for target joint positions |
| `joint_states_topic` | string | **Mandatory** | none — must be provided | Gazebo topic of the joint states to read |
| `robot_name` | string | **Mandatory** | none — must be provided | Name of the robot; used as the robot-driver topic prefix |
| `thread_sampling_time_sec` | double | Optional | `0.002` | Sampling period of the control-loop thread |

### Node: `sas_object_server_gazebo_node`

| Property | Value |
|---|---|
| **Executable** | `sas_object_server_gazebo_node` |
| **ROS node name** | `sas_object_server_gazebo_node` |
| **Description** | Synchronizes the poses of the configured world entities with ROS: sends poses to the Gazebo `set_pose` service and publishes them on the `get_pose` topic. |

#### Parameters

| Parameter | Type | Mandatory / Optional | Default | Purpose |
|---|---|---|---|---|
| `set_pose_service_name` | string | **Mandatory** | none — must be provided | Gazebo `set_pose` service name |
| `get_pose_topic_name` | string | **Mandatory** | none — must be provided | Gazebo absolute-pose info topic name |
| `entity_names` | array of strings | **Mandatory** | none — must be provided | World entities whose poses are synchronized |
| `thread_sampling_time_sec` | double | Optional | `0.01` | Sampling period of the synchronization loop |

### Node: `sas_simulator_server_gazebo_node`

| Property | Value |
|---|---|
| **Executable** | `sas_simulator_server_gazebo_node` |
| **ROS node name** | `sas_simulator_server_gazebo_node` |
| **Description** | Binds Gazebo world control services (start/stop/step simulation) to ROS. |

#### Parameters

| Parameter | Type | Mandatory / Optional | Default | Purpose |
|---|---|---|---|---|
| `service_name` | string | **Mandatory** | none — must be provided | Gazebo world control service name (e.g. `/world/<world>/control`) |
| `thread_sampling_time_sec` | double | Optional | `0.01` | Sampling period of the control loop |
| `autostart` | bool | Optional | `true` | Start the simulation on node startup |

**How mandatory/optional is determined in code:**
- **Mandatory** params are read with `sas::get_ros_parameter(...)` — if missing, the node throws and fails to start.
- **Optional** params are read with `sas::get_ros_optional_parameter(..., <default>)` — they carry in-code defaults.

**Launch arguments** (`simulator_server_launch.py`): `name` and `config_file`. The simulation is autostarted by default (`autostart: true` in the configuration file and in the node's in-code default); to disable it, pass a configuration file that sets `autostart: false` (e.g. `ros2 launch sas_robot_driver_gazebo simulator_server_launch.py config_file:=/path/to/no_autostart.yaml`).

#### Sample launches

```console
ros2 launch sas_robot_driver_gazebo robot_driver_server_launch.py
ros2 launch sas_robot_driver_gazebo object_server_launch.py
ros2 launch sas_robot_driver_gazebo simulator_server_launch.py
```

## SDF Serial-Manipulator Loader

The package builds an `M3_SerialManipulatorSimulatorFriendly` kinematic model
(vendored verbatim from
[MarinhoLab/working-needlemanipulation](https://github.com/MarinhoLab/working-needlemanipulation))
from a Gazebo-SDF model. The model is exposed two ways:

- **Client library** — `sas_robot_driver_gazebo::SdfSerialManipulatorLoader`
  (header `sas_robot_driver_gazebo/SdfSerialManipulatorLoader.h`, linked via the
  `serial_manipulator_sdf` target).
- **CLI** — the `sdf2manipulator` executable prints the loaded kinematics as
  YAML, mirroring the schema consumed by
  `marinholab.working.needlemanipulation.example_load_from_file.get_information_from_file`.

### Mapping

A serial-robot SDF model maps one-to-one onto the per-joint vectors of the M3
model:

| SDF entity | M3 model field |
|---|---|
| base (canonical) link `<pose>` | `reference_frame_` |
| joint `i` `<pose>` relative to its parent link | `offset_before_[i]` |
| joint `i` `<type>` + `<axis><xyz>` | `actuation_types_[i]` (`RZ` revolute/continuous, `TZ` prismatic) |
| child link `i` `<pose>` relative to joint `i` | `offset_after_[i]` |
| joint `i` `<axis><limit>` | lower / upper `q` limit |

The SDF-to-dual-quaternion conversion is a homomorphism with respect to
`dqrobotics`' `DQ::operator*` (verified against `gz::math::Pose3d` composition
and an independent 4×4 matrix chain).

### Requirements

- The model must be a strictly serial chain (a base link followed by joint/link
  pairs); non-serial topologies raise `std::runtime_error`.
- Every joint must act about the **+z axis of its own (joint-local) frame**; a
  non-+z or expressed-in-another-frame axis raises `std::runtime_error`.
- Only revolute, continuous, and prismatic joints are supported.

### CLI

```console
# Print the kinematics of a serial-manipulator SDF model as YAML.
sdf2manipulator sdf/r820.sdf

# Also print the per-joint limits.
sdf2manipulator sdf/r820.sdf --joint-limits
```

`r820.sdf` in this repository is a ready-made test input: 7 revolute joints,
all acting about +z.

### Test

`test_sdf_serial_manipulator_loader` is a standalone test that loads an SDF
model and checks the chain structure (joint count, actuations, DOF) and that
`model.fkm(q)` matches an independent 4×4 matrix chain for random
configurations:

```console
build/sas_robot_driver_gazebo/test_sdf_serial_manipulator_loader sdf/r820.sdf
```

## Considerations

- `gz::sim::systems::PosePublisher` has been considered to read poses of entities. However, it's more convenient for `tf2` given how the frames are described. 
- The translation tool inside Gazebo can be used to move objects and reading their pose works only after the motion is finished. The intermediate state is not reflected in the `pose` topic.
