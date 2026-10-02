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
#   Contributor: Claude (Anthropic)
# #######################################################################################

@file bridge_lag_monitor.py
@brief How late a robot driver bridge reports Gazebo's joint positions.

Samples, at 50 Hz, the joint positions Gazebo publishes (gz-transport, the
JointStatePublisher topic) and those the bridge reports
(<robot_name>/get/joint_states), then prints the delay that best aligns them
and the largest gap. The Gazebo subscription is throttled and the ROS spin
waits, so that this monitor does not fall behind itself.

    ros2 run sas_robot_driver_gazebo bridge_lag_monitor.py \
        /world/ur3e_world/model/ur3e/model/ur3e_position_controller/model/ur3e/joint_state \
        /ur3e_1/get/joint_states 45 shoulder_pan_joint shoulder_lift_joint elbow_joint \
        wrist_1_joint wrist_2_joint wrist_3_joint
"""

import sys
import time

import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile
from sensor_msgs.msg import JointState
from gz.transport13 import Node as GzNode, SubscribeOptions
from gz.msgs10.model_pb2 import Model


def main():
    """
    @brief Sample both sides and print the delay and the largest gap.
    @return None
    """
    gz_topic, ros_topic, seconds, names = sys.argv[1], sys.argv[2], float(sys.argv[3]), sys.argv[4:]
    gazebo = np.full(len(names), np.nan)
    reported = {"q": np.full(len(names), np.nan)}

    def on_gazebo(msg, *_):
        for joint in msg.joint:
            if joint.name in names:
                gazebo[names.index(joint.name)] = joint.axis1.position

    options = SubscribeOptions()
    options.msgs_per_sec = 50
    gz_node = GzNode()
    gz_node.subscribe(Model, gz_topic, on_gazebo, options)
    rclpy.init()
    node = Node("bridge_lag_monitor")
    node.create_subscription(JointState, ros_topic, lambda m: reported.__setitem__("q", np.array(m.position)),
                             QoSProfile(depth=1))
    rows = []
    start = time.monotonic()
    while time.monotonic() - start < seconds:
        rclpy.spin_once(node, timeout_sec=0.005)
        if not rows or time.monotonic() - start - rows[-1][0] >= 0.02:
            rows.append([time.monotonic() - start, *gazebo, *reported["q"]])
    rows = np.array(rows)
    t, g, r = rows[:, 0], rows[:, 1:1 + len(names)], rows[:, 1 + len(names):]
    ok = ~np.isnan(g).any(1) & ~np.isnan(r).any(1) & (t > min(10.0, seconds / 2))
    g, r = g[ok], r[ok]
    dt = float(np.median(np.diff(t)))
    errors = [np.abs(r[lag:] - g[:len(g) - lag]).max() for lag in range(0, min(1000, len(g) // 2))]
    lag = int(np.argmin(errors))
    print(f"reported {1e3 * lag * dt:.0f} ms behind Gazebo; largest gap {np.degrees(np.abs(r - g).max()):.2f} deg")


if __name__ == '__main__':
    main()
