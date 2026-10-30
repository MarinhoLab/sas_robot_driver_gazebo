# Copyright (c) 2012-2026 Murilo Marques Marinho
#
#    This file is part of sas_robot_driver_gazebo.
#
#    sas_robot_driver_gazebo is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Lesser General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    sas_robot_driver_gazebo is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public License
#    along with sas_robot_driver_gazebo.  If not, see <https://www.gnu.org/licenses/>.

"""
@file robot_driver_server_launch.py
@brief Robot driver launch file.

Launches the Gazebo robot driver ROS bridge node.
"""

import os.path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import LaunchConfigurationEquals
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    """
    @brief Create the robot driver launch description.

    Starts the ROS-to-Gazebo robot driver bridge with the parameters from a
    YAML configuration file. Pass a different file with ``config_file:=``.

    @return launch.LaunchDescription Launch description for the robot driver node.
    """
    name = LaunchConfiguration('name')
    config_file = LaunchConfiguration('config_file')

    return LaunchDescription([
        DeclareLaunchArgument(
            'name',
            default_value='ur3e_1'
        ),
        DeclareLaunchArgument(
            'config_file',
            default_value=os.path.join(get_package_share_directory('sas_robot_driver_gazebo'), 'config', 'config.yaml')
        ),
        # 'cpp' (default): the C++ bridge, sas_robot_driver_ros_gazebo_node.
        # 'python': the Python script sas_robot_driver_ros_gazebo.py, with
        # the same (C++) RobotDriverGazebo through its Python binding.
        DeclareLaunchArgument(
            'implementation',
            default_value='cpp',
            choices=['cpp', 'python']
        ),
        Node(
            package='sas_robot_driver_gazebo',
            executable='sas_robot_driver_ros_gazebo_node',
            name=name,
            parameters=[config_file],
            condition=LaunchConfigurationEquals('implementation', 'cpp')
        ),
        Node(
            package='sas_robot_driver_gazebo',
            executable='sas_robot_driver_ros_gazebo.py',
            name=name,
            parameters=[config_file],
            condition=LaunchConfigurationEquals('implementation', 'python')
        )
    ])
