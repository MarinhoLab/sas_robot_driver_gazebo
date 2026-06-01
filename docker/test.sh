#!/bin/bash

sudo apt-get install tree
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get remove -y ros-jazzy-sas-robot-driver-gazebo
sudo apt-get install -y xvfb
sudo /usr/bin/Xvfb :99 -screen 0 1280x1024x24 &

cd /root/sas_robot_driver_gazebo_devel
ls .
colcon build
source install/setup.bash

ros2 run sas_robot_driver_gazebo gazebo_service_frequency_checker
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo server_launch.py &
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo robot_driver_server_launch.py &
cd src/sas_robot_driver_gazebo/sdf
timeout --signal SIGINT 20 gz sim -s ./ur3e_world.sdf
