# Benchmark: how late the robot driver bridge reports Gazebo's joints

The bridge (`sas_robot_driver_ros_gazebo_node`, or the script
`sas_robot_driver_ros_gazebo.py`) forwards target joint positions to Gazebo
and reports Gazebo's joint positions on `<robot_name>/get/joint_states`. This
benchmark measures how late, and how far off, those reports are, with the
former pure-Python `RobotDriverGazebo` and with the C++ one that replaced it,
and profiles the Python bridge to find why it fell behind.

## Setup

- `ghcr.io/marinholab/sas-full:jazzy` (ROS 2 Jazzy, Gazebo Harmonic: gz-sim
  8.15, gz-transport 13.6, Python 3.12) under Docker Desktop on an arm64 Mac;
  this package built with colcon on top of it.
- `ur3e_world.sdf` (physics and joint states every 2 ms), `config/config.yaml`
  (`ur3e_1`), `robot_driver_server_launch.py`.
- Load: `joint_interface_example.py --ros-args -p robot_topic_name:=/ur3e_1`
  (each joint ±10° about its start, sinusoids, 100 Hz commands).
- Measurement: `scripts/bridge_lag_monitor.py`, 45 s; the analysis skips the
  first 10 s.
- CPU: the bridge process's `%CPU` from `top -b -d 1`, mean after the first
  5 s.

### The monitor

`bridge_lag_monitor.py` samples, every 20 ms, the joint positions Gazebo
publishes (gz-transport) and those the bridge reports (ROS 2), and prints the
shift that best aligns the two series (the delay) and their largest gap.

The monitor is itself a Python process with a Gazebo callback, so it can fall
behind in the same way as the bridge. A first version polled ROS 2 in a tight
loop (`rclpy.spin_once(timeout_sec=0)`) and starved its own Gazebo callback:
its "Gazebo" positions were stale, which hid most of the Python bridge's lag.
The figures in #20's description came from it. The monitor now throttles its
Gazebo subscription (`SubscribeOptions.msgs_per_sec = 50`) and waits in its
ROS 2 spin (`timeout_sec=0.005`). With it, Gazebo's joints show the full ±10°
motion in every run below.

## Results

| Bridge | `thread_sampling_time_sec` | Reported vs Gazebo | Largest gap | CPU (bridge) |
|---|---|---|---|---|
| Python driver (former) | 0.002 | no update during the run (≈ 18 s late) | 10° (the whole motion) | 26–28 % |
| Python driver (former) | 0.01 | 93–135 ms late | 1.2–1.9° | 19 % |
| C++ driver, from `sas_robot_driver_ros_gazebo.py` | 0.002 | no measurable lag | 0.17° | 32 % |
| C++ driver, from `sas_robot_driver_ros_gazebo.py` | 0.001 | no measurable lag | 0.17° | 43 % |
| C++ node | 0.002 | no measurable lag | 0.18° | 28 % |
| C++ node | 0.001 | no measurable lag | 0.17° | 39 % |
| C++ node, default launch (this branch) | 0.002 | 0 ms | 0.16° | |
| `implementation:=python` (script, C++ driver through the package), this branch | 0.002 | 0 ms | 0.16° | |

"No measurable lag": the best alignment is no shift; a gap of 0.17° is the
monitor's 20 ms sampling of the ±10° motion. With the Python driver at 2 ms,
the reported positions stayed at their initial values for the whole run in
one measurement, and began to move after about 20 s, about 17 s late, in
another.

### Two composed bridges

The R820 + UR3e of [mmmarinho/random](https://github.com/mmmarinho/random)
(`2024_revolute_curve`, `curve_revolution_vfi.gazebo.sas_driver_loop`): one
13-joint model in Gazebo, two bridges (`r820_sim`, `ur3e_sim`) composed by
`sas_robot_driver_ros_composer_node` (`longboy_sim`), a client sending
targets every 2 ms along a 26 s passage that drives several joints at their
velocity limits.

| Bridges | `thread_sampling_time_sec` | Reported vs Gazebo |
|---|---|---|
| Python drivers (former) | 0.002 | seconds late, growing while the robot moves |
| Python drivers (former) | 0.01 | 12–20 ms late |
| C++ nodes | 0.002 | 2–4 ms late, within 0.6° |

These are qualitative: Gazebo's joints were read by the client itself, a
Python process with its own Gazebo callback, which may fall behind as the
first monitor did; the single-bridge table above is the reference.

## Profile of the Python bridge

`py-spy record --native` on the bridge (former Python driver, 2 ms), 10 s at
100 Hz:

| Thread | Samples | Where |
|---|---|---|
| main (`RobotDriverROS.control_loop()`) | 82 % | mostly `clock_nanosleep` (the loop's sleep between periods) |
| gz-transport callback (`cb_deserialize` → `joint_states_callback`) | 3 % | parsing the joint-state message |
| other (waits) | 15 % | `pthread_cond_timedwait` |

`py-spy record --gil --native` (only the thread holding the Python
interpreter lock): **67 %** of the samples are the main thread in
`clock_nanosleep`, inside `control_loop()`.

So `sas_robot_driver.RobotDriverROS.control_loop()`, a C++ loop called from
Python, holds the interpreter lock while it sleeps between periods. The
gz-transport thread that delivers Gazebo's joint states to the Python
callback needs that lock, gets it rarely, and the driver's cached joint
positions fall behind or stop. What did not help (former Python driver, 2 ms):

- keeping only the latest message in the callback and parsing it when read:
  no update during the run;
- throttling the joint-state subscription (`msgs_per_sec = 100`): ≈ 21 s
  late;
- publishing joint states at 50 Hz instead of 500 Hz (JointStatePublisher
  `update_rate`): ≈ 13 s late.

A slower loop (0.01 s) gives the callback more chances (93–135 ms). The C++
driver's callback takes no interpreter lock, so it runs as soon as a message
arrives, from the C++ node and from Python alike.

## Reproducing

```bash
# Build (this branch), in sas-full:jazzy, with the submodule
git submodule update --init
colcon build --packages-select sas_robot_driver_gazebo
source install/setup.bash

# Gazebo, the bridge (C++ node; implementation:=python for the script) and the load
cd $(ros2 pkg prefix sas_robot_driver_gazebo)/share/sas_robot_driver_gazebo/sdf
gz sim -s -r ur3e_world.sdf &
ros2 launch sas_robot_driver_gazebo robot_driver_server_launch.py &
ros2 run sas_robot_driver_gazebo joint_interface_example.py --ros-args -p robot_topic_name:=/ur3e_1 &

# The measurement
ros2 run sas_robot_driver_gazebo bridge_lag_monitor.py \
    /world/ur3e_world/model/ur3e/model/ur3e_position_controller/model/ur3e/joint_state \
    /ur3e_1/get/joint_states 45 \
    shoulder_pan_joint shoulder_lift_joint elbow_joint wrist_1_joint wrist_2_joint wrist_3_joint
```

The former Python driver is `sas_robot_driver_gazebo/sas_robot_driver_gazebo.py`
at commit 2bb09a7 (`jazzy` before this change); to measure it, run that
commit's `sas_robot_driver_ros_gazebo.py` with the monitor of this branch.
For the thread rate, set `thread_sampling_time_sec` in the configuration
file. The profile: `pip install py-spy`, run the container with
`--cap-add SYS_PTRACE`, then
`py-spy record -p <bridge pid> -d 10 -r 100 -f raw --native [--gil]`.
