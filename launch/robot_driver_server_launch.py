"""
@file robot_driver_server_launch.py
@brief Robot driver launch file.

Launches the Gazebo robot driver ROS bridge node.
"""

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    """
    @brief Create the robot driver launch description.

    Declares launch arguments for Gazebo joint command and state topics and
    starts the ROS-to-Gazebo robot driver bridge.

    @return launch.LaunchDescription Launch description for the robot driver node.
    """
    joint_names = LaunchConfiguration('joint_names')
    joint_positions_topic_prefix = LaunchConfiguration('joint_positions_topic_prefix')
    joint_states_topic = LaunchConfiguration('joint_states_topic')
    robot_name = LaunchConfiguration('robot_name')

    return LaunchDescription([
        DeclareLaunchArgument(
            'joint_names',
            default_value='''["shoulder_pan_joint","shoulder_lift_joint","elbow_joint","wrist_1_joint","wrist_2_joint","wrist_3_joint"]'''
        ),
        DeclareLaunchArgument(
            'joint_positions_topic_prefix',
            default_value="/model/ur3e/joint/"
        ),
        DeclareLaunchArgument(
            'joint_states_topic',
            default_value="/world/ur3e_world/model/ur3e/model/ur3e_position_controller/model/ur3e/joint_state"
        ),
        DeclareLaunchArgument(
            'robot_name',
            default_value="ur3e_1"
        ),
        Node(
            package='sas_robot_driver_gazebo',
            executable='sas_robot_driver_ros_gazebo.py',
            parameters=[{
                "joint_names": joint_names,
                "joint_positions_topic_prefix": joint_positions_topic_prefix,
                "joint_states_topic": joint_states_topic,
                "robot_name": robot_name
            }]
        )
    ])
