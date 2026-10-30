/*
# Copyright (c) 2012-2026 Murilo Marques Marinho
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
*/

//Initial version based on official example.
#include <iostream>

/**
 * @file gazebo_subscription_frequency_checker.cpp
 * @brief Gazebo subscription timing tool.
 *
 * Measures the rate of Gazebo pose updates.
 */

#include <string>
#include <gz/msgs.hh>
#include <gz/transport.hh>
#include <sas_core/sas_clock.hpp>

static int message_count = 0;

/**
 * @brief Count Gazebo pose messages.
 *
 */
void cb(const gz::msgs::Pose_V&)
{
  message_count++;
}

/**
 * @brief Run the Gazebo subscription timing tool.
 *
 * Subscribes to a Gazebo pose topic and reports timing statistics after a
 * fixed number of messages.
 *
 */
int main(int, char **)
{
  sas::Clock clock{0.001};
  gz::transport::Node node;
  std::string topic = "/world/ur3e_world/dynamic_pose/info";

  // Subscribe to a topic by registering a callback.
  if (!node.Subscribe(topic, cb))
  {
    std::cerr << "Error subscribing to topic [" << topic << "]" << std::endl;
    return -1;
  }

  clock.init();
  while(message_count < 50)
  {
      clock.update_and_sleep();
  }
  // Message count
  std::cout << "Received " << message_count << " messages" << std::endl;

  //Statistics
  std::cout << "Statistics for the entire loop" << std::endl;
  std::cout << "  Mean computation time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Computational) << std::endl;
  std::cout << "  Mean idle time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Idle) << std::endl;
  std::cout << "  Mean effective thread sampling time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::EffectiveSampling) << std::endl;

  // Zzzzzz.
  //gz::transport::waitForShutdown();

  return 0;
}