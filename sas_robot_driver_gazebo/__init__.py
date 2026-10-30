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

"""
@file __init__.py
@brief Package entry for Gazebo drivers.

Re-exports the Gazebo robot driver classes, implemented in C++
(sas_robot_driver_gazebo.hpp) and bound with pybind11
(_sas_robot_driver_gazebo): RobotDriverGazebo is a
marinholab.sas.core.RobotDriver that sas_robot_driver.RobotDriverROS accepts.
"""

from sas_robot_driver_gazebo._sas_robot_driver_gazebo import (
    RobotDriverGazebo,
    RobotDriverGazeboConfiguration,
)

## The names this package exports.
__all__ = ["RobotDriverGazebo", "RobotDriverGazeboConfiguration"]
