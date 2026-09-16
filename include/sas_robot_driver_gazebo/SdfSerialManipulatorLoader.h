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
     * @brief Parse an SDF file and build the kinematic model.
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
