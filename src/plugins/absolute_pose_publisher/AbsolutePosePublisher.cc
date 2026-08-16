#include <chrono>
#include <memory>
#include <string>

#include <gz/common/Console.hh>
#include <gz/msgs/Utility.hh>
#include <gz/msgs/pose_v.pb.h>
#include <gz/plugin/Register.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Util.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Model.hh>
#include <gz/transport/Node.hh>
#include <sdf/Element.hh>

namespace absolute_pose_publisher
{

class AbsolutePosePublisher:
    public gz::sim::System,
    public gz::sim::ISystemConfigure,
    public gz::sim::ISystemPostUpdate
{
public:
  void Configure(
      const gz::sim::Entity &,
      const std::shared_ptr<const sdf::Element> &_sdf,
      gz::sim::EntityComponentManager &,
      gz::sim::EventManager &) override
  {
    if (_sdf->HasElement("topic"))
      this->topic = _sdf->Get<std::string>("topic");

    if (_sdf->HasElement("update_rate"))
      this->updateRate = _sdf->Get<double>("update_rate");

    if (_sdf->HasElement("publish_links"))
      this->publishLinks = _sdf->Get<bool>("publish_links");

    if (_sdf->HasElement("publish_joints"))
      this->publishJoints = _sdf->Get<bool>("publish_joints");

    this->publisher =
        this->node.Advertise<gz::msgs::Pose_V>(this->topic);

    if (!this->publisher)
    {
      gzerr << "Failed to advertise absolute-pose topic ["
            << this->topic << "]\n";
      return;
    }

    gzmsg << "Publishing absolute poses on ["
          << this->topic << "]\n";
  }

  void PostUpdate(
      const gz::sim::UpdateInfo &_info,
      const gz::sim::EntityComponentManager &_ecm) override
  {
    if (_info.paused || !this->publisher)
      return;

    if (this->updateRate > 0.0)
    {
      const auto period =
          std::chrono::duration<double>(1.0 / this->updateRate);

      if (this->hasPublished &&
          _info.simTime - this->lastPublicationTime < period)
      {
        return;
      }
    }

    gz::msgs::Pose_V output;
    gz::msgs::Set(
        output.mutable_header()->mutable_stamp(),
        _info.simTime);

    // Publish every model in the world.
    _ecm.Each<gz::sim::components::Model>(
        [&](const gz::sim::Entity &_entity,
            const gz::sim::components::Model *) -> bool
        {
          this->AddWorldPose(_entity, _ecm, output);
          return true;
        });

    // Optionally publish every link in the world.
    if (this->publishLinks)
    {
      _ecm.Each<gz::sim::components::Link>(
          [&](const gz::sim::Entity &_entity,
              const gz::sim::components::Link *) -> bool
          {
            this->AddWorldPose(_entity, _ecm, output);
            return true;
          });
    }

    // Optionally publish every joint in the world.
    if (this->publishJoints)
    {
      _ecm.Each<gz::sim::components::Joint>(
          [&](const gz::sim::Entity &_entity,
              const gz::sim::components::Joint *) -> bool
          {
            this->AddWorldPose(_entity, _ecm, output);
            return true;
          });
    }

    this->publisher.Publish(output);
    this->lastPublicationTime = _info.simTime;
    this->hasPublished = true;
  }

private:
  void AddWorldPose(
      const gz::sim::Entity &_entity,
      const gz::sim::EntityComponentManager &_ecm,
      gz::msgs::Pose_V &_output) const
  {
    const gz::math::Pose3d worldPose =
        gz::sim::worldPose(_entity, _ecm);

    auto *poseMsg = _output.add_pose();
    poseMsg->set_id(_entity);

    std::string name =
        gz::sim::scopedName(_entity, _ecm, "/", false);

    const auto firstSlash = name.find('/');
    if (firstSlash != std::string::npos)
    {
        name.erase(0, firstSlash + 1);
    }

    poseMsg->set_name(name);

    gz::msgs::Set(poseMsg, worldPose);
  }

private:
  gz::transport::Node node;
  gz::transport::Node::Publisher publisher;

  std::string topic{"/absolute_pose/info"};
  double updateRate{50.0};
  bool publishLinks{true};
  bool publishJoints{false};

  bool hasPublished{false};
  std::chrono::steady_clock::duration lastPublicationTime{0};
};

}  // namespace absolute_pose_publisher

GZ_ADD_PLUGIN(
    absolute_pose_publisher::AbsolutePosePublisher,
    gz::sim::System,
    absolute_pose_publisher::AbsolutePosePublisher::ISystemConfigure,
    absolute_pose_publisher::AbsolutePosePublisher::ISystemPostUpdate)

GZ_ADD_PLUGIN_ALIAS(
    absolute_pose_publisher::AbsolutePosePublisher,
    "absolute_pose_publisher::AbsolutePosePublisher")
