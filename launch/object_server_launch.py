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
