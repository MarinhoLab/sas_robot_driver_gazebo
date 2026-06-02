from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    set_pose_service_name = LaunchConfiguration('set_pose_service_name')
    get_pose_topic_name = LaunchConfiguration('get_pose_topic_name')
    entity_names = LaunchConfiguration('entity_names')
    control_service_name = LaunchConfiguration('control_service_name')
    autostart = LaunchConfiguration('autostart')

    return LaunchDescription([
        DeclareLaunchArgument(
            'set_pose_service_name',
            default_value="/world/ur3e_world/set_pose"
        ),
        DeclareLaunchArgument(
            'get_pose_topic_name',
            default_value="/world/ur3e_world/pose/info"
        ),
        DeclareLaunchArgument(
            'entity_names',
            default_value="[frame_x, frame_xd]"
        ),
        DeclareLaunchArgument(
            'control_service_name',
            default_value="/world/ur3e_world/control"
        ),
        DeclareLaunchArgument(
            'autostart',
            default_value="false"
        ),
        Node(
            package='sas_robot_driver_gazebo',
            executable='sas_object_server_gazebo_node',
            parameters=[{
                "set_pose_service_name": set_pose_service_name,
                "get_pose_topic_name": get_pose_topic_name,
                "entity_names": entity_names
            }]
        ),
        Node(
            package='sas_robot_driver_gazebo',
            executable='sas_simulator_server_gazebo_node',
            parameters=[{
                "service_name": control_service_name,
                "autostart": autostart
            }]
        )
    ])
