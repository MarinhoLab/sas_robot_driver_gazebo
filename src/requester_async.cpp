#include <iostream>
#include <gz/msgs.hh>
#include <gz/transport.hh>

//////////////////////////////////////////////////
/// \brief Service response callback.
void responseCb(const gz::msgs::Boolean &_rep, const bool _result)
{
  if (_result)
    std::cout << "Response: [" << _rep.data() << "]" << std::endl;
  else
    std::cerr << "Service call failed" << std::endl;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  gz::transport::Node node;
  gz::msgs::Pose req;
  req.set_name("pose_x");
  req.mutable_position()->set_z(2.0);

  std::cout << "Press <CTRL-C> to exit" << std::endl;

  // Request the "/echo" service.
  node.Request("/world/ur3e_position_world/set_pose", req, responseCb);

  // Zzzzzz.
  gz::transport::waitForShutdown();
}