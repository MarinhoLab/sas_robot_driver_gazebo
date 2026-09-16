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

# Verify the serial-manipulator-SDF loader builds a correct kinematic model from
# each bundled robot SDF. Every joint acts about +z (RZ); fkm(q) is cross-checked
# against an independent 4x4 matrix chain. Models are paired with their expected
# joint count ("model_sdf:expected_joints").
TEST_BIN="/root/sas_robot_driver_gazebo_devel/build/sas_robot_driver_gazebo/test_sdf_serial_manipulator_loader"
CLI_BIN="$(ros2 pkg prefix sas_robot_driver_gazebo)/lib/sas_robot_driver_gazebo/sdf2manipulator"
SDF_SHARE="$(ros2 pkg prefix sas_robot_driver_gazebo --share)/sdf"
SDF_MODELS=("r820.sdf:7" "ur3e.sdf:6")

for entry in "${SDF_MODELS[@]}"; do
  model="${entry%%:*}"
  expected_joints="${entry##*:}"
  sdf_path="$SDF_SHARE/$model"
  test_log="/tmp/sdf_loader_test_${model}.log"
  cli_out="/tmp/sdf2manipulator_${model}.out"

  "$TEST_BIN" "$sdf_path" "$expected_joints" > "$test_log" 2>&1 && \
    (echo "PASS: SDF serial-manipulator loader test ($model, $expected_joints joints)"; tail -1 "$test_log") || \
    (echo "FAIL: SDF serial-manipulator loader test ($model)"; cat "$test_log"; exit 1)

  # Verify the sdf2manipulator CLI emits the expected YAML schema for the model.
  "$CLI_BIN" "$sdf_path" > "$cli_out" 2>&1 && \
    grep -q "actuation_types:" "$cli_out" && \
    echo "PASS: sdf2manipulator CLI output schema ($model)" || \
    (echo "FAIL: sdf2manipulator CLI ($model)"; cat "$cli_out"; exit 1)
done

ros2 run sas_robot_driver_gazebo gazebo_service_frequency_checker
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo object_server_launch.py &
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo simulator_server_launch.py &
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo robot_driver_server_launch.py &
cd sdf
timeout --signal SIGINT 20 gz sim -s ./r820_world.sdf
