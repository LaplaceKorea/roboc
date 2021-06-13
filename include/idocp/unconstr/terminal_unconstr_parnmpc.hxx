#ifndef IDOCP_TERMINAL_UNCONSTR_PARNMPC_HXX_
#define IDOCP_TERMINAL_UNCONSTR_PARNMPC_HXX_

#include "idocp/unconstr/terminal_unconstr_parnmpc.hpp"

#include <stdexcept>
#include <cassert>

namespace idocp {

inline TerminalUnconstrParNMPC::TerminalUnconstrParNMPC(
    const Robot& robot, const std::shared_ptr<CostFunction>& cost, 
    const std::shared_ptr<Constraints>& constraints) 
  : cost_(cost),
    cost_data_(cost->createCostFunctionData(robot)),
    constraints_(constraints),
    constraints_data_(constraints->createConstraintsData(robot, 0)),
    unconstr_dynamics_(robot),
    use_kinematics_(false),
    stage_cost_(0),
    constraint_violation_(0) {
  if (cost_->useKinematics() || constraints_->useKinematics()) {
    use_kinematics_ = true;
  }
  try {
    if (robot.hasFloatingBase()) {
      throw std::logic_error(
          "robot has floating base: robot should have no constraints!");
    }
    if (robot.maxPointContacts() > 0) {
      throw std::logic_error(
          "robot can have contacts: robot should have no constraints!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
}


inline TerminalUnconstrParNMPC::TerminalUnconstrParNMPC() 
  : cost_(),
    cost_data_(),
    constraints_(),
    constraints_data_(),
    unconstr_dynamics_(),
    use_kinematics_(false),
    stage_cost_(0),
    constraint_violation_(0) {
}


inline TerminalUnconstrParNMPC::~TerminalUnconstrParNMPC() {
}


inline bool TerminalUnconstrParNMPC::isFeasible(Robot& robot, 
                                                const SplitSolution& s) {
  return constraints_->isFeasible(robot, constraints_data_, s);
}


inline void TerminalUnconstrParNMPC::initConstraints(Robot& robot, 
                                                     const int time_step, 
                                                     const SplitSolution& s) { 
  assert(time_step >= 0);
  constraints_data_ = constraints_->createConstraintsData(robot, time_step);
  constraints_->setSlackAndDual(robot, constraints_data_, s);
}


inline void TerminalUnconstrParNMPC::computeKKTResidual(
    Robot& robot, const double t, const double dt, 
    const Eigen::VectorXd& q_prev, const Eigen::VectorXd& v_prev, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  if (use_kinematics_) {
    robot.updateKinematics(s.q);
  }
  kkt_residual.setZero();
  stage_cost_  = cost_->linearizeStageCost(robot, cost_data_, t, dt, s, 
                                           kkt_residual);
  stage_cost_ += cost_->linearizeTerminalCost(robot, cost_data_, t, s, 
                                              kkt_residual);
  constraints_->linearizePrimalAndDualResidual(robot, constraints_data_, dt, s, 
                                               kkt_residual);
  unconstr::stateequation::linearizeBackwardEulerTerminal(dt, q_prev, v_prev, s,  
                                                          kkt_matrix, kkt_residual);
  unconstr_dynamics_.linearizeUnconstrDynamics(robot, dt, s, kkt_residual);
}


inline void TerminalUnconstrParNMPC::computeKKTSystem(
    Robot& robot, const double t, const double dt, 
    const Eigen::VectorXd& q_prev, const Eigen::VectorXd& v_prev, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  if (use_kinematics_) {
    robot.updateKinematics(s.q);
  }
  kkt_matrix.setZero();
  kkt_residual.setZero();
  stage_cost_  = cost_->quadratizeStageCost(robot, cost_data_, t, dt, s, 
                                            kkt_residual, kkt_matrix);
  stage_cost_ += cost_->quadratizeTerminalCost(robot, cost_data_, t, s, 
                                               kkt_residual, kkt_matrix);
  constraints_->condenseSlackAndDual(robot, constraints_data_, dt, s, 
                                     kkt_matrix, kkt_residual);
  unconstr::stateequation::linearizeBackwardEulerTerminal(dt, q_prev, v_prev, s,  
                                                          kkt_matrix, kkt_residual);
  unconstr_dynamics_.linearizeUnconstrDynamics(robot, dt, s, kkt_residual);
  unconstr_dynamics_.condenseUnconstrDynamics(kkt_matrix, kkt_residual);
}


inline void TerminalUnconstrParNMPC::expandPrimalAndDual(
    const double dt, const SplitSolution& s, const SplitKKTMatrix& kkt_matrix, 
    const SplitKKTResidual& kkt_residual, SplitDirection& d) {
  assert(dt > 0);
  unconstr_dynamics_.expandPrimal(d);
  unconstr_dynamics_.expandDual(dt, kkt_matrix, kkt_residual, d);
  constraints_->expandSlackAndDual(constraints_data_, s, d);
}


inline double TerminalUnconstrParNMPC::maxPrimalStepSize() {
  return constraints_->maxSlackStepSize(constraints_data_);
}


inline double TerminalUnconstrParNMPC::maxDualStepSize() {
  return constraints_->maxDualStepSize(constraints_data_);
}


inline void TerminalUnconstrParNMPC::updatePrimal(const Robot& robot, 
                                                  const double primal_step_size, 
                                                  const SplitDirection& d, 
                                                  SplitSolution& s) {
  assert(primal_step_size > 0);
  assert(primal_step_size <= 1);
  s.integrate(robot, primal_step_size, d);
  constraints_->updateSlack(constraints_data_, primal_step_size);
}


inline void TerminalUnconstrParNMPC::updateDual(const double dual_step_size) {
  assert(dual_step_size > 0);
  assert(dual_step_size <= 1);
  constraints_->updateDual(constraints_data_, dual_step_size);
}


inline double TerminalUnconstrParNMPC::squaredNormKKTResidual(
    const SplitKKTResidual& kkt_residual, const double dt) const {
  assert(dt > 0);
  double nrm = 0;
  nrm += kkt_residual.squaredNormKKTResidual();
  nrm += (dt*dt) * unconstr_dynamics_.squaredNormKKTResidual();
  nrm += (dt*dt) * constraints_data_.squaredNormKKTResidual();
  return nrm;
}


inline double TerminalUnconstrParNMPC::stageCost(Robot& robot, const double t,  
                                                 const double dt, 
                                                 const SplitSolution& s, 
                                                 const double primal_step_size) {
  assert(dt > 0);
  assert(primal_step_size >= 0);
  assert(primal_step_size <= 1);
  if (use_kinematics_) {
    robot.updateKinematics(s.q, s.v, s.a);
  }
  double cost = 0;
  cost += cost_->computeStageCost(robot, cost_data_, t, dt, s);
  cost += cost_->computeTerminalCost(robot, cost_data_, t, s);
  if (primal_step_size > 0) {
    cost += dt * constraints_->costSlackBarrier(constraints_data_, 
                                                primal_step_size);
  }
  else {
    cost += dt * constraints_->costSlackBarrier(constraints_data_);
  }
  return cost;
}


inline double TerminalUnconstrParNMPC::constraintViolation(
    Robot& robot, const double t, const double dt, 
    const Eigen::VectorXd& q_prev, const Eigen::VectorXd& v_prev, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  if (use_kinematics_) {
    robot.updateKinematics(s.q);
  }
  constraints_->computePrimalAndDualResidual(robot, constraints_data_, s);
  unconstr::stateequation::computeBackwardEulerResidual(dt, q_prev, v_prev, s, 
                                                        kkt_residual);
  unconstr_dynamics_.computeUnconstrDynamicsResidual(robot, s);
  double violation = 0;
  violation += kkt_residual.l1NormConstraintViolation();
  violation += dt * unconstr_dynamics_.l1NormConstraintViolation();
  violation += dt * constraints_data_.l1NormConstraintViolation();
  return violation;
}


inline void TerminalUnconstrParNMPC::computeTerminalCostHessian(
    Robot& robot, const double t, const SplitSolution& s, 
    SplitKKTMatrix& kkt_matrix, SplitKKTResidual& kkt_residual) {
  if (use_kinematics_) {
    robot.updateKinematics(s.q);
  }
  kkt_matrix.setZero();
  kkt_residual.setZero();
  double cost = cost_->quadratizeTerminalCost(robot, cost_data_, t, s, 
                                              kkt_residual, kkt_matrix);
}

} // namespace idocp

#endif // IDOCP_TERMINAL_UNCONSTR_PARNMPC_HXX_ 