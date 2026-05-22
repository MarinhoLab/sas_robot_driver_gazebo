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
#include "sas_object_server_gazebo.hpp"

namespace sas
{

std::unique_ptr<ObjectServerGazebo::PoseSubscriberSingleton> ObjectServerGazebo::pose_subscriber_;

void ObjectServerGazebo::_set_pose_service_response_callback(const gz::msgs::Boolean&, const bool)
{
    //TODO if needed add something here
}

ObjectServerGazebo::ObjectServerGazebo(const std::shared_ptr<ObjectServer>& object_server,
                                       const std::string& gazebo_entity_name,
                                       const std::string& gazebo_set_pose_service_name,
                                       const std::string& gazebo_world_dynamic_pose_topic):
       sas::Object("sas::ObjectServerGazebo"),
       object_server_(object_server),
       gazebo_entity_name_(gazebo_entity_name),
       gazebo_set_pose_service_name_(gazebo_set_pose_service_name)
{
    pose_to_gazebo_msg_.set_name(gazebo_entity_name_);
    if(!pose_subscriber_)
    {
        pose_subscriber_ = std::make_unique<PoseSubscriberSingleton>();
        pose_subscriber_->gazebo_node_.Subscribe(gazebo_world_dynamic_pose_topic,
                                        &PoseSubscriberSingleton::_get_pose_subscriber_callback,
                                        pose_subscriber_.get()
                                        );
    }
}

void ObjectServerGazebo::update()
{
    if(object_server_->is_enabled())
    {
        //Send target pose to Gazebo
        auto target_pose{object_server_->get_target_pose()};
        auto r{target_pose.rotation()};
        auto t{target_pose.translation()};

        pose_to_gazebo_msg_.mutable_position()->set_x(t.q[1]);
        pose_to_gazebo_msg_.mutable_position()->set_y(t.q[2]);
        pose_to_gazebo_msg_.mutable_position()->set_z(t.q[3]);

        pose_to_gazebo_msg_.mutable_orientation()->set_w(r.q[0]);
        pose_to_gazebo_msg_.mutable_orientation()->set_x(r.q[1]);
        pose_to_gazebo_msg_.mutable_orientation()->set_y(r.q[2]);
        pose_to_gazebo_msg_.mutable_orientation()->set_z(r.q[3]);
    }
    //Get current pose from Gazebo
    gz::msgs::Pose_V poses_from_gazebo;
    {
        std::scoped_lock lock(ObjectServerGazebo::pose_subscriber_->msg_mutex);
        poses_from_gazebo = ObjectServerGazebo::pose_subscriber_->poses_from_gazebo_;
    }
    for(auto i=0;i<poses_from_gazebo.pose_size();i++)
    {
        auto pose = poses_from_gazebo.pose(i);
        if(pose.name() == gazebo_entity_name_)
        {
            auto pose_r = DQ(
                pose.orientation().w(),
                pose.orientation().x(),
                pose.orientation().y(),
                pose.orientation().z()
            );
            auto pose_t = DQ(
                pose.position().x(),
                pose.position().y(),
                pose.position().z()
            );
            object_server_->send_pose(pose_r + 0.5 * E_ * pose_t * pose_r);
        }
    }
}

}