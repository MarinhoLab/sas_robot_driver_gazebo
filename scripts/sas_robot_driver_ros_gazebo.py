#!/bin/python3
from sas_core import ShutdownSignaler
from sas_common import rclcpp_init, rclcpp_Node, rclcpp_spin_some, rclcpp_shutdown
from sas_robot_driver import RobotDriverROS, RobotDriverROSConfiguration
from sas_robot_driver_gazebo import RobotDriverGazebo, RobotDriverGazeboConfiguration

def main():

    rclcpp_init()
    roscpp_node = rclcpp_Node("sas_robot_driver_ros_gazebo_cpp")

    ss = ShutdownSignaler()
    gazebo_cfg = RobotDriverGazeboConfiguration()
    gazebo_cfg.joint_names = [
        "elbow_joint",
        "shoulder_lift_joint",
        "shoulder_pan_joint",
        "wrist_1_joint",
        "wrist_2_joint",
        "wrist_3_joint"
    ]
    gazebo_cfg.joint_positions_topic_prefix = "/model/ur30_1/joint/"
    gazebo_cfg.joint_states_topic = "/world/default/model/ur30_1/joint_state"

    srdg = RobotDriverGazebo(ss, gazebo_cfg)

    rdrg_cfg = RobotDriverROSConfiguration()
    rdrg_cfg.robot_driver_provider_prefix = "test"
    rdrg_cfg.thread_sampling_time_sec = 0.001

    sas_robot_driver_ros = RobotDriverROS(roscpp_node,
                                          srdg,
                                          rdrg_cfg,
                                          ss
                                          )
    sas_robot_driver_ros.control_loop()

if __name__ == '__main__':
    main()