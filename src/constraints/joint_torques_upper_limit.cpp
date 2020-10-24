#include "idocp/constraints/joint_torques_upper_limit.hpp"

#include <assert.h>


namespace idocp {

JointTorquesUpperLimit::JointTorquesUpperLimit(
    const Robot& robot, const double barrier, 
    const double fraction_to_boundary_rate)
  : ConstraintComponentBase(barrier, fraction_to_boundary_rate),
    dimc_(robot.jointEffortLimit().size()),
    dim_passive_(robot.dim_passive()),
    umax_(robot.jointEffortLimit().tail(robot.dimv()-robot.dim_passive())) {
}


JointTorquesUpperLimit::JointTorquesUpperLimit()
  : ConstraintComponentBase(),
    dimc_(0),
    dim_passive_(0),
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
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s) const {
  assert(dtau > 0);
  data.slack = dtau * (umax_-s.u);
  setSlackAndDualPositive(data);
}


void JointTorquesUpperLimit::augmentDualResidual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, KKTResidual& kkt_residual) const {
  kkt_residual.lu().noalias() += dtau * data.dual;
}


void JointTorquesUpperLimit::condenseSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, KKTMatrix& kkt_matrix, 
    KKTResidual& kkt_residual) const {
  kkt_matrix.Quu().diagonal().array()
      += dtau * dtau * data.dual.array() / data.slack.array();
  computePrimalAndDualResidual(robot, data, dtau, s);
  kkt_residual.lu().array() 
      += dtau * (data.dual.array()*data.residual.array()-data.duality.array()) 
              / data.slack.array();
}


void JointTorquesUpperLimit::computeSlackAndDualDirection(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, const SplitDirection& d) const {
  data.dslack = - dtau * d.du() - data.residual;
  computeDualDirection(data);
}


void JointTorquesUpperLimit::computePrimalAndDualResidual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s) const {
  data.residual = dtau * (s.u-umax_) + data.slack;
  computeDuality(data);
}


int JointTorquesUpperLimit::dimc() const {
  return dimc_;
}

} // namespace idocp