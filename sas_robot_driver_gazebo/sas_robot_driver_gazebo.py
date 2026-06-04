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

"""
@file sas_robot_driver_gazebo.py
@brief Gazebo robot driver classes.

Provides the configuration class and RobotDriver implementation used to
exchange joint commands and joint states with Gazebo.
"""

import time
import numpy as np

from sas_core import RobotDriver, ShutdownSignaler

from gz.transport13 import Node
from gz.msgs10.model_pb2 import Model
from gz.msgs10.double_pb2 import Double

class RobotDriverGazeboConfiguration:
    """
    @class RobotDriverGazeboConfiguration
    @brief Configuration for RobotDriverGazebo.

    Stores the joint command topic prefix, controlled joint names, and the
    Gazebo topic used for joint state messages.
    """

    def __init__(self):
        self.joint_positions_topic_prefix: str|None = None
        self.joint_names: list[str]|None = None
        self.joint_states_topic: str|None = None

class RobotDriverGazebo(RobotDriver):
    """
    @class RobotDriverGazebo
    @brief Gazebo implementation of RobotDriver.

    Publishes joint position commands to Gazebo and receives joint states,
    velocities, torques, and limits from Gazebo model messages.
    """

    def __init__(self,
                 ss: ShutdownSignaler,
                 configuration: RobotDriverGazeboConfiguration):
        """
        @brief Construct a Gazebo robot driver.

        @param ss ShutdownSignaler instance used by the base RobotDriver.
        @param configuration Configuration values for Gazebo topics and joints.
        """
        RobotDriver.__init__(self, ss)

        #: Configuration used by this driver instance.
        self.configuration: RobotDriverGazeboConfiguration = configuration
        #: Number of controlled joints.
        self.DOF: int = len(self.configuration.joint_names)
        #: Suffix appended to each joint command topic.
        self.joint_positions_topic_postfix: str = "/0/cmd_pos"
        #: Gazebo publishers used to send joint commands.
        self.joint_publishers = []
        #: Cached joint positions.
        self.joint_positions = [None] * self.DOF
        #: Cached joint velocities.
        self.joint_velocities = [None] * self.DOF
        #: Cached joint torques.
        self.joint_torques = [None] * self.DOF
        #: Cached lower joint limits.
        self.limit_lower = [None] * self.DOF
        #: Cached upper joint limits.
        self.limit_upper = [None] * self.DOF
        #: Gazebo transport node used by the driver.
        self.node = None

    def connect(self):
        """
        @brief Connect to Gazebo topics.

        Creates Gazebo publishers for joint commands and subscribes to the
        configured joint state topic.

        @return None
        @exception RuntimeError Raised when the joint state subscription fails.
        """
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
        """
        @brief Disconnect from Gazebo.
        @return None
        """
        pass

    def initialize(self):
        """
        @brief Wait for joint states and limits.

        Blocks until joint positions and joint limits have been received from
        Gazebo, then stores the limits in the base RobotDriver.

        @return None
        """
        while (None in self.joint_positions) or (True in np.isnan(self.joint_positions)):
            print(f"Waiting for valid joint position messages from Gazebo for {self.configuration.joint_positions_topic_prefix}...")
            time.sleep(0.1)
        while (None in self.limit_lower) or (True in np.isnan(self.limit_lower)):
            time.sleep(0.1)
        while (None in self.limit_upper) or (True in np.isnan(self.limit_upper)):
            time.sleep(0.1)
        self.set_joint_limits((self.limit_lower, self.limit_upper))
        print(f"get_joint_limits({self.get_joint_limits()})")

    def deinitialize(self):
        """
        @brief Deinitialize the driver.
        @return None
        """
        pass

    def get_joint_positions(self):
        """
        @brief Return the current joint positions.

        @return list Current joint positions in radians.
        @exception RuntimeError Raised when joint positions are unavailable.
        """
        if None in self.joint_positions:
            raise RuntimeError("Joint positions not initialized")
        return self.joint_positions

    def get_joint_velocities(self):
        """
        @brief Return the current joint velocities.

        @return list Current joint velocities.
        @exception RuntimeError Raised when joint velocities are unavailable.
        """
        if None in self.joint_velocities:
            raise RuntimeError("Joint velocities not initialized")
        return self.joint_velocities

    def get_joint_torques(self):
        """
        @brief Return the current joint torques.

        @return list Current joint torques.
        @exception RuntimeError Raised when joint torques are unavailable.
        """
        if None in self.joint_torques:
            raise RuntimeError("Joint torques not initialized")
        return self.joint_torques

    def set_target_joint_positions(self, target_joint_positions_rad):
        """
        @brief Publish target joint positions.

        @param target_joint_positions_rad Sequence of target joint positions in radians.
        @return None
        """
        for i in range(len(self.joint_publishers)):
            double_msg = Double()
            double_msg.data = target_joint_positions_rad[i]
            self.joint_publishers[i].publish(double_msg)

    def joint_states_callback(self, msg):
        """
        @brief Update cached joint state values.

        Reads positions, velocities, torques, and limits for configured joints
        from a Gazebo model message.

        @param msg Gazebo model message containing joint state data.
        @return None
        """
        joints = msg.joint
        for joint in joints:
            for i in range(self.DOF):
                if joint.name == self.configuration.joint_names[i]:
                    self.joint_positions[i] = joint.axis1.position
                    self.joint_velocities[i] = joint.axis1.velocity
                    self.joint_torques[i] = joint.axis1.force
                    self.limit_lower[i] = joint.axis1.limit_lower
                    self.limit_upper[i] = joint.axis1.limit_upper
