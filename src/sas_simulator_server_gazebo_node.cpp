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
*/

/**
 * @file sas_simulator_server_gazebo_node.cpp
 * @brief Gazebo simulator server node.
 *
 * Exposes Gazebo simulation control through ROS.
 */

#include <gz/msgs.hh>
#include <gz/transport.hh>
#include <rclcpp/rclcpp.hpp>
#include <sas_core/sas_clock.hpp>
#include <sas_core/sas_shutdown_signaler.hpp>
#include <sas_common/sas_common.hpp>
#include <sas_common/sas_simulator_server.hpp>

/*********************************************
 * SIGNAL HANDLER
 * *******************************************/
#include<signal.h>
static sas::ShutdownSignaler ss;
void sig_int_handler(int)
{
    ss.shutdown();
}

/**
 * @brief Receive Gazebo world control responses.
 */
void world_control_callback(const gz::msgs::Boolean&, const bool)
{
    //TODO add something here if needed
}

/**
 * @brief Request simulation start.
 *
 * Sends a Gazebo world control request with pause disabled.
 *
 * @param node Gazebo transport node used for the request.
 * @param service_name Gazebo world control service name.
 */
void start_simulation_callback(gz::transport::Node& node, const std::string& service_name)
{
    gz::msgs::WorldControl msg;
    msg.set_pause(false);
    node.Request(service_name,
        msg,
        &world_control_callback);
}

/**
 * @brief Request simulation stop.
 *
 * Sends a Gazebo world control request with pause enabled.
 *
 * @param node Gazebo transport node used for the request.
 * @param service_name Gazebo world control service name.
 */
void stop_simulation_callback(gz::transport::Node& node, const std::string& service_name)
{
    gz::msgs::WorldControl msg;
    msg.set_pause(true);
    node.Request(service_name,
        msg,
        &world_control_callback);
}

/**
 * @brief Run the Gazebo simulator server node.
 *
 * Reads ROS parameters, binds Gazebo control requests to a SimulatorServer,
 * and processes ROS callbacks until shutdown.
 */
int main(int argc, char **argv)
{
    rclcpp::init(argc,argv,rclcpp::InitOptions(),rclcpp::SignalHandlerOptions::None);
    auto node = std::make_shared<rclcpp::Node>("sas_simulator_server_gazebo_node");

    try
    {
        gz::transport::Node gazebo_node;

        std::string service_name; //Example: {"/world/shapes/control"};
        sas::get_ros_parameter(node,"service_name",service_name);
        double sampling_time;
        sas::get_ros_optional_parameter(node, "thread_sampling_time_sec",sampling_time,0.01);
        bool autostart;
        sas::get_ros_optional_parameter(node,"autostart",autostart,true);

        sas::Clock clock{sampling_time};

        auto simulator_server = sas::SimulatorServer(node);
        auto f1 = [&gazebo_node, &service_name](){start_simulation_callback(gazebo_node, service_name);};
        auto f2 = [&gazebo_node, &service_name](){stop_simulation_callback(gazebo_node, service_name);};
        simulator_server.set_start_simulation_callback(f1);
        simulator_server.set_stop_simulation_callback(f2);

        if (autostart)
        {
            RCLCPP_INFO(node->get_logger(), "Autostarting the simulation...");
            start_simulation_callback(gazebo_node, service_name);
        }

        clock.init();
        while(!ss.should_shutdown())
        {
            rclcpp::spin_some(node);
            clock.update_and_sleep();
        }

        //Statistics
        std::cout << "Statistics for the entire loop" << std::endl;
        std::cout << "  Mean computation time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Computational) << std::endl;
        std::cout << "  Mean idle time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Idle) << std::endl;
        std::cout << "  Mean effective thread sampling time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::EffectiveSampling) << std::endl;

    }
    catch (const std::exception& e)
    {
        RCLCPP_ERROR_STREAM_ONCE(node->get_logger(), std::string("::Exception::") + e.what());
    }

    return 0;
}