/**
 * @file SdfSerialManipulatorLoader.cc
 * @brief Implementation of SdfSerialManipulatorLoader.
 */

#include <sas_robot_driver_gazebo/SdfSerialManipulatorLoader.h>

#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <gz/math/Pose3.hh>

#include <sdf/Error.hh>
#include <sdf/Joint.hh>
#include <sdf/JointAxis.hh>
#include <sdf/Link.hh>
#include <sdf/Model.hh>
#include <sdf/ParserConfig.hh>
#include <sdf/Root.hh>
#include <sdf/World.hh>

namespace sas_robot_driver_gazebo
{

namespace
{
constexpr double kAxisTolerance = 1e-6;

/**
 * @brief Strip a leading URI scheme (e.g. "model://", "file://") from a string.
 *
 * @param uri The input (may or may not carry a scheme).
 * @return The path portion after the scheme, or the input unchanged if it has
 *         no "://" separator.
 */
std::string StripScheme(const std::string &uri)
{
  const std::size_t pos = uri.find("://");
  return pos == std::string::npos ? uri : uri.substr(pos + 3);
}

/**
 * @brief Build a ParserConfig that resolves <include> URIs against @p base_dir.
 *
 * @details libsdformat's default file lookup resolves relative include paths
 * against the *current working directory*, which makes loading a scene file
 * location-dependent. This callback instead resolves every include against the
 * directory containing the loaded file, so a scene/model file can be loaded
 * from anywhere. Schemes are stripped first so "model://foo.sdf" and the
 * relative "foo.sdf" are treated the same. If the file is not found in the
 * base directory the callback returns empty, letting libsdformat fall back to
 * its normal resolution (URI path map, GZ_SIM_RESOURCE_PATH, ...).
 */
sdf::ParserConfig MakeParserConfig(const std::string &base_dir)
{
  sdf::ParserConfig config;
  config.SetFindCallback([base_dir](const std::string &uri) -> std::string {
    const std::string name = StripScheme(uri);
    if (name.empty())
    {
      return std::string();
    }
    const std::filesystem::path candidate = std::filesystem::path(base_dir) / name;
    return std::filesystem::exists(candidate) ? candidate.string() : std::string();
  });
  return config;
}

/**
 * @brief A kinematic model candidate found in a loaded SDF tree.
 */
struct ModelCandidate
{
  std::string scope;  // "::"-separated path of model names to this model
  const sdf::Model *model;
};

/**
 * @brief Recursively collect candidate kinematic models.
 *
 * A model is a candidate iff it directly owns both links and joints (the
 * signature of a kinematic body). Wrapper models (an <include> plus a fixed
 * base joint, no links) and static models (links but no joints) are skipped,
 * as are their ancestors.
 */
void CollectModelCandidates(
  const sdf::Model *model, const std::string &scope,
  std::vector<ModelCandidate> &candidates)
{
  if (model == nullptr)
  {
    return;
  }
  if (model->LinkCount() > 0 && model->JointCount() > 0)
  {
    candidates.push_back(ModelCandidate{scope, model});
  }
  const uint64_t count = model->ModelCount();
  for (uint64_t i = 0; i < count; ++i)
  {
    const sdf::Model *child = model->ModelByIndex(i);
    const std::string child_scope = scope + "::" + child->Name();
    CollectModelCandidates(child, child_scope, candidates);
  }
}

/**
 * @brief Collect candidate kinematic models from a loaded sdf::Root.
 *
 * Handles both file kinds: a world (its top-level models are walked) and a
 * bare model file (root.Model()).
 */
std::vector<ModelCandidate> CollectAllCandidates(const sdf::Root &root)
{
  std::vector<ModelCandidate> candidates;
  for (uint64_t i = 0; i < root.WorldCount(); ++i)
  {
    const sdf::World *world = root.WorldByIndex(i);
    if (world == nullptr)
    {
      continue;
    }
    for (uint64_t k = 0; k < world->ModelCount(); ++k)
    {
      const sdf::Model *top = world->ModelByIndex(k);
      if (top != nullptr)
      {
        CollectModelCandidates(top, top->Name(), candidates);
      }
    }
  }
  if (root.WorldCount() == 0 && root.Model() != nullptr)
  {
    CollectModelCandidates(root.Model(), root.Model()->Name(), candidates);
  }
  return candidates;
}

/**
 * @brief Convert an SDF pose to a unit dual quaternion.
 *
 * @details This is a homomorphism with respect to dqrobotics' DQ::operator*:
 * poseToDq(A) * poseToDq(B) == poseToDq(A * B) for gz::math::Pose3d products,
 * which is the property the M3 model relies on when it multiplies the
 * per-joint offset/actuation dual quaternions. The translation is expressed
 * in the pose's own (rotated) frame before forming the dual part.
 */
DQ_robotics::DQ PoseToDq(const gz::math::Pose3d &pose)
{
  const auto &rot = pose.Rot();
  const auto &trans = pose.Pos();

  const double qw = rot.W();
  const double qx = rot.X();
  const double qy = rot.Y();
  const double qz = rot.Z();

  // Translation expressed in the pose's own frame.
  const gz::math::Vector3d tself = rot.Inverse().RotateVector(trans);
  const double tx = tself.X();
  const double ty = tself.Y();
  const double tz = tself.Z();

  const double dw = -0.5 * (qx * tx + qy * ty + qz * tz);
  const double dx = 0.5 * (qw * tx + qy * tz - qz * ty);
  const double dy = 0.5 * (qw * ty + qz * tx - qx * tz);
  const double dz = 0.5 * (qw * tz + qx * ty - qy * tx);

  return DQ_robotics::DQ(qw, qx, qy, qz, dw, dx, dy, dz);
}

using M3 = DQ_robotics::M3_SerialManipulatorSimulatorFriendly;
using Actuation = M3::ActuationType;

bool IsUnitZ(const gz::math::Vector3d &axis)
{
  return std::abs(axis.X()) < kAxisTolerance &&
         std::abs(axis.Y()) < kAxisTolerance &&
         std::abs(axis.Z() - 1.0) < kAxisTolerance;
}

}  // namespace

SdfManipulatorResult SdfSerialManipulatorLoader::LoadFromFile(
  const std::string &path, const std::string &model_scope) const
{
  const std::filesystem::path fs_path(path);
  const std::string base_dir =
    fs_path.is_relative() ? std::string(".")
                          : fs_path.parent_path().string();

  const sdf::ParserConfig config = MakeParserConfig(base_dir);
  sdf::Root root;
  const sdf::Errors errors = root.Load(path, config);

  const std::vector<ModelCandidate> candidates = CollectAllCandidates(root);

  if (candidates.empty())
  {
    std::string reason =
      "No kinematic model (a model with both links and joints) could be "
      "loaded from SDF file '" + path + "'.";
    for (const auto &err : errors)
    {
      reason += " [" + err.Message() + "]";
    }
    throw std::runtime_error(reason);
  }

  // Select the candidate.
  const ModelCandidate *selected = nullptr;
  if (model_scope.empty())
  {
    // Automatic: unique candidate, or the most deeply nested one (longest
    // scope, i.e. most "::" separators) when several exist.
    for (const auto &c : candidates)
    {
      if (selected == nullptr || c.scope.length() > selected->scope.length())
      {
        selected = &c;
      }
    }
  }
  else
  {
    for (const auto &c : candidates)
    {
      if (c.scope == model_scope)
      {
        selected = &c;
        break;
      }
    }
    if (selected == nullptr)
    {
      std::string reason =
        "Requested model scope '" + model_scope + "' not found in SDF file '" +
        path + "'. Available scopes:";
      for (const auto &c : candidates)
      {
        reason += " [" + c.scope + "]";
      }
      throw std::runtime_error(reason);
    }
  }

  SdfManipulatorResult result = this->LoadFromModel(selected->model);
  result.source_path = path;
  result.source_scope = selected->scope;
  return result;
}

SdfManipulatorResult SdfSerialManipulatorLoader::LoadFromSdfFile(
  const std::string &path) const
{
  const std::filesystem::path fs_path(path);
  const std::string base_dir =
    fs_path.is_relative() ? std::string(".")
                          : fs_path.parent_path().string();

  const sdf::ParserConfig config = MakeParserConfig(base_dir);
  sdf::Root root;
  const sdf::Errors errors = root.Load(path, config);

  const sdf::Model *model = root.Model();
  if (model == nullptr)
  {
    std::string reason =
      std::string("No <model> could be parsed from SDF file '") + path +
      "'.";
    for (const auto &err : errors)
    {
      reason += " [" + err.Message() + "]";
    }
    throw std::runtime_error(reason);
  }

  SdfManipulatorResult result = this->LoadFromModel(model);
  result.source_path = path;
  result.source_scope = model->Name();
  return result;
}

SdfManipulatorResult SdfSerialManipulatorLoader::LoadFromModel(
  const sdf::Model *model) const
{
  if (model == nullptr)
  {
    throw std::runtime_error("LoadFromModel received a null model pointer.");
  }

  // Index links by name.
  std::unordered_map<std::string, const sdf::Link *> linkByName;
  for (uint64_t i = 0; i < model->LinkCount(); ++i)
  {
    const sdf::Link *link = model->LinkByIndex(i);
    if (link)
    {
      linkByName[link->Name()] = link;
    }
  }

  // The base is the canonical link (the one that is never a joint child).
  const sdf::Link *base = model->CanonicalLink();
  if (base == nullptr)
  {
    throw std::runtime_error("Could not determine the base (canonical) link.");
  }

  std::vector<DQ_robotics::DQ> offset_before;
  std::vector<DQ_robotics::DQ> offset_after;
  std::vector<Actuation> actuation_types;
  std::vector<std::string> joint_names;
  std::vector<std::string> link_names;
  std::vector<std::string> actuation_type_names;
  Eigen::VectorXd lower_limits = Eigen::VectorXd::Zero(0);
  Eigen::VectorXd upper_limits = Eigen::VectorXd::Zero(0);

  std::string current = base->Name();
  // Guard against cycles / disconnected chains.
  const size_t maxSteps = model->JointCount() + 1;

  for (size_t step = 0; step < maxSteps; ++step)
  {
    // Find the joint whose parent is the current link.
    const sdf::Joint *joint = nullptr;
    for (uint64_t i = 0; i < model->JointCount(); ++i)
    {
      const sdf::Joint *j = model->JointByIndex(i);
      if (j && j->ParentName() == current)
      {
        joint = j;
        break;
      }
    }
    if (joint == nullptr)
    {
      break;  // End of the serial chain.
    }

    // Enforce the +z actuation-axis requirement.
    const sdf::JointAxis *axis = joint->Axis(0);
    if (axis == nullptr)
    {
      throw std::runtime_error(
        "Joint '" + joint->Name() + "' has no axis specification.");
    }
    if (!axis->XyzExpressedIn().empty())
    {
      throw std::runtime_error(
        "Joint '" + joint->Name() + "' expresses its axis in frame '" +
        axis->XyzExpressedIn() + "'; only the joint-local frame is supported.");
    }
    const gz::math::Vector3d axisVec = axis->Xyz();
    if (!IsUnitZ(axisVec))
    {
      throw std::runtime_error(
        "Joint '" + joint->Name() + "' acts about axis (" +
        std::to_string(axisVec.X()) + ", " + std::to_string(axisVec.Y()) +
        ", " + std::to_string(axisVec.Z()) +
        "); the actuation must be about the +z axis of the joint frame.");
    }

    const sdf::JointType type = joint->Type();
    Actuation act;
    std::string actName;
    switch (type)
    {
      case sdf::JointType::REVOLUTE:
      case sdf::JointType::CONTINUOUS:
        act = Actuation::RZ;
        actName = "RZ";
        break;
      case sdf::JointType::PRISMATIC:
        act = Actuation::TZ;
        actName = "TZ";
        break;
      default:
        throw std::runtime_error(
          "Joint '" + joint->Name() +
          "' has an unsupported joint type; only revolute, continuous and "
          "prismatic joints are supported.");
    }

    actuation_type_names.push_back(actName);

    const std::string &childName = joint->ChildName();
    auto it = linkByName.find(childName);
    if (it == linkByName.end())
    {
      throw std::runtime_error(
        "Child link '" + childName + "' of joint '" + joint->Name() +
        "' was not found in the model.");
    }
    const sdf::Link *child = it->second;

    offset_before.push_back(PoseToDq(joint->RawPose()));
    offset_after.push_back(PoseToDq(child->RawPose()));
    actuation_types.push_back(act);
    joint_names.push_back(joint->Name());
    link_names.push_back(childName);

    const double lower = axis->Lower();
    const double upper = axis->Upper();
    if (lower_limits.size() == 0)
    {
      lower_limits = Eigen::VectorXd(1);
      upper_limits = Eigen::VectorXd(1);
    }
    else
    {
      lower_limits.conservativeResize(lower_limits.size() + 1);
      upper_limits.conservativeResize(upper_limits.size() + 1);
    }
    lower_limits(lower_limits.size() - 1) = lower;
    upper_limits(upper_limits.size() - 1) = upper;

    current = childName;
  }

  if (actuation_types.empty())
  {
    throw std::runtime_error(
      "No serial chain of joints found starting from base link '" +
      base->Name() + "'; the model is not a serial manipulator.");
  }

  M3 manipulator(offset_before, offset_after, actuation_types);

  // Reference frame: pose of the base link in the world.
  manipulator.set_reference_frame(PoseToDq(base->RawPose()));
  manipulator.set_name(model->Name());

  if (lower_limits.size() == static_cast<Eigen::Index>(actuation_types.size()))
  {
    manipulator.set_lower_q_limit(lower_limits);
    manipulator.set_upper_q_limit(upper_limits);
  }

  return SdfManipulatorResult{
    std::move(manipulator),
    std::move(joint_names),
    std::move(link_names),
    std::move(actuation_type_names),
    std::move(offset_before),
    std::move(offset_after),
    lower_limits,
    upper_limits,
  };
}

}  // namespace sas_robot_driver_gazebo
