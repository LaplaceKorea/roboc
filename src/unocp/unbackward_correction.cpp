#include "idocp/unocp/unbackward_correction.hpp"

#include <omp.h>
#include <stdexcept>
#include <cassert>

namespace idocp {

UnBackwardCorrection::UnBackwardCorrection(const Robot& robot, const double T, 
                                           const int N, const int num_proc)
  : N_(N),
    num_proc_(num_proc),
    T_(T),
    dtau_(T/N),
    corrector_(N, SplitUnBackwardCorrection(robot)),
    s_new_(N, SplitSolution(robot)),
    aux_mat_(N, Eigen::MatrixXd::Zero(2*robot.dimv(), 2*robot.dimv())),
    primal_step_sizes_(Eigen::VectorXd::Zero(N)),
    dual_step_sizes_(Eigen::VectorXd::Zero(N)) {
  try {
    if (T <= 0) {
      throw std::out_of_range("invalid value: T must be positive!");
    }
    if (N <= 0) {
      throw std::out_of_range("invalid value: N must be positive!");
    }
    if (num_proc <= 0) {
      throw std::out_of_range("invalid value: num_proc must be positive!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
}


UnBackwardCorrection::UnBackwardCorrection()
  : N_(0),
    num_proc_(0),
    T_(0),
    dtau_(0),
    corrector_(),
    s_new_(),
    aux_mat_(),
    primal_step_sizes_(),
    dual_step_sizes_() {
}


UnBackwardCorrection::~UnBackwardCorrection() {
}


void UnBackwardCorrection::initAuxMat(std::vector<Robot>& robots, 
                                      UnParNMPC& parnmpc, const double t, 
                                      const UnSolution& s) {
  parnmpc.terminal.computeTerminalCostHessian(robots[0], t+T_, s[N_-1], 
                                              aux_mat_[N_-1]);
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<N_-1; ++i) {
    aux_mat_[i] = aux_mat_[N_-1];
  }
}


void UnBackwardCorrection::coarseUpdate(std::vector<Robot>& robots, 
                                        UnParNMPC& parnmpc, const double t, 
                                        const Eigen::VectorXd& q, 
                                        const Eigen::VectorXd& v,
                                        UnKKTMatrix& unkkt_matrix, 
                                        UnKKTResidual& unkkt_residual,
                                        const UnSolution& s, UnDirection& d) {
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<N_; ++i) {
    if (i == 0) {
      parnmpc[i].linearizeOCP(robots[omp_get_thread_num()], t+dtau_, dtau_, 
                              q, v, s[i], s[i+1], 
                              unkkt_matrix[i], unkkt_residual[i]);
      corrector_[i].coarseUpdate(aux_mat_[i+1], dtau_, unkkt_matrix[i], 
                                 unkkt_residual[i], s[i], d[i], s_new_[i]);
    }
    else if (i < N_-1) {
      parnmpc[i].linearizeOCP(robots[omp_get_thread_num()], t+(i+1)*dtau_, 
                              dtau_, s[i-1].q, s[i-1].v, s[i], s[i+1], 
                              unkkt_matrix[i], unkkt_residual[i]);
      corrector_[i].coarseUpdate(aux_mat_[i+1], dtau_, unkkt_matrix[i], 
                                 unkkt_residual[i], s[i], d[i], s_new_[i]);
    }
    else {
      parnmpc.terminal.linearizeOCP(robots[omp_get_thread_num()], t+T_, dtau_, 
                                    s[i-1].q, s[i-1].v, s[i], 
                                    unkkt_matrix[i], unkkt_residual[i]);
      corrector_[i].coarseUpdate(dtau_, unkkt_matrix[i], unkkt_residual[i], 
                                 s[i], d[i], s_new_[i]);
    }
  }
}


void UnBackwardCorrection::backwardCorrection(std::vector<Robot>& robots, 
                                              UnParNMPC& parnmpc, 
                                              const UnSolution& s, 
                                              UnDirection& d) {
  for (int i=N_-2; i>=0; --i) {
    corrector_[i].backwardCorrectionSerial(s[i+1], s_new_[i+1], s_new_[i]);
  }
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=N_-2; i>=0; --i) {
    corrector_[i].backwardCorrectionParallel(d[i], s_new_[i]);
  }
  for (int i=1; i<N_; ++i) {
    corrector_[i].forwardCorrectionSerial(s[i-1], s_new_[i-1], s_new_[i]);
  }
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<N_; ++i) {
    if (i > 0) {
      corrector_[i].forwardCorrectionParallel(d[i], s_new_[i]);
      aux_mat_[i] = corrector_[i].auxMat();
    }
    SplitUnBackwardCorrection::computeDirection(s[i], s_new_[i], d[i]);
    if (i <N_-1) {
      parnmpc[i].computeCondensedDirection(robots[omp_get_thread_num()],
                                           dtau_, s[i], d[i]);
      primal_step_sizes_.coeffRef(i) = parnmpc[i].maxPrimalStepSize();
      dual_step_sizes_.coeffRef(i)   = parnmpc[i].maxDualStepSize();
    }
    else {
      parnmpc.terminal.computeCondensedDirection(robots[omp_get_thread_num()],
                                                 dtau_, s[i], d[i]);
      primal_step_sizes_.coeffRef(i) = parnmpc.terminal.maxPrimalStepSize();
      dual_step_sizes_.coeffRef(i)   = parnmpc.terminal.maxDualStepSize();
    }
  }
}


double UnBackwardCorrection::primalStepSize() const {
  return primal_step_sizes_.minCoeff();
}


double UnBackwardCorrection::dualStepSize() const {
  return dual_step_sizes_.minCoeff();
}

} // namespace idocp