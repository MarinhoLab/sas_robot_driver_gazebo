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
#include <rclcpp/rclcpp.hpp>
#include "sas_object_server_gazebo.hpp"
#include <sas_core/sas_clock.hpp>
#include <sas_core/sas_shutdown_signaler.hpp>
#include <sas_common/sas_common.hpp>

/*********************************************
 * SIGNAL HANDLER
 * *******************************************/
#include<signal.h>
static sas::ShutdownSignaler ss;
void sig_int_handler(int)
{
    ss.shutdown();
}

int main(int argc, char **argv)
{
    rclcpp::init(argc,argv,rclcpp::InitOptions(),rclcpp::SignalHandlerOptions::None);
    auto node = std::make_shared<rclcpp::Node>("sas_object_server_gazebo_node");
    sas::Clock clock{0.001};

    try
    {

        sas::ObjectServerGazeboConfiguration configuration;
        sas::get_ros_parameter(node,"set_pose_service_name",configuration.set_pose_service_name);
        sas::get_ros_parameter(node,"get_pose_topic_name",configuration.get_pose_topic_name);
        sas::get_ros_parameter(node,"entity_names",configuration.entity_names);

        ///Example:
        //configuration.set_pose_service_name = "/world/ur3e_world/set_pose";
        //configuration.get_pose_topic_name = "/world/ur3e_world/pose/info";
        //configuration.entity_names = std::vector<std::string>{
        //    "frame_x",
        //    "frame_xd"
        //    };

        std::vector<std::unique_ptr<sas::ObjectServerGazebo>> object_server_gazebo_list;
        for(const std::string& entity_name: configuration.entity_names)
        {
            object_server_gazebo_list.emplace_back(
                std::make_unique<sas::ObjectServerGazebo>(
                std::make_shared<sas::ObjectServer>(node, entity_name),
                entity_name,
                configuration.set_pose_service_name,
                configuration.get_pose_topic_name
                )
            );
        }

        clock.init();
        while(!ss.should_shutdown())
        {
            rclcpp::spin_some(node);
            clock.update_and_sleep();

            for(auto& object_server_gazebo: object_server_gazebo_list)
            {
                object_server_gazebo->update();
            }
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