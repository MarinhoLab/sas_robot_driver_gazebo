"""
# Copyright (c) 2026 Murilo Marques Marinho
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
from gz.transport13 import Node
from gz.msgs10.model_pb2 import Model
from gz.msgs10.double_pb2 import Double

import time

joint_positions = []

def joint_state_cb(msg: Model):
    global joint_positions
    joints = msg.joint

    joint_positions = [
        joints[1].axis1.position,
        joints[2].axis1.position,
        joints[3].axis1.position,
        joints[4].axis1.position,
        joints[5].axis1.position,
        joints[6].axis1.position
    ]
    print(f'{joint_positions}')

def main():
    # create a transport node
    node = Node()
    topic = "/world/default/model/ur30_1/joint_state"

    command_topic_prefix = "/model/ur30_1/joint/"
    command_topic_postfix = "/0/cmd_pos"
    joint_names = [
        "elbow_joint",
        "shoulder_lift_joint",
        "shoulder_pan_joint",
        "wrist_1_joint",
        "wrist_2_joint",
        "wrist_3_joint"
    ]
    joint_publishers = []
    for joint_name in joint_names:
        joint_publishers.append(node.advertise(f""
                                               f"{command_topic_prefix}"
                                               f"{joint_name}"
                                               f"{command_topic_postfix}", Double))

    if node.subscribe(Model, topic, joint_state_cb):
        print("Subscribing to type {} on topic [{}]".format(
            Model, topic))
    else:
        print("Error subscribing to topic [{}]".format(topic))
        return

    # wait for shutdown
    try:
        while True:
            time.sleep(0.001)
            for i in range(len(joint_publishers)):
                double_msg = Double()
                double_msg.data = 1.0
                joint_publishers[i].publish(double_msg)
    except KeyboardInterrupt:
        pass
    print("Done")

if __name__ == "__main__":
    main()
