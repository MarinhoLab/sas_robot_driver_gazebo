#pragma once
/**
 * @file SdfSerialManipulatorLoader.h
 * @brief Loads an M3_SerialManipulatorSimulatorFriendly kinematic model from a
 *        Gazebo-SDF-compatible model description.
 *
 * A serial-manipulator SDF model (one base link followed by a chain of
 * joint/link pairs, each joint acting about the +z axis of its joint frame)
 * maps one-to-one onto the three per-joint vectors that define an
 * M3_SerialManipulatorSimulatorFriendly:
 *
 *   joint <pose>        (rel. to parent link) -> offset_before_[i]
 *   joint <type>+<axis> (about +z)           -> actuation_types_[i]
 *   child link <pose>   (rel. to joint)      -> offset_after_[i]
 *   base link <pose>    (world)             -> reference_frame_
 *
 * The conversion from an SDF pose to a dual quaternion is a homomorphism with
 * respect to dqrobotics' DQ::operator* (verified against gz::math::Pose3d
 * composition and an independent 4x4 matrix chain).
 */

#include <string>
#include <vector>

#include <Eigen/Dense>

#include <gz/math/Pose3.hh>
#include <sdf/Model.hh>
#include <sdf/World.hh>

#include <dqrobotics/DQ.h>

#include "M3_SerialManipulatorSimulatorFriendly.h"

namespace sas_robot_driver_gazebo
{

/**
 * @brief Result of loading a serial manipulator from SDF.
 *
 * @details Contains the fully-constructed kinematic model (joint limits and
 * reference frame already applied) plus the names used to build it, so callers
 * can correlate the i-th joint of the model with its SDF name.
 */
struct SdfManipulatorResult
{
    /** @brief The constructed model. */
    DQ_robotics::M3_SerialManipulatorSimulatorFriendly model;

    /** @brief Name of each SDF joint, in chain order (parent -> child). */
    std::vector<std::string> joint_names;

    /** @brief Name of each SDF child link, in chain order. */
    std::vector<std::string> link_names;

    /** @brief Actuation type string per joint: "RZ" or "TZ". */
    std::vector<std::string> actuation_type_names;

    /** @brief Per-joint pre-actuation offset DQs, in chain order. */
    std::vector<DQ_robotics::DQ> offsets_before;

    /** @brief Per-joint post-actuation offset DQs, in chain order. */
    std::vector<DQ_robotics::DQ> offsets_after;

    /** @brief Lower joint limits, in chain order. */
    Eigen::VectorXd lower_limits;

    /** @brief Upper joint limits, in chain order. */
    Eigen::VectorXd upper_limits;

    /** @brief Path of the SDF file the model was loaded from. */
    std::string source_path;

    /**
     * @brief Name of the kinematic model used to build the result.
     *
     * For a model file this is the model's name; for a world file it is the
     * "::"-separated chain of nested model names from the world's top-level
     * model down to the kinematic model (e.g.
     * "ur3e::ur3e_position_controller::ur3e").
     */
    std::string source_scope;
};

/**
 * @brief Builds an M3_SerialManipulatorSimulatorFriendly from an SDF model.
 *
 * @details The loader expects a strictly serial kinematic chain: a base (the
 * model's canonical) link, then a sequence of joints each connecting the
 * previous link to a new link, every joint acting about the +z axis of its
 * joint frame (so every actuation is RZ for revolute/continuous joints and TZ
 * for prismatic joints). Non-serial topologies, unsupported joint types, or
 * off-axis actuation axes raise std::runtime_error.
 */
class SdfSerialManipulatorLoader
{
public:
    /**
     * @brief Load a serial manipulator from an SDF model or world file.
     *
     * @details This is the recommended entry point. It transparently handles
     * both Gazebo-SDF files:
     *  - A <model> file: the model is used directly (equivalent to
     *    LoadFromSdfFile).
     *  - A <world> file (scene): the kinematic model is located within the
     *    world, including models nested inside other models through
     *    <include> chains (e.g. a robot model wrapped in a position-controller
     *    model inside a world). The kinematic model is the one (anywhere in
     *    the world's model tree) that has both links and joints; models with
     *    only links (static frames, ground planes) or only fixed joints
     *    (wrapper models) are skipped.
     *
     * Relative <include> URIs are resolved against the directory of @p path,
     * so the file's location is sufficient (no current-directory dependency).
     *
     * @param path Path to a Gazebo-SDF model or world file.
     * @param model_scope Optional "::"-separated chain of model names (from a
     *         top-level model in the world down to the target) that selects
     *         which model to load. Empty: automatic detection (the single
     *         model in the tree that has both links and joints). If several
     *         candidate models exist, the most deeply nested one is used and
     *         result.source_scope reports the selection; pass model_scope to
     *         disambiguate explicitly.
     * @return The loaded model plus joint/link names and the source scope
     *         that produced it.
     * @throws std::runtime_error if the file cannot be parsed, no kinematic
     *         model is found, model_scope does not match, the selected model
     *         is not a serial manipulator, or a joint does not act about +z.
     */
    SdfManipulatorResult LoadFromFile(
      const std::string &path,
      const std::string &model_scope = std::string()) const;

    /**
     * @brief Parse an SDF model file and build the kinematic model.
     * @param path Path to a Gazebo-SDF file whose <model> is a serial manipulator.
     * @return The loaded model and its joint/link names.
     * @throws std::runtime_error if the file cannot be parsed, is not a serial
     *         manipulator, or a joint does not act about the +z axis.
     */
    SdfManipulatorResult LoadFromSdfFile(const std::string &path) const;

    /**
     * @brief Build the kinematic model from an already-loaded SDF model.
     * @param model Pointer to a loaded sdf::Model (must not be null).
     * @return The loaded model and its joint/link names.
     * @throws std::runtime_error on structural or actuation violations.
     */
    SdfManipulatorResult LoadFromModel(const sdf::Model *model) const;
};

}  // namespace sas_robot_driver_gazebo
