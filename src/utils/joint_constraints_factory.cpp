#include "roboc/utils/joint_constraints_factory.hpp"

#include "roboc/constraints/joint_position_lower_limit.hpp"
#include "roboc/constraints/joint_position_upper_limit.hpp"
#include "roboc/constraints/joint_velocity_lower_limit.hpp"
#include "roboc/constraints/joint_velocity_upper_limit.hpp"
#include "roboc/constraints/joint_torques_lower_limit.hpp"
#include "roboc/constraints/joint_torques_upper_limit.hpp"


namespace roboc {

JointConstraintsFactory::JointConstraintsFactory(const Robot& robot) 
  : robot_(robot) {
}


JointConstraintsFactory::~JointConstraintsFactory() {
}


std::shared_ptr<roboc::Constraints> JointConstraintsFactory::create() const {
  auto constraints = std::make_shared<roboc::Constraints>();
  auto joint_position_lower = std::make_shared<roboc::JointPositionLowerLimit>(robot_);
  auto joint_position_upper = std::make_shared<roboc::JointPositionUpperLimit>(robot_);
  auto joint_velocity_lower = std::make_shared<roboc::JointVelocityLowerLimit>(robot_);
  auto joint_velocity_upper = std::make_shared<roboc::JointVelocityUpperLimit>(robot_);
  auto joint_torques_lower = std::make_shared<roboc::JointTorquesLowerLimit>(robot_);
  auto joint_torques_upper = std::make_shared<roboc::JointTorquesUpperLimit>(robot_);
  constraints->push_back(joint_position_lower);
  constraints->push_back(joint_position_upper);
  constraints->push_back(joint_velocity_lower);
  constraints->push_back(joint_velocity_upper);
  constraints->push_back(joint_torques_lower);
  constraints->push_back(joint_torques_upper);
  return constraints;
}

} // namespace roboc