#!/bin/bash

sudo apt-get install tree
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get remove -y ros-jazzy-sas-robot-driver-gazebo
sudo apt-get install -y xvfb tree
sudo /usr/bin/Xvfb :99 -screen 0 1280x1024x24 &

cd /root/sas_robot_driver_gazebo_devel
ls .
colcon build
source install/setup.bash
echo "GZ_SIM_SYSTEM_PLUGIN_PATH = $GZ_SIM_SYSTEM_PLUGIN_PATH"

cd $(ros2 pkg prefix sas_robot_driver_gazebo --share)
tree .

# Verify setup_vendor.sh is installed and callable
SCRIPT="$(ros2 pkg prefix sas_robot_driver_gazebo)/lib/sas_robot_driver_gazebo/setup_vendor.sh"
test -x "$SCRIPT" && echo "PASS: setup_vendor.sh is installed and executable at $SCRIPT" || (echo "FAIL: script not found or not executable" && exit 1)
bash "$SCRIPT" --help 2>&1 || true

# Verify GZ_SIM_RESOURCE_PATH includes local sdf, local vendor, and home vendor
echo "$GZ_SIM_RESOURCE_PATH" | grep -q "sas_robot_driver_gazebo/sdf" && \
  echo "PASS: GZ_SIM_RESOURCE_PATH includes local sdf" || \
  (echo "FAIL: GZ_SIM_RESOURCE_PATH missing local sdf" && exit 1)
echo "$GZ_SIM_RESOURCE_PATH" | grep -q "sas_robot_driver_gazebo/vendor" && \
  echo "PASS: GZ_SIM_RESOURCE_PATH includes local vendor" || \
  (echo "FAIL: GZ_SIM_RESOURCE_PATH missing local vendor" && exit 1)
echo "$GZ_SIM_RESOURCE_PATH" | grep -q ".sas/sas_robot_driver_gazebo/vendor" && \
  echo "PASS: GZ_SIM_RESOURCE_PATH includes home vendor dir" || \
  (echo "FAIL: GZ_SIM_RESOURCE_PATH missing home vendor dir" && exit 1)

# Verify SDF files parse correctly (r820 uses local mujoco meshes, ur3e needs cloned vendor repos)
gz sdf -p "$(ros2 pkg prefix sas_robot_driver_gazebo --share)/sdf/r820.sdf" > /dev/null 2>&1 && \
  echo "PASS: r820.sdf parsed successfully" || \
  (echo "FAIL: r820.sdf parse failed" && exit 1)
gz sdf -p "$(ros2 pkg prefix sas_robot_driver_gazebo --share)/sdf/reference_frame.sdf" > /dev/null 2>&1 && \
  echo "PASS: reference_frame.sdf parsed successfully" || \
  (echo "FAIL: reference_frame.sdf parse failed" && exit 1)

# Verify the serial-manipulator-SDF loader builds a correct kinematic model
# from r820.sdf (7 x RZ joints; fkm cross-checked against an independent 4x4 chain).
TEST_BIN="/root/sas_robot_driver_gazebo_devel/build/sas_robot_driver_gazebo/test_sdf_serial_manipulator_loader"
R820_SDF="$(ros2 pkg prefix sas_robot_driver_gazebo --share)/sdf/r820.sdf"
"$TEST_BIN" "$R820_SDF" > /tmp/sdf_loader_test.log 2>&1 && \
  (echo "PASS: SDF serial-manipulator loader test"; tail -1 /tmp/sdf_loader_test.log) || \
  (echo "FAIL: SDF serial-manipulator loader test"; cat /tmp/sdf_loader_test.log; exit 1)

# Verify the sdf2manipulator CLI emits the expected YAML schema.
CLI_BIN="$(ros2 pkg prefix sas_robot_driver_gazebo)/lib/sas_robot_driver_gazebo/sdf2manipulator"
"$CLI_BIN" "$R820_SDF" > /tmp/sdf2manipulator.out 2>&1 && \
  grep -q "actuation_types:" /tmp/sdf2manipulator.out && \
  echo "PASS: sdf2manipulator CLI output schema" || \
  (echo "FAIL: sdf2manipulator CLI"; cat /tmp/sdf2manipulator.out; exit 1)

ros2 run sas_robot_driver_gazebo gazebo_service_frequency_checker
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo object_server_launch.py &
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo simulator_server_launch.py &
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo robot_driver_server_launch.py &
cd sdf
timeout --signal SIGINT 20 gz sim -s ./r820_world.sdf
