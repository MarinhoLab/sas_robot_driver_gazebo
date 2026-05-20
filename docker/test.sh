#!/bin/bash

sudo apt-get install tree

cd /root/sas_robot_driver_gazebo_devel
ls .
colcon build
source install/setup.bash

