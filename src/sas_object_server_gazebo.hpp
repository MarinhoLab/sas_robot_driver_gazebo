#pragma once
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
#include <dqrobotics/DQ.h>

#include <gz/msgs.hh>
#include <gz/transport.hh>

#include <sas_core/sas_object.hpp>
#include <sas_common/sas_object_server.hpp>

using namespace DQ_robotics;

namespace sas
{

struct ObjectServerGazeboConfiguration
{
    std::string set_pose_service_name;
    std::string get_pose_topic_name;
    std::vector<std::string> entity_names;
};

class ObjectServerGazebo: private sas::Object
{
    private:
        class PoseSubscriberSingleton
        {
            public:
            std::mutex msg_mutex;
            void _get_pose_subscriber_callback(const gz::msgs::Pose_V& msg)
            {
                std::unique_lock<std::mutex> lock(msg_mutex, std::try_to_lock);
                if (lock.owns_lock()) {
                    poses_from_gazebo_ = msg;
                }
            }
            gz::transport::Node gazebo_node_;
            gz::msgs::Pose_V poses_from_gazebo_;
        };

        static std::unique_ptr<PoseSubscriberSingleton> pose_subscriber_;
        std::shared_ptr<ObjectServer> object_server_;

        std::string gazebo_entity_name_;
        std::string gazebo_set_pose_service_name_;

        gz::transport::Node gazebo_node_;
        gz::msgs::Pose pose_to_gazebo_msg_;
        DQ last_pose_to_gazebo_{0};

        void _set_pose_service_response_callback(const gz::msgs::Boolean&, const bool);
    public:
        ObjectServerGazebo() = delete;
        ObjectServerGazebo(const ObjectServerGazebo&) = delete;
        ObjectServerGazebo(ObjectServerGazebo&&) = default;

        ObjectServerGazebo(const std::shared_ptr<ObjectServer>& object_server,
                           const std::string& gazebo_entity_name,
                           const std::string& gazebo_set_pose_service_name,
                           const std::string& gazebo_world_dynamic_pose_topic);

        void update();
};

}

