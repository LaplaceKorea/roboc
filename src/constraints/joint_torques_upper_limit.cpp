#include "idocp/constraints/joint_torques_upper_limit.hpp"

#include <cassert>


namespace idocp {

JointTorquesUpperLimit::JointTorquesUpperLimit(
    const Robot& robot, const double barrier, 
    const double fraction_to_boundary_rate)
  : ConstraintComponentBase(barrier, fraction_to_boundary_rate),
    dimc_(robot.jointEffortLimit().size()),
    umax_(robot.jointEffortLimit()) {
}


JointTorquesUpperLimit::JointTorquesUpperLimit()
  : ConstraintComponentBase(),
    dimc_(0),
    umax_() {
}


JointTorquesUpperLimit::~JointTorquesUpperLimit() {
}


bool JointTorquesUpperLimit::useKinematics() const {
  return false;
}


KinematicsLevel JointTorquesUpperLimit::kinematicsLevel() const {
  return KinematicsLevel::AccelerationLevel;
}


bool JointTorquesUpperLimit::isFeasible(Robot& robot, 
                                        ConstraintComponentData& data, 
                                        const SplitSolution& s) const {
  for (int i=0; i<dimc_; ++i) {
    if (s.u.coeff(i) > umax_.coeff(i)) {
      return false;
    }
  }
  return true;
}


void JointTorquesUpperLimit::setSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s) const {
  data.slack = umax_ - s.u;
  setSlackAndDualPositive(data);
}


void JointTorquesUpperLimit::augmentDualResidual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  assert(dtau > 0);
  kkt_residual.lu().noalias() += dtau * data.dual;
}


void JointTorquesUpperLimit::condenseSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) const {
  assert(dtau > 0);
  kkt_matrix.Quu().diagonal().array()
      += dtau * data.dual.array() / data.slack.array();
  computePrimalAndDualResidual(robot, data, s);
  kkt_residual.lu().array() 
      += dtau * (data.dual.array()*data.residual.array()-data.duality.array()) 
              / data.slack.array();
}


void JointTorquesUpperLimit::computeSlackAndDualDirection(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s, 
    const SplitDirection& d) const {
  data.dslack = - d.du() - data.residual;
  computeDualDirection(data);
}


void JointTorquesUpperLimit::computePrimalAndDualResidual(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s) const {
  data.residual = s.u - umax_ + data.slack;
  computeDuality(data);
}


int JointTorquesUpperLimit::dimc() const {
  return dimc_;
}

} // namespace idocp