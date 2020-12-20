#include "idocp/constraints/joint_position_upper_limit.hpp"

#include <cassert>


namespace idocp {

JointPositionUpperLimit::JointPositionUpperLimit(
    const Robot& robot, const double barrier, 
    const double fraction_to_boundary_rate)
  : ConstraintComponentBase(barrier, fraction_to_boundary_rate),
    dimc_(robot.lowerJointPositionLimit().size()),
    dim_passive_(robot.dim_passive()),
    qmax_(robot.upperJointPositionLimit()) {
}


JointPositionUpperLimit::JointPositionUpperLimit()
  : ConstraintComponentBase(),
    dimc_(0),
    dim_passive_(0),
    qmax_() {
}


JointPositionUpperLimit::~JointPositionUpperLimit() {
}


bool JointPositionUpperLimit::useKinematics() const {
  return false;
}

KinematicsLevel JointPositionUpperLimit::kinematicsLevel() const {
  return KinematicsLevel::PositionLevel;
}


bool JointPositionUpperLimit::isFeasible(Robot& robot, 
                                         ConstraintComponentData& data, 
                                         const SplitSolution& s) const {
  for (int i=0; i<dimc_; ++i) {
    if (s.q.tail(dimc_).coeff(i) > qmax_.coeff(i)) {
      return false;
    }
  }
  return true;
}


void JointPositionUpperLimit::setSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s) const {
  data.slack = qmax_ - s.q.tail(dimc_);
  setSlackAndDualPositive(data);
}


void JointPositionUpperLimit::augmentDualResidual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  assert(dtau > 0);
  kkt_residual.lq().tail(dimc_).noalias() += dtau * data.dual;
}


void JointPositionUpperLimit::condenseSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) const {
  assert(dtau > 0);
  kkt_matrix.Qqq().diagonal().tail(dimc_).array()
      += dtau * data.dual.array() / data.slack.array();
  computePrimalAndDualResidual(robot, data, s);
  kkt_residual.lq().tail(dimc_).array() 
      += dtau * (data.dual.array()*data.residual.array()-data.duality.array()) 
              / data.slack.array();
}


void JointPositionUpperLimit::computeSlackAndDualDirection(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s, 
    const SplitDirection& d) const {
  data.dslack = - d.dq().tail(dimc_) - data.residual;
  computeDualDirection(data);
}


void JointPositionUpperLimit::computePrimalAndDualResidual(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s) const {
  data.residual = s.q.tail(dimc_) - qmax_ + data.slack;
  computeDuality(data);
}


int JointPositionUpperLimit::dimc() const {
  return dimc_;
}

} // namespace idocp