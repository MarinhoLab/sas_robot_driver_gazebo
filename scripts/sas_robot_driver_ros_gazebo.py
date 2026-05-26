#!/bin/python3
"""
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
#
# #######################################################################################
#
#   Author: Murilo M. Marinho, email: murilomarinho@ieee.org
#
# #######################################################################################
"""
import rclpy
from rclpy.node import Node
from sas_core import ShutdownSignaler
from sas_common import rclcpp_init, rclcpp_Node, rclcpp_spin_some, rclcpp_shutdown
from sas_robot_driver import RobotDriverROS, RobotDriverROSConfiguration
from sas_robot_driver_gazebo import RobotDriverGazebo, RobotDriverGazeboConfiguration

def main():

    rclcpp_init()
    roscpp_node = rclcpp_Node("sas_robot_driver_ros_gazebo_cpp")
    rclpy.init()
    rospy_node = Node('sas_robot_driver_ros_gazebo_py')
    try:

        rospy_node.declare_parameter('joint_names')
        joint_names = rospy_node.get_parameter('joint_names').get_parameter_value().string_array_value

        rospy_node.declare_parameter('joint_positions_topic_prefix')
        joint_positions_topic_prefix = rospy_node.get_parameter('joint_positions_topic_prefix').get_parameter_value().string_value

        rospy_node.declare_parameter('joint_states_topic')
        joint_states_topic = rospy_node.get_parameter('joint_states_topic').get_parameter_value().string_value

        rospy_node.declare_parameter('robot_name')
        robot_name = rospy_node.get_parameter('robot_name').get_parameter_value().string_value

        rospy_node.declare_parameter('thread_sampling_time_sec', 0.002)
        sampling_time = rospy_node.get_parameter('thread_sampling_time_sec').get_parameter_value().double_value

        ss = ShutdownSignaler()
        gazebo_cfg = RobotDriverGazeboConfiguration()
        gazebo_cfg.joint_names = joint_names

        gazebo_cfg.joint_positions_topic_prefix = joint_positions_topic_prefix#"/model/ur3e_1/joint/"
        gazebo_cfg.joint_states_topic = joint_states_topic#"/world/ur3e_position_world/model/ur3e_1/model/ur3e_1_position_controller/model/ur3e_1/joint_state"

        srdg = RobotDriverGazebo(ss, gazebo_cfg)

        rdrg_cfg = RobotDriverROSConfiguration()
        rdrg_cfg.robot_driver_provider_prefix = robot_name
        rdrg_cfg.thread_sampling_time_sec = sampling_time

        sas_robot_driver_ros = RobotDriverROS(roscpp_node,
                                              srdg,
                                              rdrg_cfg,
                                              ss
                                              )
        sas_robot_driver_ros.control_loop()
    except KeyboardInterrupt:
        pass
    except ...:
        pass

if __name__ == '__main__':
    main()