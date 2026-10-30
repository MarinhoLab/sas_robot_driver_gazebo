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
@file object_server_launch.py
@brief Object server launch file.

Launches the Gazebo object server node, which synchronizes the poses of the
specified world entities with ROS topics.
"""

import os.path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    """
    @brief Create the object server launch description.

    Starts the Gazebo object server node with the parameters from a YAML
    configuration file. Pass a different file with ``config_file:=``.

    @return launch.LaunchDescription Launch description for the object server node.
    """
    name = LaunchConfiguration('name')
    config_file = LaunchConfiguration('config_file')

    return LaunchDescription([
        DeclareLaunchArgument(
            'name',
            default_value='sas_object_server_gazebo_node'
        ),
        DeclareLaunchArgument(
            'config_file',
            default_value=os.path.join(get_package_share_directory('sas_robot_driver_gazebo'), 'config', 'config.yaml')
        ),
        Node(
            package='sas_robot_driver_gazebo',
            executable='sas_object_server_gazebo_node',
            name=name,
            parameters=[config_file]
        )
    ])
