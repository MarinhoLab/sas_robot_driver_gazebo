#include <iostream>
#include <gz/msgs.hh>
#include <gz/transport.hh>
#include <sas_core/sas_clock.hpp>

void response_callback(const gz::msgs::Boolean&, const bool)
{

}

//////////////////////////////////////////////////
int main(int, char**)
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

  //Statistics
  std::cout << "Statistics for the entire loop" << std::endl;
  std::cout << "  Mean computation time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Computational) << std::endl;
  std::cout << "  Mean idle time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::Idle) << std::endl;
  std::cout << "  Mean effective thread sampling time: " << clock.get_statistics(sas::Statistics::Mean,sas::Clock::TimeType::EffectiveSampling) << std::endl;

  // Zzzzzz.
  // gz::transport::waitForShutdown();
  return 0;
}