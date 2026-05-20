# Copyright (c) 2016-2026 Murilo Marques Marinho
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
# ################################################################
#
#   Author: Murilo M. Marinho, email: murilomarinho@ieee.org
#
# ################################################################
from sas_core import RobotDriver, ShutdownSignaler

from gz.transport13 import Node
from gz.msgs10.model_pb2 import Model
from gz.msgs10.double_pb2 import Double

class RobotDriverGazeboConfiguration:
    def __init__(self):
        self.joint_positions_topic_prefix: str|None = None
        self.joint_names: list[str]|None = None
        self.joint_states_topic: str|None = None

class RobotDriverGazebo(RobotDriver):

    def __init__(self,
                 ss: ShutdownSignaler,
                 configuration: RobotDriverGazeboConfiguration):
        RobotDriver.__init__(self, ss)

        self.configuration: RobotDriverGazeboConfiguration = configuration
        self.DOF: int = len(self.configuration.joint_names)
        self.joint_positions_topic_postfix: str = "/0/cmd_pos"
        self.joint_publishers = []
        self.joint_positions = [None] * self.DOF
        self.node = None

    def connect(self):
        self.node = Node()
        for joint_name in self.configuration.joint_names:
            self.joint_publishers.append(self.node.advertise(f""
                                                   f"{self.configuration.joint_positions_topic_prefix}"
                                                   f"{joint_name}"
                                                   f"{self.joint_positions_topic_postfix}", Double))
        if not self.node.subscribe(
                Model,
                self.configuration.joint_states_topic,
                self.joint_states_callback):
            raise RuntimeError(f"Failed to subscribe to topic [{self.joint_states_topic}]")

    def disconnect(self):
        pass

    def initialize(self):
        pass

    def deinitialize(self):
        pass

    def get_joint_positions(self):
        if self.joint_positions is None:
            raise RuntimeError("Joint positions not initialized")
        return self.joint_positions

    def set_target_joint_positions(self, target_joint_positions_rad):
        for i in range(len(self.joint_publishers)):
            double_msg = Double()
            double_msg.data = target_joint_positions_rad[i]
            self.joint_publishers[i].publish(double_msg)

    def joint_states_callback(self, msg: Model):
        joints = msg.joint
        for i in range(self.DOF):
            self.joint_positions[i] = joints[i].axis1.position