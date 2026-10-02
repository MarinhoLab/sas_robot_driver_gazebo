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
 * Publishes joint position commands to Gazebo's JointPositionController
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
 * @struct RobotDriverGazeboConfiguration
 * @brief Configuration of RobotDriverGazebo: the joints and the Gazebo topics.
 */
struct RobotDriverGazeboConfiguration
{
    /// Names of the Gazebo joints to control, in the driver's order.
    std::vector<std::string> joint_names;
    /// Prefix of the joints' command topics, which are `prefix + joint name + "/0/cmd_pos"`.
    std::string joint_positions_topic_prefix;
    /// Gazebo topic of the joint states (gz.msgs.Model, from a JointStatePublisher).
    std::string joint_states_topic;
};

/**
 * @class RobotDriverGazebo
 * @brief Gazebo implementation of RobotDriver.
 *
 * Exchanges joint commands and joint states with Gazebo through
 * gz-transport: one gz.msgs.Double command topic per joint (a
 * JointPositionController) and one gz.msgs.Model joint-state topic (a
 * JointStatePublisher). Thread-safe: the joint-state callback and the
 * getters share the cached values under a mutex.
 */
class RobotDriverGazebo: public RobotDriver
{
private:
    RobotDriverGazeboConfiguration configuration_;                  ///< Joints and Gazebo topics.
    gz::transport::Node node_;                                       ///< gz-transport node of the publishers and the subscription.
    std::vector<gz::transport::Node::Publisher> joint_publishers_;   ///< One command publisher per joint, in configuration order.
    std::unordered_map<std::string, int> joint_index_;              ///< Index of each configured joint name.

    mutable std::mutex mutex_;          ///< Guards the cached joint states below.
    Eigen::VectorXd positions_;         ///< Latest joint positions [rad] (NaN until received).
    Eigen::VectorXd velocities_;        ///< Latest joint velocities [rad/s] (NaN until received).
    Eigen::VectorXd torques_;           ///< Latest joint torques [N m] (NaN until received).
    Eigen::VectorXd limit_lower_;       ///< Lower joint limits [rad], as Gazebo reports them.
    Eigen::VectorXd limit_upper_;       ///< Upper joint limits [rad], as Gazebo reports them.
    std::vector<bool> received_;        ///< Whether each joint's state has been received.
    bool all_received_{false};          ///< Whether every joint's state has been received.

    /**
     * @brief Copy the configured joints' states from a Gazebo joint-state message.
     *
     * Runs on gz-transport's thread; joints that are not configured are ignored.
     * @param msg Joint states published by Gazebo's JointStatePublisher.
     */
    void _joint_states_callback(const gz::msgs::Model& msg);

    /**
     * @brief Copy one of the cached vectors, once every joint's state has been received.
     * @param values The cached vector to copy.
     * @param what What the values are, for the exception's message.
     * @return A copy of values.
     * @throws std::runtime_error if some joint's state has not been received yet.
     */
    Eigen::VectorXd _copy_if_received(const Eigen::VectorXd& values, const std::string& what) const;

public:
    RobotDriverGazebo(const RobotDriverGazebo&)=delete;   ///< Not copyable.
    RobotDriverGazebo()=delete;                           ///< Needs a configuration.
    ~RobotDriverGazebo() override = default;              ///< Destructor.

    /**
     * @brief Construct a Gazebo robot driver.
     * @param shutdown_signaler Signals the driver to stop waiting (e.g. on SIGINT).
     * @param configuration Joint names and Gazebo topics.
     * @throws std::invalid_argument if configuration has no joints.
     */
    RobotDriverGazebo(const std::shared_ptr<ShutdownSignaler>& shutdown_signaler,
                      const RobotDriverGazeboConfiguration& configuration);

    /**
     * @brief The configuration this driver was constructed with.
     * @return The joints and Gazebo topics.
     */
    const RobotDriverGazeboConfiguration& get_configuration() const;

    /**
     * @brief The latest joint positions from Gazebo.
     * @return Joint positions [rad], in configuration order.
     * @throws std::runtime_error before every joint's state has been received.
     */
    Eigen::VectorXd get_joint_positions() override;

    /**
     * @brief Publish target joint positions to Gazebo's joint position controllers.
     * @param target_joint_positions_rad Target joint positions [rad], in configuration order.
     * @throws std::invalid_argument if the size differs from the number of joints.
     */
    void set_target_joint_positions(const Eigen::VectorXd& target_joint_positions_rad) override;

    /**
     * @brief The latest joint velocities from Gazebo.
     * @return Joint velocities [rad/s], in configuration order.
     * @throws std::runtime_error before every joint's state has been received.
     */
    Eigen::VectorXd get_joint_velocities() override;

    /**
     * @brief The latest joint torques from Gazebo.
     * @return Joint torques [N m], in configuration order.
     * @throws std::runtime_error before every joint's state has been received.
     */
    Eigen::VectorXd get_joint_torques() override;

    /**
     * @brief Advertise the joints' command topics and subscribe to the joint-state topic.
     * @throws std::runtime_error if a topic cannot be advertised or subscribed to.
     */
    void connect() override;

    /**
     * @brief Unsubscribe from the joint-state topic.
     */
    void disconnect() override;

    /**
     * @brief Wait for every joint's state and limits from Gazebo, then set the joint limits.
     * @throws std::runtime_error if a shutdown is signaled while waiting.
     */
    void initialize() override;

    /**
     * @brief Nothing to undo in Gazebo.
     */
    void deinitialize() override;
};

}
