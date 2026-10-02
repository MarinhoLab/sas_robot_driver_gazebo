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
