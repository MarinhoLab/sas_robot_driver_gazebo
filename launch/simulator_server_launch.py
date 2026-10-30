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
@file simulator_server_launch.py
@brief Simulator server launch file.

Launches the Gazebo simulator server node, which binds Gazebo world control
services (start/stop/step) to ROS.
"""

import os.path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    """
    @brief Create the simulator server launch description.

    Starts the Gazebo simulator server node with the parameters from a YAML
    configuration file. Pass a different file with ``config_file:=``. The
    simulation is autostarted by default (``autostart: true`` in the
    configuration file); to disable it, provide a configuration file with
    ``autostart: false``.

    @return launch.LaunchDescription Launch description for the simulator server node.
    """
    name = LaunchConfiguration('name')
    config_file = LaunchConfiguration('config_file')

    return LaunchDescription([
        DeclareLaunchArgument(
            'name',
            default_value='sas_simulator_server_gazebo_node'
        ),
        DeclareLaunchArgument(
            'config_file',
            default_value=os.path.join(get_package_share_directory('sas_robot_driver_gazebo'), 'config', 'config.yaml')
        ),
        Node(
            package='sas_robot_driver_gazebo',
            executable='sas_simulator_server_gazebo_node',
            name=name,
            parameters=[config_file]
        )
    ])
