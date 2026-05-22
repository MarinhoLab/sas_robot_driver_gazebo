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

    std::string world_topic = "/world/ur3e_world/pose/info";

    auto os = std::make_shared<sas::ObjectServer>(node);
    sas::ObjectServerGazebo osg{os,"frame_x","/world/ur3e_world/set_pose",world_topic};

    sas::Clock clock{0.001};

    try
    {
        clock.init();
        while(!ss.should_shutdown())
        {
            rclcpp::spin_some(node);
            clock.update_and_sleep();
            osg.update();
        }
    }
    catch (const std::exception& e)
    {
        RCLCPP_ERROR_STREAM_ONCE(node->get_logger(), std::string("::Exception::") + e.what());
    }

    return 0;
}