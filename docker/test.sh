#!/bin/bash

sudo apt-get install tree
sudo apt-get update && sudo apt-get upgrade -y

cd /root/sas_robot_driver_gazebo_devel
ls .
colcon build
source install/setup.bash

#ros2 run sas_robot_driver_gazebo sas_robot_driver_ros_gazebo.py
ros2 run sas_robot_driver_gazebo gazebo_service_frequency_checker
#ros2 run sas_robot_driver_gazebo sas_object_server_gazebo_node
#ros2 run sas_robot_driver_gazebo sas_simulator_server_gazebo_node
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo server_launch.py
timeout --signal SIGINT 20 ros2 launch sas_robot_driver_gazebo robot_driver_server_launch.py
