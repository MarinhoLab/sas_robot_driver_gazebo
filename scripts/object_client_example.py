#!/usr/bin/python3
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
#   Based on `joint_interface_example.py` from `sas_ur_control_template`
#
# #######################################################################################

@file object_client_example.py
@brief Object pose example script.

Moves a Gazebo object through ObjectClient commands.
"""

import time

from math import sin, cos, pi

import numpy
from dqrobotics import *  # Despite what PyCharm might say, this is very much necessary or DQs will not be recognized
from dqrobotics.utils.DQ_Math import deg2rad

import rclpy
from rclpy.node import Node

from sas_common import (rclcpp_init, 
                        rclcpp_Node, 
                        rclcpp_spin_some, 
                        rclcpp_shutdown,
                        ObjectClient)

from sas_core import Clock, Statistics


def main(args=None):
    """
    @brief Run the object pose example.

    Initializes ROS interfaces, waits for the object client to become enabled,
    and sends target poses in a loop.

    @param args Optional ROS command-line arguments.
    @return None
    """
    try:
        rclpy.init(args=args)
        rospy_node = Node('sas_robot_driver_gazebo_object_client_example_node_py')

        rclcpp_init()
        node = rclcpp_Node("sas_robot_driver_gazebo_object_client_example_node_cpp")

        rospy_node.declare_parameter('object_name', 'frame_x')
        object_name = rospy_node.get_parameter('object_name').get_parameter_value().string_value

        sampling_time = 0.001
        clock = Clock(sampling_time)
        clock.init()

        oc = ObjectClient(node, object_name)

        while not oc.is_enabled():
            rclcpp_spin_some(node)
            time.sleep(0.1)

        print(f"topic prefix = {oc.get_topic_prefix()}")

        x = oc.get_pose()

        # For some iterations. Note that this can be stopped with CTRL+C.
        for i in range(0, 5000):
            clock.update_and_sleep()

            A = 1.0
            w = 1.0

            tx = A * sin(2*pi*w*i*sampling_time)
            ty = A * cos(2*pi*w*i*sampling_time)

            xd = x * (1 + 0.5 * E_ * (i_ * tx + j_ * ty))

            oc.send_pose(xd)

            rclcpp_spin_some(node)

        # Statistics
        print("Statistics for the entire loop")
        print("  Mean computation time: {}".format(clock.get_statistics(
            Statistics.Mean, Clock.TimeType.Computational)
        ))
        print("  Mean idle time: {}".format(clock.get_statistics(
            Statistics.Mean, Clock.TimeType.Idle)
        ))
        print("  Mean effective thread sampling time: {}".format(clock.get_statistics(
            Statistics.Mean, Clock.TimeType.EffectiveSampling)
        ))

        rclcpp_shutdown()

    except KeyboardInterrupt:
        print("Interrupted by user")
    except Exception as e:
        print("Unhandled excepts", e)


if __name__ == '__main__':
    main()