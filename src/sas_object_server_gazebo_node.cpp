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

#include <sas_core/sas_clock.hpp>

using namespace DQ_robotics;

void response_callback(const gz::msgs::Boolean &_rep, const bool _result)
{

}

int main(int argc, char **argv)
{
  sas::Clock clock{0.001};

  gz::transport::Node node;
  gz::msgs::Pose req;
  req.set_name("frame_x");

  std::cout << "Press <CTRL-C> to exit" << std::endl;

  clock.init();
  for(int i=0;i<50;i++)
  {
      auto z = 0.001 * i;
      req.mutable_position()->set_z(z);
      node.Request("/world/ur3e_position_world/set_pose", req, response_callback);
      clock.update_and_sleep();
  }

  std::cout << "Statistics for the entire loop" << std::endl;
  std::cout << "  Mean computation time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Computational) << std::endl;
  std::cout << "  Mean idle time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Idle) << std::endl;
  std::cout << "  Mean effective thread sampling time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::EffectiveSampling) << std::endl;

  gz::transport::waitForShutdown();
}