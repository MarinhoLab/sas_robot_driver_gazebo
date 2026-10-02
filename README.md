# sas_robot_driver_gazebo

> [!TIP]
> Repository for this module: https://github.com/MarinhoLab/sas_robot_driver_gazebo <br/>
> More information about SmartArmStack is available in https://smartarmstack.github.io/.

## Licenses

Check any licenses in repositories in `vendor` if you decide to use them. This repository's owner holds no
liability for misuse. 

## Folder structure

The folders below have special meanings, the others follow usual naming.

| Folder    | Meaning                                                                                                    |
|-----------|------------------------------------------------------------------------------------------------------------|
| `sdf`     | Base Gazebo scene files.                                                                                   |
| `vendor`  | Vendor models. Check the licenses if you decide to use them in your work. They are not shared in this repo |
| `convert` | Vendor-model to Gazebo-SDF conversion scripts. See `convert/README.md` for usage.                          |

## Vendor Setup

Vendor robot models are not shared in this repository. `scripts/setup_vendor.sh`
clones them into `~/.sas/sas_robot_driver_gazebo/vendor`, skipping any that are
already present:

```console
# Clone a single vendor's models
./scripts/setup_vendor.sh ur

# Clone all supported vendors (ur, unitree, bota, agilex)
./scripts/setup_vendor.sh all
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
| `thread_sampling_time_sec` | double | Optional | `0.01` | Sampling period of the control-loop thread (see [Bridge loop period](#bridge-loop-period)) |

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

```console
# Print the kinematics of a serial-manipulator SDF model as YAML.
sdf2manipulator sdf/r820.sdf

# Also print the per-joint limits.
sdf2manipulator sdf/r820.sdf --joint-limits

# Load the kinematic model from a world file (auto-detected).
sdf2manipulator sdf/ur3e_world.sdf

# Select a specific (possibly nested) model by its "::"-separated scope.
sdf2manipulator sdf/ur3e_world.sdf --model-scope "ur3e::ur3e_position_controller::ur3e"
```

`r820.sdf` (7 revolute joints) and `ur30.sdf` / `ur3e.sdf` (6 revolute joints
each) are ready-made model-file test inputs, and `r820_world.sdf` /
`ur3e_world.sdf` are ready-made world-file inputs that nest the same robots
behind `<include>` chains; all have every joint acting about +z.

## Considerations

### Bridge loop period

The bridge (`sas_robot_driver_ros_gazebo.py`) runs its `RobotDriverROS` loop
every `thread_sampling_time_sec`, 0.01 s (100 Hz) by default. At 0.002 s
(500 Hz), the joint positions it reports fall far behind Gazebo's: in
`ur3e_world.sdf` with `joint_interface_example.py` (±10° sinusoids), they did
not update at all during a 45 s run. At 0.01 s they are 93–135 ms behind
(within 1.2–1.9°). `sas_robot_driver.RobotDriverROS.control_loop()` holds the
Python interpreter lock while its C++ loop sleeps between periods (py-spy
`--gil`: 67 % of the samples), so the gz-transport thread running the Python
joint-state callback rarely runs; a longer period gives it more chances. The
commands reach Gazebo at once.
