#include "idocp/constraints/joint_velocity_upper_limit.hpp"


namespace idocp {

JointVelocityUpperLimit::JointVelocityUpperLimit(
    const Robot& robot, const double barrier, 
    const double fraction_to_boundary_rate)
  : ConstraintComponentBase(barrier, fraction_to_boundary_rate),
    dimc_(robot.jointVelocityLimit().size()),
    vmax_(robot.jointVelocityLimit()) {
}


JointVelocityUpperLimit::JointVelocityUpperLimit()
  : ConstraintComponentBase(),
    dimc_(0),
    vmax_() {
}


JointVelocityUpperLimit::~JointVelocityUpperLimit() {
}


bool JointVelocityUpperLimit::useKinematics() const {
  return false;
}


KinematicsLevel JointVelocityUpperLimit::kinematicsLevel() const {
  return KinematicsLevel::VelocityLevel;
}


bool JointVelocityUpperLimit::isFeasible(Robot& robot, 
                                         ConstraintComponentData& data, 
                                         const SplitSolution& s) const {
  for (int i=0; i<dimc_; ++i) {
    if (s.v.tail(dimc_).coeff(i) > vmax_.coeff(i)) {
      return false;
    }
  }
  return true;
}


void JointVelocityUpperLimit::setSlack(Robot& robot, 
                                       ConstraintComponentData& data, 
                                       const SplitSolution& s) const {
  data.slack = vmax_ - s.v.tail(dimc_);
}


void JointVelocityUpperLimit::computePrimalAndDualResidual(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s) const {
  data.residual = s.v.tail(dimc_) - vmax_ + data.slack;
  computeDuality(data);
}


void JointVelocityUpperLimit::computePrimalResidualDerivatives(
    Robot& robot, ConstraintComponentData& data, const double dt, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  kkt_residual.lv().tail(dimc_).noalias() += dt * data.dual;
}


void JointVelocityUpperLimit::condenseSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const double dt, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) const {
  kkt_matrix.Qvv().diagonal().tail(dimc_).array()
      += dt * data.dual.array() / data.slack.array();
  kkt_residual.lv().tail(dimc_).array() 
      += dt * (data.dual.array()*data.residual.array()-data.duality.array()) 
              / data.slack.array();
}


void JointVelocityUpperLimit::expandSlackAndDual(
    ConstraintComponentData& data, const SplitSolution& s, 
    const SplitDirection& d) const {
  data.dslack = - d.dv().tail(dimc_) - data.residual;
  computeDualDirection(data);
}


int JointVelocityUpperLimit::dimc() const {
  return dimc_;
}

} // namespace idocp