#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <gz/common/Console.hh>
#include <gz/msgs/boolean.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/plugin/Register.hh>
#include <gz/sim/Entity.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Util.hh>
#include <gz/sim/components/Collision.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Sensor.hh>
#include <gz/sim/components/Visual.hh>
#include <gz/transport/Node.hh>
#include <sdf/Element.hh>

namespace scoped_set_pose
{

class ScopedSetPose:
    public gz::sim::System,
    public gz::sim::ISystemConfigure,
    public gz::sim::ISystemPostUpdate
{
public:
  void Configure(
      const gz::sim::Entity &_entity,
      const std::shared_ptr<const sdf::Element> &_sdf,
      gz::sim::EntityComponentManager &_ecm,
      gz::sim::EventManager &) override
  {
    const auto worldNameComp =
        _ecm.Component<gz::sim::components::Name>(_entity);

    if (!worldNameComp)
    {
      gzerr << "ScopedSetPose must be attached to a world entity.\n";
      return;
    }

    this->worldName = worldNameComp->Data();
    this->defaultSetPoseService = "/world/" + this->worldName + "/set_pose";
    this->service = "/world/" + this->worldName + "/set_pose_scoped";

    if (_sdf->HasElement("service"))
      this->service = _sdf->Get<std::string>("service");

    if (_sdf->HasElement("target_service"))
      this->defaultSetPoseService = _sdf->Get<std::string>("target_service");

    if (_sdf->HasElement("timeout_ms"))
      this->timeoutMs = _sdf->Get<unsigned int>("timeout_ms");

    if (!this->node.Advertise(
            this->service,
            &ScopedSetPose::OnSetPose,
            this))
    {
      gzerr << "Failed to advertise scoped set-pose service ["
            << this->service << "]\n";
      return;
    }

    gzmsg << "Scoped set-pose service: [" << this->service << "]\n"
          << "Forwarding to Gazebo set-pose service: ["
          << this->defaultSetPoseService << "]\n";
  }

  void PostUpdate(
      const gz::sim::UpdateInfo &,
      const gz::sim::EntityComponentManager &_ecm) override
  {
    std::unordered_map<std::string, gz::sim::Entity> next;

    auto addEntity = [&](const gz::sim::Entity &_entity)
    {
      const std::string full =
          gz::sim::scopedName(_entity, _ecm, "/", false);

      if (!full.empty())
      {
        next[full] = _entity;
        next[this->WithoutWorldPrefix(full)] = _entity;
      }

      const auto nameComp =
          _ecm.Component<gz::sim::components::Name>(_entity);
      if (nameComp)
        next[nameComp->Data()] = _entity;
    };

    _ecm.Each<gz::sim::components::Model>(
        [&](const gz::sim::Entity &_entity,
            const gz::sim::components::Model *) -> bool
        {
          addEntity(_entity);
          return true;
        });

    _ecm.Each<gz::sim::components::Link>(
        [&](const gz::sim::Entity &_entity,
            const gz::sim::components::Link *) -> bool
        {
          addEntity(_entity);
          return true;
        });

    _ecm.Each<gz::sim::components::Joint>(
        [&](const gz::sim::Entity &_entity,
            const gz::sim::components::Joint *) -> bool
        {
          addEntity(_entity);
          return true;
        });

    _ecm.Each<gz::sim::components::Sensor>(
        [&](const gz::sim::Entity &_entity,
            const gz::sim::components::Sensor *) -> bool
        {
          addEntity(_entity);
          return true;
        });

    _ecm.Each<gz::sim::components::Collision>(
        [&](const gz::sim::Entity &_entity,
            const gz::sim::components::Collision *) -> bool
        {
          addEntity(_entity);
          return true;
        });

    _ecm.Each<gz::sim::components::Visual>(
        [&](const gz::sim::Entity &_entity,
            const gz::sim::components::Visual *) -> bool
        {
          addEntity(_entity);
          return true;
        });

    std::lock_guard<std::mutex> lock(this->mutex);
    this->entityByName = std::move(next);
  }

private:
  bool OnSetPose(
      const gz::msgs::Pose &_req,
      gz::msgs::Boolean &_rep)
  {
    gz::msgs::Pose forwarded = _req;
    const std::string requestedName = _req.name();

    gz::sim::Entity entity = gz::sim::kNullEntity;
    if (!requestedName.empty())
    {
      const std::string normalized = this->NormalizeName(requestedName);

      std::lock_guard<std::mutex> lock(this->mutex);
      auto it = this->entityByName.find(normalized);
      if (it == this->entityByName.end())
        it = this->entityByName.find(this->WithWorldPrefix(normalized));

      if (it != this->entityByName.end())
        entity = it->second;
    }

    if (entity != gz::sim::kNullEntity)
    {
      // Gazebo's default /world/<world>/set_pose service is gz.msgs.Pose ->
      // gz.msgs.Boolean. This plugin keeps the same wire format and only
      // enriches the request with the resolved entity ID before forwarding it.
      forwarded.set_id(static_cast<uint32_t>(entity));
      forwarded.set_name(requestedName);
    }
    else if (forwarded.id() == 0u)
    {
      gzerr << "Scoped set-pose could not resolve entity name ["
            << requestedName << "] and no non-zero ID was provided.\n";
      _rep.set_data(false);
      return true;
    }

    bool result = false;
    gz::msgs::Boolean defaultRep;

    const bool executed = this->node.Request(
        this->defaultSetPoseService,
        forwarded,
        this->timeoutMs,
        defaultRep,
        result);

    if (!executed || !result)
    {
      gzerr << "Request to [" << this->defaultSetPoseService
            << "] failed or timed out.\n";
      _rep.set_data(false);
      return true;
    }

    _rep.set_data(defaultRep.data());
    return true;
  }

  std::string NormalizeName(std::string _name) const
  {
    // Accept both Gazebo-style and frame-style names.
    std::string::size_type pos = 0;
    while ((pos = _name.find("::", pos)) != std::string::npos)
    {
      _name.replace(pos, 2, "/");
      pos += 1;
    }

    while (!_name.empty() && _name.front() == '/')
      _name.erase(_name.begin());

    return this->WithoutWorldPrefix(_name);
  }

  std::string WithoutWorldPrefix(const std::string &_name) const
  {
    const std::string prefix = this->worldName + "/";
    if (_name.rfind(prefix, 0) == 0)
      return _name.substr(prefix.size());

    return _name;
  }

  std::string WithWorldPrefix(const std::string &_name) const
  {
    if (_name.rfind(this->worldName + "/", 0) == 0)
      return _name;

    return this->worldName + "/" + _name;
  }

private:
  gz::transport::Node node;

  std::string worldName;
  std::string service;
  std::string defaultSetPoseService;
  unsigned int timeoutMs{1000u};

  std::mutex mutex;
  std::unordered_map<std::string, gz::sim::Entity> entityByName;
};

}  // namespace scoped_set_pose

GZ_ADD_PLUGIN(
    scoped_set_pose::ScopedSetPose,
    gz::sim::System,
    scoped_set_pose::ScopedSetPose::ISystemConfigure,
    scoped_set_pose::ScopedSetPose::ISystemPostUpdate)

GZ_ADD_PLUGIN_ALIAS(
    scoped_set_pose::ScopedSetPose,
    "scoped_set_pose::ScopedSetPose")
