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
    configuration file. Pass a different file with ``config_file:=``.
    The ``autostart`` launch argument overrides the value from the
    configuration file (e.g. ``autostart:=true``).

    @return launch.LaunchDescription Launch description for the simulator server node.
    """
    name = LaunchConfiguration('name')
    config_file = LaunchConfiguration('config_file')
    autostart = LaunchConfiguration('autostart')

    return LaunchDescription([
        DeclareLaunchArgument(
            'name',
            default_value='sas_simulator_server_gazebo_node'
        ),
        DeclareLaunchArgument(
            'config_file',
            default_value=os.path.join(get_package_share_directory('sas_robot_driver_gazebo'), 'config', 'config.yaml')
        ),
        DeclareLaunchArgument(
            'autostart',
            default_value='false'
        ),
        Node(
            package='sas_robot_driver_gazebo',
            executable='sas_simulator_server_gazebo_node',
            name=name,
            parameters=[config_file, {'autostart': autostart}]
        )
    ])
