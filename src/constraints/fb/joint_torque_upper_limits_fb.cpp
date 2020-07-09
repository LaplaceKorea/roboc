#include "constraints/fb/joint_torque_upper_limits_fb.hpp"
#include "constraints/fb/fb_func.hpp"

#include <cmath>
#include <assert.h>


namespace idocp {
namespace fb {

JointTorqueUpperLimits::JointTorqueUpperLimits(const Robot& robot, 
                                               const double barrier) 
  : dimq_(robot.dimq()),
    dimv_(robot.dimv()),
    dimc_(robot.jointEffortLimit().size()),
    barrier_(barrier),
    umax_(robot.jointEffortLimit()),
    slack_(umax_-Eigen::VectorXd::Constant(umax_.size(), barrier)),
    dual_(Eigen::VectorXd::Constant(umax_.size(), barrier)),
    residual_(Eigen::VectorXd::Zero(umax_.size())),
    dslack_(Eigen::VectorXd::Zero(umax_.size())), 
    ddual_(Eigen::VectorXd::Zero(umax_.size())),
    fb_residual_(Eigen::VectorXd::Zero(umax_.size())),
    slack_tilde_(Eigen::VectorXd::Zero(umax_.size())),
    dual_tilde_(Eigen::VectorXd::Zero(umax_.size())) {
  assert(barrier_ > 0);
  assert(umax_.minCoeff() > 0);
}


bool JointTorqueUpperLimits::isFeasible(const Robot& robot, 
                                        const Eigen::VectorXd& u) {
  assert(u.size() == robot.dimv());
  for (int i=0; i<dimc_; ++i) {
    if (u.coeff(i) > umax_.coeff(i)) {
      return false;
    }
  }
  return true;
}


void JointTorqueUpperLimits::setSlackAndDual(const Robot& robot, 
                                             const double dtau, 
                                             const Eigen::VectorXd& u) {
  assert(dtau > 0);
  assert(u.size() == robot.dimv());
  slack_ = dtau * (umax_-u);
  fbfunc::SetSlackAndDualPositive(dimc_, barrier_, slack_, dual_);
}


void JointTorqueUpperLimits::condenseSlackAndDual(const Robot& robot, 
                                                  const double dtau, 
                                                  const Eigen::VectorXd& u, 
                                                  Eigen::MatrixXd& Cuu, 
                                                  Eigen::VectorXd& Cu) {
  assert(dtau > 0);
  assert(Cuu.rows() == robot.dimv());
  assert(Cuu.cols() == robot.dimv());
  assert(Cu.size() == robot.dimv());
  fbfunc::ComputeFischerBurmeisterRadius(dimc_, barrier_, slack_, dual_, fb_residual_);
  slack_tilde_.array() = 1 - slack_.array() / fb_residual_.array();
  dual_tilde_.array() = 1 - dual_.array() / fb_residual_.array();
  fb_residual_.noalias() -= slack_;
  fb_residual_.noalias() -= dual_;
  for (int i=0; i<dimv_; ++i) {
    Cuu.coeffRef(i, i) += dtau * dtau * slack_tilde_.coeff(i) / dual_tilde_.coeff(i);
  }
  residual_ = dtau * (u-umax_) + slack_;
  Cu.array() += dtau * slack_tilde_.array() * residual_.array() / dual_tilde_.array();
  Cu.array() += dtau * fb_residual_.array() / dual_tilde_.array();
}


void JointTorqueUpperLimits::computeSlackAndDualDirection(
    const Robot& robot, const double dtau, const Eigen::VectorXd& du) {
  assert(dtau > 0);
  assert(du.size() == robot.dimv());
  dslack_ = - dtau * du - residual_;
  fbfunc::ComputeDualDirection(dual_tilde_, slack_tilde_, dslack_, fb_residual_, ddual_);
}


double JointTorqueUpperLimits::maxSlackStepSize(const double margin_rate) {
  assert(margin_rate > 0);
  return fbfunc::FractionToBoundary(dimc_, margin_rate, slack_, dslack_);
}


double JointTorqueUpperLimits::maxDualStepSize(const double margin_rate) {
  assert(margin_rate > 0);
  return fbfunc::FractionToBoundary(dimc_, margin_rate, dual_, ddual_);
}


void JointTorqueUpperLimits::updateSlack(const double step_size) {
  assert(step_size > 0);
  slack_.noalias() += step_size * dslack_;
}


void JointTorqueUpperLimits::updateDual(const double step_size) {
  assert(step_size > 0);
  dual_.noalias() += step_size * ddual_;
}


double JointTorqueUpperLimits::costSlackBarrier() {
  return fbfunc::SlackBarrierCost(dimc_, barrier_, slack_);
}


double JointTorqueUpperLimits::costSlackBarrier(const double step_size) {
  return fbfunc::SlackBarrierCost(dimc_, barrier_, slack_+step_size*dslack_);
}


void JointTorqueUpperLimits::augmentDualResidual(const Robot& robot, 
                                                 const double dtau,
                                                 Eigen::VectorXd& Cu) {
  assert(dtau > 0);
  assert(Cu.size() == robot.dimv());
  Cu.noalias() += dtau * dual_;
}


double JointTorqueUpperLimits::residualL1Nrom(const Robot& robot, 
                                              const double dtau, 
                                              const Eigen::VectorXd& u) {
  assert(dtau > 0);
  assert(u.size() == robot.dimv());
  residual_ = dtau * (u-umax_) + slack_;
  return residual_.lpNorm<1>();
}


double JointTorqueUpperLimits::residualSquaredNrom(const Robot& robot, 
                                                   const double dtau, 
                                                   const Eigen::VectorXd& u) {
  assert(dtau > 0);
  assert(u.size() == robot.dimv());
  residual_ = dtau * (u-umax_) + slack_;
  double error = 0;
  error += residual_.squaredNorm();
  fbfunc::ComputeFischerBurmeisterRadius(dimc_, barrier_, slack_, dual_, residual_);
  residual_.noalias() -= slack_;
  residual_.noalias() -= dual_;
  error += residual_.squaredNorm();
  return error;
}

} // namespace fb 
} // namespace idocp