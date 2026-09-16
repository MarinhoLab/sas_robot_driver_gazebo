/**
 * @file sdf2manipulator.cc
 * @brief Command-line tool: load an M3_SerialManipulatorSimulatorFriendly from
 *        a Gazebo-SDF model and print it as YAML.
 *
 * The output mirrors the schema consumed by
 * marinholab.working.needlemanipulation.example_load_from_file.
 * get_information_from_file, so it can be diffed against the hand-authored
 * *_robot.yaml files to confirm an SDF encodes the same kinematics.
 */

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include <sas_robot_driver_gazebo/SdfSerialManipulatorLoader.h>

namespace
{

void PrintDqList(const char *key, const std::vector<DQ_robotics::DQ> &v)
{
  std::cout << key << ":\n";
  for (const auto &dq : v)
  {
    const auto vec = dq.vec8();
    std::cout << "- [";
    for (int i = 0; i < 8; ++i)
    {
      std::cout << vec(i);
      if (i < 7)
      {
        std::cout << ", ";
      }
    }
    std::cout << "]\n";
  }
}

void PrintVectorList(const char *key, const Eigen::VectorXd &v)
{
  std::cout << key << ":\n";
  for (int i = 0; i < v.size(); ++i)
  {
    std::cout << "- " << v(i) << "\n";
  }
}

}  // namespace

int main(int argc, char **argv)
{
  std::string path;
  std::string model_scope;
  bool printLimits = false;
  for (int i = 1; i < argc; ++i)
  {
    const std::string arg = argv[i];
    if (arg == "--model-scope" && i + 1 < argc)
    {
      model_scope = argv[++i];
    }
    else if (arg == "--joint-limits")
    {
      printLimits = true;
    }
    else if (path.empty())
    {
      path = arg;
    }
    else
    {
      std::cerr << "Unknown argument: " << arg << "\n";
      std::cerr
        << "Usage: sdf2manipulator <model.sdf|world.sdf> "
           "[--model-scope <a::b::c>] [--joint-limits]\n";
      return 1;
    }
  }
  if (path.empty())
  {
    std::cerr
      << "Usage: sdf2manipulator <model.sdf|world.sdf> "
         "[--model-scope <a::b::c>] [--joint-limits]\n";
    return 1;
  }

  try
  {
    sas_robot_driver_gazebo::SdfSerialManipulatorLoader loader;
    const auto result = loader.LoadFromFile(path, model_scope);

    const auto &model = result.model;

    // Actuation types.
    std::cout << "actuation_types:\n";
    for (const auto &name : result.actuation_type_names)
    {
      std::cout << "- " << name << "\n";
    }

    // Per-joint offsets, in the same order as actuation_types, so the output
    // round-trips through get_information_from_file.
    PrintDqList("offsets_before", result.offsets_before);
    PrintDqList("offsets_after", result.offsets_after);

    std::cout << "joint_names:\n";
    for (const auto &n : result.joint_names)
    {
      std::cout << "- " << n << "\n";
    }

    std::cout << "link_names:\n";
    for (const auto &n : result.link_names)
    {
      std::cout << "- " << n << "\n";
    }

    if (printLimits)
    {
      PrintVectorList("lower_q_limits", result.lower_limits);
      PrintVectorList("upper_q_limits", result.upper_limits);
    }

    // A quick self-check: fkm at q=0 should equal the reference frame composed
    // with all identity actuations. Print its translation for sanity.
    Eigen::VectorXd q0(
      static_cast<Eigen::Index>(result.actuation_type_names.size()));
    q0.setZero();
    const auto x0 = model.fkm(q0);
    const auto t0 = x0.translation().vec3();
    std::cout << "# fkm(q=0) translation: [" << t0(0) << ", " << t0(1) << ", "
              << t0(2) << "]\n";

    std::cout << "# loaded " << result.actuation_type_names.size()
              << " joints from " << result.source_scope
              << " (in " << path << ")\n";
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
