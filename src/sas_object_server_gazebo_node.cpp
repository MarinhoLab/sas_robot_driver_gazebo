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

    //std::string
    //std::string
    //std::vector<string>

    sas::ObjectServerGazeboConfiguration configuration;
    configuration.set_pose_service_name = "/world/ur3e_world/set_pose";
    configuration.get_pose_topic_name = "/world/ur3e_world/pose/info";
    configuration.entity_names = std::vector<std::string>{
        "frame_x",
        "frame_xd"
        };

    std::vector<sas::ObjectServerGazebo> object_server_gazebo_list;
    for(const auto& entity_name: configuration.entity_names)
    {
        auto a = std::make_shared<sas::ObjectServer>(node, entity_name);
        object_server_gazebo_list.emplace_back(
            a,
            entity_name,
            configuration.set_pose_service_name,
            configuration.get_pose_topic_name
        );
    }

    //auto osx = std::make_shared<sas::ObjectServer>(node, "frame_x");
    //sas::ObjectServerGazebo osg_x{osx,"frame_x",set_pose_service_name,get_pose_topic_name};
    //auto osxd = std::make_shared<sas::ObjectServer>(node, "frame_xd");
    //sas::ObjectServerGazebo osg_xd{osxd,"frame_xd",set_pose_service_name,get_pose_topic_name};

    sas::Clock clock{0.001};

    try
    {
        clock.init();
        while(!ss.should_shutdown())
        {
            rclcpp::spin_some(node);
            clock.update_and_sleep();

            for(auto& object_server_gazebo: object_server_gazebo_list)
            {
                object_server_gazebo.update();
            }
        }
    }
    catch (const std::exception& e)
    {
        RCLCPP_ERROR_STREAM_ONCE(node->get_logger(), std::string("::Exception::") + e.what());
    }

    return 0;
}