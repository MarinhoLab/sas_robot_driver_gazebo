/*
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
# ################################################################
#
#   Author: Murilo M. Marinho, email: murilomarinho@ieee.org
#
# ################################################################
# Contributors:
#   ---
#   Claude (Anthropic), pybind11 bindings of the C++ RobotDriverGazebo
# ################################################################
*/

/**
 * @file sas_robot_driver_gazebo_py.cpp
 * @brief pybind11 bindings: sas_robot_driver_gazebo._sas_robot_driver_gazebo.
 *
 * Exposes the C++ RobotDriverGazebo and its configuration to Python, as a
 * subclass of marinholab.sas.core.RobotDriver that
 * sas_robot_driver.RobotDriverROS accepts. The package re-exports both
 * (sas_robot_driver_gazebo.RobotDriverGazebo, ...Configuration); they replace
 * the former pure-Python driver, with the same constructor, methods and
 * `configuration` attribute (the getters return NumPy arrays).
 */

#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

#include <sas_robot_driver_gazebo/sas_robot_driver_gazebo.hpp>

namespace py = pybind11;
using sas::RobotDriverGazebo;
using sas::RobotDriverGazeboConfiguration;

/**
 * @brief The Python module sas_robot_driver_gazebo._sas_robot_driver_gazebo.
 *
 * Binds RobotDriverGazeboConfiguration and RobotDriverGazebo; the getters,
 * set_target_joint_positions and initialize release the Python interpreter
 * lock while they run in C++.
 */
PYBIND11_MODULE(_sas_robot_driver_gazebo, m)
{
    m.doc() = "The C++ RobotDriverGazebo of sas_robot_driver_gazebo and its configuration.";

    // The base class RobotDriver and ShutdownSignaler are registered there.
    py::module_::import("marinholab.sas.core");

    py::class_<RobotDriverGazeboConfiguration>(m, "RobotDriverGazeboConfiguration",
                                               "Configuration of RobotDriverGazebo: the joints and the Gazebo topics.")
        .def(py::init<>())
        .def_readwrite("joint_names", &RobotDriverGazeboConfiguration::joint_names,
                       "Names of the Gazebo joints to control, in the driver's order.")
        .def_readwrite("joint_positions_topic_prefix", &RobotDriverGazeboConfiguration::joint_positions_topic_prefix,
                       "Prefix of the joints' command topics: prefix + joint name + '/0/cmd_pos'.")
        .def_readwrite("joint_states_topic", &RobotDriverGazeboConfiguration::joint_states_topic,
                       "Gazebo topic of the joint states (gz.msgs.Model, from a JointStatePublisher).");

    // py::smart_holder, as marinholab.sas.core binds RobotDriver: a std::shared_ptr
    // holder is refused by RobotDriverROS ("Non-owning holder (load_as_shared_ptr)").
    py::class_<RobotDriverGazebo, marinholab::sas::core::RobotDriver, py::smart_holder>(
                m, "RobotDriverGazebo", "Gazebo implementation of RobotDriver (C++).")
        .def(py::init<const std::shared_ptr<marinholab::sas::core::ShutdownSignaler>&,
                      const RobotDriverGazeboConfiguration&>(),
             py::arg("shutdown_signaler"), py::arg("configuration"),
             "Construct a Gazebo robot driver from a ShutdownSignaler and a RobotDriverGazeboConfiguration.")
        .def("get_joint_positions", &RobotDriverGazebo::get_joint_positions, py::call_guard<py::gil_scoped_release>(),
             "The latest joint positions from Gazebo [rad], in configuration order.")
        .def("set_target_joint_positions", &RobotDriverGazebo::set_target_joint_positions,
             py::arg("target_joint_positions_rad"), py::call_guard<py::gil_scoped_release>(),
             "Publish target joint positions [rad] to Gazebo's joint position controllers.")
        .def("get_joint_velocities", &RobotDriverGazebo::get_joint_velocities, py::call_guard<py::gil_scoped_release>(),
             "The latest joint velocities from Gazebo [rad/s], in configuration order.")
        .def("get_joint_torques", &RobotDriverGazebo::get_joint_torques, py::call_guard<py::gil_scoped_release>(),
             "The latest joint torques from Gazebo [N m], in configuration order.")
        .def_property_readonly("configuration", &RobotDriverGazebo::get_configuration,
                               "The configuration this driver was constructed with.")
        .def("connect", &RobotDriverGazebo::connect,
             "Advertise the joints' command topics and subscribe to the joint-state topic.")
        .def("disconnect", &RobotDriverGazebo::disconnect, "Unsubscribe from the joint-state topic.")
        .def("initialize", &RobotDriverGazebo::initialize, py::call_guard<py::gil_scoped_release>(),
             "Wait for every joint's state and limits from Gazebo, then set the joint limits.")
        .def("deinitialize", &RobotDriverGazebo::deinitialize, "Nothing to undo in Gazebo.");
}
