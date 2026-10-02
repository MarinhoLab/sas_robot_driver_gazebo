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
#   Claude (Anthropic), C++ port of sas_robot_driver_ros_gazebo.py
# ################################################################
*/

/**
 * @file sas_robot_driver_ros_gazebo_node.cpp
 * @brief Gazebo robot driver bridge, in C++.
 *
 * The C++ counterpart of scripts/sas_robot_driver_ros_gazebo.py, with the
 * same parameters: a RobotDriverROS backed by the C++ RobotDriverGazebo.
 */

#include <atomic>
#include <csignal>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <sas_common/sas_common.hpp>
#include <sas_core/sas_shutdown_signaler.hpp>
#include <sas_robot_driver/sas_robot_driver_ros.hpp>
#include <sas_robot_driver_gazebo/sas_robot_driver_gazebo.hpp>

static std::atomic_bool kill_this_process(false);
void sig_int_handler(int)
{
    kill_this_process = true;
}

int main(int argc, char** argv)
{
    if(signal(SIGINT, sig_int_handler) == SIG_ERR)
        throw std::runtime_error("::Error setting the signal int handler.");

    rclcpp::init(argc, argv, rclcpp::InitOptions(), rclcpp::SignalHandlerOptions::None);
    auto node = std::make_shared<rclcpp::Node>("sas_robot_driver_ros_gazebo");

    try
    {
        RCLCPP_INFO_STREAM_ONCE(node->get_logger(), "::Loading parameters from parameter server.");
        sas::RobotDriverGazeboConfiguration configuration;
        sas::get_ros_parameter(node, "joint_names", configuration.joint_names);
        sas::get_ros_parameter(node, "joint_positions_topic_prefix", configuration.joint_positions_topic_prefix);
        sas::get_ros_parameter(node, "joint_states_topic", configuration.joint_states_topic);

        sas::RobotDriverROSConfiguration robot_driver_ros_configuration;
        sas::get_ros_parameter(node, "robot_name", robot_driver_ros_configuration.robot_driver_provider_prefix);
        sas::get_ros_optional_parameter(node, "thread_sampling_time_sec",
                                        robot_driver_ros_configuration.thread_sampling_time_sec, 0.002);
        RCLCPP_INFO_STREAM_ONCE(node->get_logger(), "::Parameters OK.");

        auto shutdown_signaler = std::make_shared<sas::ShutdownSignaler>(&kill_this_process);
        auto robot_driver_gazebo = std::make_shared<sas::RobotDriverGazebo>(shutdown_signaler, configuration);
        sas::RobotDriverROS robot_driver_ros(node,
                                             robot_driver_gazebo,
                                             robot_driver_ros_configuration,
                                             shutdown_signaler);
        robot_driver_ros.control_loop();
    }
    catch(const std::exception& e)
    {
        RCLCPP_ERROR_STREAM_ONCE(node->get_logger(), std::string("::Exception::") + e.what());
    }

    rclcpp::shutdown();
    return 0;
}
