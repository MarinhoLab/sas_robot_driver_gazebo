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
#   Claude (Anthropic), C++ port of sas_robot_driver_gazebo.py
# ################################################################
*/

/**
 * @file sas_robot_driver_gazebo.cpp
 * @brief Gazebo implementation of sas::RobotDriver, in C++.
 */

#include <sas_robot_driver_gazebo/sas_robot_driver_gazebo.hpp>

#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

#include <gz/msgs/double.pb.h>

namespace sas
{

RobotDriverGazebo::RobotDriverGazebo(const std::shared_ptr<ShutdownSignaler>& shutdown_signaler,
                                     const RobotDriverGazeboConfiguration& configuration):
    RobotDriver(shutdown_signaler),
    configuration_(configuration)
{
    const auto n = static_cast<Eigen::Index>(configuration_.joint_names.size());
    if(n == 0)
        throw std::invalid_argument("RobotDriverGazebo: joint_names is empty.");
    for(Eigen::Index i = 0; i < n; ++i)
        joint_index_[configuration_.joint_names.at(static_cast<size_t>(i))] = static_cast<int>(i);
    positions_ = Eigen::VectorXd::Constant(n, std::nan(""));
    velocities_ = positions_;
    torques_ = positions_;
    limit_lower_ = positions_;
    limit_upper_ = positions_;
    received_.assign(static_cast<size_t>(n), false);
}

void RobotDriverGazebo::_joint_states_callback(const gz::msgs::Model& msg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for(const auto& joint: msg.joint())
    {
        const auto found = joint_index_.find(joint.name());
        if(found == joint_index_.end())
            continue;
        const auto i = found->second;
        const auto& axis = joint.axis1();
        positions_(i) = axis.position();
        velocities_(i) = axis.velocity();
        torques_(i) = axis.force();
        limit_lower_(i) = axis.limit_lower();
        limit_upper_(i) = axis.limit_upper();
        received_.at(static_cast<size_t>(i)) = true;
    }
    if(!all_received_)
    {
        all_received_ = true;
        for(const bool r: received_)
            all_received_ = all_received_ && r;
    }
}

Eigen::VectorXd RobotDriverGazebo::_copy_if_received(const Eigen::VectorXd& values, const std::string& what) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(!all_received_)
        throw std::runtime_error("RobotDriverGazebo: " + what + " not initialized.");
    return values;
}

const RobotDriverGazeboConfiguration& RobotDriverGazebo::get_configuration() const
{
    return configuration_;
}

Eigen::VectorXd RobotDriverGazebo::get_joint_positions()
{
    return _copy_if_received(positions_, "joint positions");
}

Eigen::VectorXd RobotDriverGazebo::get_joint_velocities()
{
    return _copy_if_received(velocities_, "joint velocities");
}

Eigen::VectorXd RobotDriverGazebo::get_joint_torques()
{
    return _copy_if_received(torques_, "joint torques");
}

void RobotDriverGazebo::set_target_joint_positions(const Eigen::VectorXd& target_joint_positions_rad)
{
    if(target_joint_positions_rad.size() != static_cast<Eigen::Index>(joint_publishers_.size()))
        throw std::invalid_argument("RobotDriverGazebo::set_target_joint_positions: expected "
                                    + std::to_string(joint_publishers_.size()) + " values, got "
                                    + std::to_string(target_joint_positions_rad.size()) + ".");
    gz::msgs::Double msg;
    for(size_t i = 0; i < joint_publishers_.size(); ++i)
    {
        msg.set_data(target_joint_positions_rad(static_cast<Eigen::Index>(i)));
        joint_publishers_.at(i).Publish(msg);
    }
}

void RobotDriverGazebo::connect()
{
    joint_publishers_.clear();
    for(const auto& joint_name: configuration_.joint_names)
    {
        const auto topic = configuration_.joint_positions_topic_prefix + joint_name + "/0/cmd_pos";
        joint_publishers_.push_back(node_.Advertise<gz::msgs::Double>(topic));
        if(!joint_publishers_.back())
            throw std::runtime_error("RobotDriverGazebo: failed to advertise [" + topic + "].");
    }
    if(!node_.Subscribe(configuration_.joint_states_topic,
                        &RobotDriverGazebo::_joint_states_callback, this))
        throw std::runtime_error("RobotDriverGazebo: failed to subscribe to ["
                                 + configuration_.joint_states_topic + "].");
}

void RobotDriverGazebo::disconnect()
{
    node_.Unsubscribe(configuration_.joint_states_topic);
}

void RobotDriverGazebo::initialize()
{
    // Wait for every joint's state (positions and limits) from Gazebo.
    while(true)
    {
        {
            // Unlimited joints (e.g. continuous ones) report infinite limits.
            std::lock_guard<std::mutex> lock(mutex_);
            if(all_received_ && !limit_lower_.array().isNaN().any() && !limit_upper_.array().isNaN().any())
                break;
        }
        if(shutdown_signaler_ && shutdown_signaler_->should_shutdown())
            throw std::runtime_error("RobotDriverGazebo::initialize: shutdown while waiting for joint states on ["
                                     + configuration_.joint_states_topic + "].");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::lock_guard<std::mutex> lock(mutex_);
    set_joint_limits({limit_lower_, limit_upper_});
}

void RobotDriverGazebo::deinitialize()
{
}

}
