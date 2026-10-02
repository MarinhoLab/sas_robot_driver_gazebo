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
#pragma once

/**
 * @file sas_robot_driver_gazebo.hpp
 * @brief Gazebo implementation of sas::RobotDriver, in C++.
 *
 * The C++ counterpart of sas_robot_driver_gazebo.RobotDriverGazebo (Python):
 * publishes joint position commands to Gazebo's JointPositionController
 * topics and keeps the latest joint states from a JointStatePublisher topic.
 * The joint-state callback runs on gz-transport's thread and only copies the
 * configured joints into vectors under a mutex, so it never waits for the
 * driver's control loop (nor for a Python interpreter lock).
 */

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <Eigen/Dense>
#include <gz/msgs/model.pb.h>
#include <gz/transport/Node.hh>
#include <sas_core/sas_robot_driver.hpp>

namespace sas
{

/**
 * @brief Configuration of RobotDriverGazebo (the same fields as the Python driver's).
 */
struct RobotDriverGazeboConfiguration
{
    /// Names of the Gazebo joints to control, in the driver's order.
    std::vector<std::string> joint_names;
    /// Prefix of the joints' command topics: <prefix><joint name>/0/cmd_pos.
    std::string joint_positions_topic_prefix;
    /// Gazebo topic of the joint states (gz.msgs.Model, JointStatePublisher).
    std::string joint_states_topic;
};

/**
 * @brief Gazebo implementation of RobotDriver.
 */
class RobotDriverGazebo: public RobotDriver
{
private:
    RobotDriverGazeboConfiguration configuration_;
    gz::transport::Node node_;
    std::vector<gz::transport::Node::Publisher> joint_publishers_;
    std::unordered_map<std::string, int> joint_index_;

    mutable std::mutex mutex_;
    Eigen::VectorXd positions_;
    Eigen::VectorXd velocities_;
    Eigen::VectorXd torques_;
    Eigen::VectorXd limit_lower_;
    Eigen::VectorXd limit_upper_;
    std::vector<bool> received_;
    bool all_received_{false};

    void _joint_states_callback(const gz::msgs::Model& msg);
    Eigen::VectorXd _copy_if_received(const Eigen::VectorXd& values, const std::string& what) const;

public:
    RobotDriverGazebo(const RobotDriverGazebo&)=delete;
    RobotDriverGazebo()=delete;
    ~RobotDriverGazebo() override = default;

    /**
     * @brief Construct a Gazebo robot driver.
     * @param shutdown_signaler Signals the driver to stop waiting (e.g. on SIGINT).
     * @param configuration Joint names and Gazebo topics.
     */
    RobotDriverGazebo(const std::shared_ptr<ShutdownSignaler>& shutdown_signaler,
                      const RobotDriverGazeboConfiguration& configuration);

    Eigen::VectorXd get_joint_positions() override;
    void set_target_joint_positions(const Eigen::VectorXd& target_joint_positions_rad) override;
    Eigen::VectorXd get_joint_velocities() override;
    Eigen::VectorXd get_joint_torques() override;

    void connect() override;
    void disconnect() override;
    void initialize() override;
    void deinitialize() override;
};

}
