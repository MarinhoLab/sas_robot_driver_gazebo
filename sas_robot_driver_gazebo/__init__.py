"""
@file __init__.py
@brief Package entry for Gazebo drivers.

Re-exports the Gazebo robot driver classes, implemented in C++
(sas_robot_driver_gazebo.hpp) and bound with pybind11
(_sas_robot_driver_gazebo): RobotDriverGazebo is a
marinholab.sas.core.RobotDriver that sas_robot_driver.RobotDriverROS accepts.
"""

from ._sas_robot_driver_gazebo import RobotDriverGazebo, RobotDriverGazeboConfiguration

__all__ = ["RobotDriverGazebo", "RobotDriverGazeboConfiguration"]
