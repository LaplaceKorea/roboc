#ifndef IDOCP_TERMINAL_OCP_HXX_
#define IDOCP_TERMINAL_OCP_HXX_

#include "idocp/ocp/terminal_ocp.hpp"

#include <cassert>

namespace idocp {

inline TerminalOCP::TerminalOCP(const Robot& robot, 
                                const std::shared_ptr<CostFunction>& cost, 
                                const std::shared_ptr<Constraints>& constraints) 
  : cost_(cost),
    cost_data_(cost->createCostFunctionData(robot)),
    constraints_(constraints),
    constraints_data_(),
    state_equation_(robot),
    use_kinematics_(false),
    terminal_cost_(0) {
  if (cost_->useKinematics() || constraints_->useKinematics()) {
    use_kinematics_ = true;
  }
}


inline TerminalOCP::TerminalOCP() 
  : cost_(),
    cost_data_(),
    constraints_(),
    constraints_data_(),
    state_equation_(),
    use_kinematics_(false),
    terminal_cost_(0) {
}


inline TerminalOCP::~TerminalOCP() {
}


inline bool TerminalOCP::isFeasible(Robot& robot, const SplitSolution& s) {
  // TODO: add inequality constraints at the terminal OCP.
  return true;
}


inline void TerminalOCP::initConstraints(Robot& robot, const int time_stage, 
                                         const SplitSolution& s) {
  assert(time_stage >= 0);
  // TODO: add inequality constraints at the terminal OCP.
}


inline void TerminalOCP::linearizeOCP(Robot& robot, const double t, 
                                      const Eigen::VectorXd& q_prev,
                                      const SplitSolution& s,
                                      SplitKKTMatrix& kkt_matrix, 
                                      SplitKKTResidual& kkt_residual) {
  if (use_kinematics_) {
    robot.updateKinematics(s.q, s.v);
  }
  kkt_matrix.Qxx.setZero();
  kkt_residual.lx.setZero();
  terminal_cost_ = cost_->quadratizeTerminalCost(robot, cost_data_, t, s, 
                                                 kkt_residual, kkt_matrix);
  state_equation_.linearizeForwardEulerLieDerivative(robot, q_prev, s, 
                                                     kkt_matrix, kkt_residual);
}

 
inline double TerminalOCP::maxPrimalStepSize() {
  return 1;
  // TODO: add inequality constraints at the terminal OCP.
}


inline double TerminalOCP::maxDualStepSize() {
  return 1;
  // TODO: add inequality constraints at the terminal OCP.
}


inline void TerminalOCP::computeCondensedPrimalDirection(const SplitSolution& s, 
                                                         SplitDirection& d) {
}


inline void TerminalOCP::computeCondensedDualDirection(SplitDirection& d) {
  state_equation_.correctCostateDirection(d);
}


inline void TerminalOCP::updatePrimal(const Robot& robot, 
                                      const double step_size, 
                                      const SplitDirection& d, 
                                      SplitSolution& s) const {
  assert(step_size > 0);
  assert(step_size <= 1);
  s.lmd.noalias() += step_size * d.dlmd();
  s.gmm.noalias() += step_size * d.dgmm();
  robot.integrateConfiguration(d.dq(), step_size, s.q);
  s.v.noalias() += step_size * d.dv();
}


inline void TerminalOCP::updateDual(const double step_size) {
  assert(step_size > 0);
  assert(step_size <= 1);
  // TODO: add inequality constraints at the terminal OCP.
}


inline void TerminalOCP::computeKKTResidual(Robot& robot, const double t,  
                                            const Eigen::VectorXd& q_prev,
                                            const SplitSolution& s,
                                            SplitKKTMatrix& kkt_matrix,
                                            SplitKKTResidual& kkt_residual) {
  if (use_kinematics_) {
    robot.updateKinematics(s.q, s.v);
  }
  kkt_residual.lx.setZero();
  terminal_cost_ = cost_->linearizeTerminalCost(robot, cost_data_, t, s, 
                                                kkt_residual);
  state_equation_.linearizeForwardEuler(robot, q_prev, s, 
                                        kkt_matrix, kkt_residual);
}


inline double TerminalOCP::squaredNormKKTResidual(
    const SplitKKTResidual& kkt_residual) const {
  double error = 0;
  error += kkt_residual.lx.squaredNorm();
  return error;
}


inline double TerminalOCP::terminalCost(Robot& robot, const double t, 
                                        const SplitSolution& s) {
  if (use_kinematics_) {
    robot.updateKinematics(s.q, s.v);
  }
  return cost_->computeTerminalCost(robot, cost_data_, t, s);
}

} // namespace idocp

#endif // IDOCP_TERMINAL_OCP_HXX_ 