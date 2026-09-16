/**
 * @file test_sdf_serial_manipulator_loader.cc
 * @brief Standalone test for SdfSerialManipulatorLoader.
 *
 * Loads a serial-manipulator SDF model (e.g. sdf/r820.sdf or sdf/ur3e.sdf) and
 * checks:
 *   1. The kinematic chain has the expected number of revolute joints (when an
 *       expected count is given), all acting about +z (RZ), with matching DOF
 *       and limit counts.
 *   2. model.fkm(q) matches an independent 4x4 matrix chain built from the
 *       same SDF poses (so the test is not circular: the reference does not
 *       use the loader's DQ conversion).
 *
 * Usage: test_sdf_serial_manipulator_loader <model.sdf> [expected_joint_count]
 */

#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>

#include <Eigen/Dense>

#include <gz/math/Pose3.hh>
#include <sdf/Joint.hh>
#include <sdf/JointAxis.hh>
#include <sdf/Link.hh>
#include <sdf/Model.hh>
#include <sdf/ParserConfig.hh>
#include <sdf/Root.hh>
#include <sdf/World.hh>

#include <sas_robot_driver_gazebo/SdfSerialManipulatorLoader.h>

namespace
{

struct ChainLink
{
  gz::math::Pose3d joint_pose;  // joint pose rel. to parent link
  gz::math::Pose3d link_pose;   // child link pose rel. to joint
  bool revolute = true;
};

// Walk the SDF chain collecting the raw poses for the independent reference.
std::vector<ChainLink> CollectChain(const sdf::Model *model)
{
  const sdf::Link *base = model->CanonicalLink();
  std::vector<ChainLink> chain;
  std::string current = base->Name();
  const size_t maxSteps = model->JointCount() + 1;
  for (size_t step = 0; step < maxSteps; ++step)
  {
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
    if (!joint)
    {
      break;
    }
    const sdf::Link *child = model->LinkByName(joint->ChildName());
    ChainLink cl;
    cl.joint_pose = joint->RawPose();
    cl.link_pose = child->RawPose();
    cl.revolute =
      joint->Type() == sdf::JointType::REVOLUTE ||
      joint->Type() == sdf::JointType::CONTINUOUS;
    chain.push_back(cl);
    current = joint->ChildName();
  }
  return chain;
}

// Resolve a "::"-separated scope to the kinematic model it names within a
// loaded root. Works for both model files (scope == model name) and world
// files (nested models reached through <include> chains).
namespace
{
const sdf::Model *FindModelIn(const sdf::Model *top, const std::string &top_scope,
  const std::string &scope)
{
  if (top == nullptr)
  {
    return nullptr;
  }
  if (top_scope == scope)
  {
    return top;
  }
  for (uint64_t i = 0; i < top->ModelCount(); ++i)
  {
    const sdf::Model *child = top->ModelByIndex(i);
    if (child == nullptr)
    {
      continue;
    }
    const sdf::Model *found = FindModelIn(
      child, top_scope + "::" + child->Name(), scope);
    if (found != nullptr)
    {
      return found;
    }
  }
  return nullptr;
}
}  // namespace

const sdf::Model *FindModelByScope(const sdf::Root &root, const std::string &scope)
{
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
      if (top == nullptr)
      {
        continue;
      }
      const sdf::Model *found = FindModelIn(top, top->Name(), scope);
      if (found != nullptr)
      {
        return found;
      }
    }
  }
  if (root.WorldCount() == 0 && root.Model() != nullptr)
  {
    return FindModelIn(root.Model(), root.Model()->Name(), scope);
  }
  return nullptr;
}

Eigen::Matrix3d RotFromQuat(const gz::math::Quaterniond &q)
{
  const double w = q.W();
  const double x = q.X();
  const double y = q.Y();
  const double z = q.Z();
  Eigen::Matrix3d m;
  m(0, 0) = 1 - 2 * (y * y + z * z);
  m(0, 1) = 2 * (x * y - z * w);
  m(0, 2) = 2 * (x * z + y * w);
  m(1, 0) = 2 * (x * y + z * w);
  m(1, 1) = 1 - 2 * (x * x + z * z);
  m(1, 2) = 2 * (y * z - x * w);
  m(2, 0) = 2 * (x * z - y * w);
  m(2, 1) = 2 * (y * z + x * w);
  m(2, 2) = 1 - 2 * (x * x + y * y);
  return m;
}

Eigen::Matrix4d ToMatrix(const gz::math::Pose3d &pose)
{
  Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
  m.block<3, 3>(0, 0) = RotFromQuat(pose.Rot());
  m(0, 3) = pose.Pos().X();
  m(1, 3) = pose.Pos().Y();
  m(2, 3) = pose.Pos().Z();
  return m;
}

// Independent world pose of the final child-link frame for configuration q.
Eigen::Matrix4d IndependentFkm(
  const sdf::Model *model, const std::vector<ChainLink> &chain,
  const Eigen::VectorXd &q)
{
  const sdf::Link *base = model->CanonicalLink();
  Eigen::Matrix4d world = ToMatrix(base->RawPose());
  for (size_t i = 0; i < chain.size(); ++i)
  {
    world = world * ToMatrix(chain[i].joint_pose);
    if (chain[i].revolute)
    {
      // Rotation about the +z axis of the joint frame.
      Eigen::Matrix4d act = Eigen::Matrix4d::Identity();
      act.block<3, 3>(0, 0) =
        Eigen::AngleAxisd(q(i), Eigen::Vector3d::UnitZ()).toRotationMatrix();
      world = world * act;
    }
    else
    {
      // Translation along the +z axis of the joint frame.
      Eigen::Matrix4d act = Eigen::Matrix4d::Identity();
      act(2, 3) = q(i);
      world = world * act;
    }
    world = world * ToMatrix(chain[i].link_pose);
  }
  return world;
}

int g_failures = 0;

void Check(bool cond, const std::string &msg)
{
  if (cond)
  {
    std::cout << "  PASS: " << msg << "\n";
  }
  else
  {
    std::cout << "  FAIL: " << msg << "\n";
    ++g_failures;
  }
}

}  // namespace

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "Usage: test_sdf_serial_manipulator_loader <model.sdf> "
                "[expected_joint_count]\n";
    return 2;
  }

  const std::string path = argv[1];
  // Optional: expected number of joints in the chain.
  long expected_joints = 0;  // 0 => do not check the count.
  if (argc > 2)
  {
    expected_joints = std::stoul(argv[2]);
    if (expected_joints <= 0)
    {
      std::cerr << "Invalid expected joint count: " << argv[2] << "\n";
      return 2;
    }
  }

  std::cout << "Loading " << path << "\n";
  const sas_robot_driver_gazebo::SdfManipulatorResult result =
    sas_robot_driver_gazebo::SdfSerialManipulatorLoader().LoadFromFile(path);

  const auto &model = result.model;
  const size_t n = result.actuation_type_names.size();

  std::cout << "Loaded " << n << " joints from scope '" << result.source_scope
            << "'.\n";

  // 1. Structure.
  if (expected_joints > 0)
  {
    Check(n == static_cast<size_t>(expected_joints),
      "model has the expected " + std::to_string(expected_joints) +
      " joints");
  }
  bool allRz = true;
  for (const auto &name : result.actuation_type_names)
  {
    if (name != "RZ")
    {
      allRz = false;
    }
  }
  Check(allRz, "all joints act about +z (RZ)");
  Check(model.get_dim_configuration_space() == static_cast<int>(n),
    "configuration space matches joint count");
  Check(result.lower_limits.size() == static_cast<long>(n) &&
        result.upper_limits.size() == static_cast<long>(n),
    "lower and upper limits match joint count");

  // 3. FKM cross-check against an independent 4x4 matrix chain. The reference
  // model is resolved via the reported source scope, which works for both
  // model files (scope == model name) and world files (nested scope).
  const std::filesystem::path fs_path(path);
  const std::string base_dir =
    fs_path.is_relative() ? std::string(".")
                          : fs_path.parent_path().string();
  sdf::ParserConfig config;
  config.SetFindCallback([base_dir](const std::string &uri) -> std::string {
    std::string name = uri;
    const std::size_t pos = name.find("://");
    if (pos != std::string::npos)
    {
      name = name.substr(pos + 3);
    }
    const std::filesystem::path candidate =
      std::filesystem::path(base_dir) / name;
    return std::filesystem::exists(candidate) ? candidate.string()
                                              : std::string();
  });
  sdf::Root root;
  (void)root.Load(path, config);
  const sdf::Model *sdfModel = FindModelByScope(root, result.source_scope);
  if (sdfModel == nullptr)
  {
    std::cerr << "Failed to re-resolve reference model scope '"
              << result.source_scope << "' in " << path << "\n";
    return 1;
  }
  const std::vector<ChainLink> chain = CollectChain(sdfModel);

  std::mt19937 rng(12345);
  std::uniform_real_distribution<double> dist(-1.0, 1.0);
  double maxErr = 0.0;
  for (int trial = 0; trial < 10; ++trial)
  {
    Eigen::VectorXd q(static_cast<Eigen::Index>(n));
    for (size_t i = 0; i < n; ++i)
    {
      q(i) = dist(rng);
    }
    const DQ_robotics::DQ x = model.fkm(q);
    const Eigen::Matrix4d M = IndependentFkm(sdfModel, chain, q);
    const auto t = x.translation().vec3();
    const Eigen::Vector3d tRef(M(0, 3), M(1, 3), M(2, 3));
    const double posErr = (t - tRef).norm();
    maxErr = std::max(maxErr, posErr);
    Check(posErr < 1e-8,
      "fkm translation matches independent chain (trial " +
      std::to_string(trial) + ", err=" + std::to_string(posErr) + ")");
  }

  // Rotation cross-check on a single configuration.
  {
    Eigen::VectorXd q(static_cast<Eigen::Index>(n));
    for (size_t i = 0; i < n; ++i)
    {
      q(i) = dist(rng);
    }
    const DQ_robotics::DQ x = model.fkm(q);
    const Eigen::Matrix4d M = IndependentFkm(sdfModel, chain, q);
    const Eigen::Matrix3d Rref = M.block<3, 3>(0, 0);
    // DQ::vec4() returns [w, x, y, z]; gz-math Quaterniond(w,x,y,z).
    const auto rq = x.rotation().vec4();
    const gz::math::Quaterniond qd(rq(0), rq(1), rq(2), rq(3));
    const Eigen::Matrix3d Rdq = RotFromQuat(qd);
    const double cosErr = (Rref * Rdq.transpose()).trace() / 3.0;
    Check(cosErr > 1.0 - 1e-8,
      "fkm rotation matches independent chain (cosErr=" +
      std::to_string(cosErr) + ")");
  }

  std::cout << "\nMax FKM position error over trials: " << maxErr << "\n";
  if (g_failures == 0)
  {
    std::cout << "ALL TESTS PASSED\n";
    return 0;
  }
  std::cout << g_failures << " TEST(S) FAILED\n";
  return 1;
}
