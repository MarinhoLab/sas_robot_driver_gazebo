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
#include <gz/msgs.hh>
#include <gz/transport.hh>
#include <rclcpp/rclcpp.hpp>
#include <sas_core/sas_clock.hpp>
#include <sas_core/sas_shutdown_signaler.hpp>
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

void world_control_callback(const gz::msgs::Boolean&, const bool)
{
    //TODO add something here if needed
}

void start_simulation_callback(gz::transport::Node& node, const std::string& service_name)
{
    gz::msgs::WorldControl msg;
    msg.set_pause(false);
    node.Request(service_name,
        msg,
        &world_control_callback);
}

void stop_simulation_callback(gz::transport::Node& node, const std::string& service_name)
{
    gz::msgs::WorldControl msg;
    msg.set_pause(true);
    node.Request(service_name,
        msg,
        &world_control_callback);
}

int main(int argc, char **argv)
{
    rclcpp::init(argc,argv,rclcpp::InitOptions(),rclcpp::SignalHandlerOptions::None);
    auto node = std::make_shared<rclcpp::Node>("sas_simulator_server_gazebo_node");
    gz::transport::Node gazebo_node;

    std::string service_name{"/world/shapes/control"};

    auto simulator_server = sas::SimulatorServer(node);
    auto f1 = [&gazebo_node, &service_name](){start_simulation_callback(gazebo_node, service_name);};
    auto f2 = [&gazebo_node, &service_name](){stop_simulation_callback(gazebo_node, service_name);};
    simulator_server.set_start_simulation_callback(f1);
    simulator_server.set_stop_simulation_callback(f2);

    sas::Clock clock{0.001};
    try
    {
        clock.init();
        while(!ss.should_shutdown())
        {
            rclcpp::spin_some(node);
            clock.update_and_sleep();
        }
    }
    catch (const std::exception& e)
    {
        RCLCPP_ERROR_STREAM_ONCE(node->get_logger(), std::string("::Exception::") + e.what());
    }

    //Statistics
    std::cout << "Statistics for the entire loop" << std::endl;
    std::cout << "  Mean computation time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Computational) << std::endl;
    std::cout << "  Mean idle time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Idle) << std::endl;
    std::cout << "  Mean effective thread sampling time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::EffectiveSampling) << std::endl;

    return 0;
}