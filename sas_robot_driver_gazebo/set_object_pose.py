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
from urllib import request

from gz.transport13 import Node
from gz.msgs10.pose_pb2 import Pose
from gz.msgs10.boolean_pb2 import Boolean

def response_callback(response, result):
    print(f"{result}, {response}")

def main():
    # create a transport node
    node = Node()
    service_name = "/world/ur3e_position_world/set_pose"
    request = Pose()
    request.name = "frame_x"
    request.position.x = 0.0
    request.position.y = 0.0
    request.position.z = 2.0

    response = None

    while response is None or not response.data:
        result, response = node.request(service_name, request)
        print(f"{result}, {response}")


if __name__ == "__main__":
    main()
