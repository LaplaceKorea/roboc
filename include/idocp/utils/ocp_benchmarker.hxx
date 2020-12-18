#ifndef IDOCP_UTILS_OCP_BENCHMARKER_HXX_
#define IDOCP_UTILS_OCP_BENCHMARKER_HXX_ 

#include <iostream>
#include <chrono>

#include "idocp/ocp/ocp_solver.hpp"
// #include "idocp/ocp/parnmpc_solver.hpp"

namespace idocp {

template <typename OCPSolverType>
inline OCPBenchmarker<OCPSolverType>::OCPBenchmarker(
    const std::string& benchmark_name, const Robot& robot, 
    const std::shared_ptr<CostFunction>& cost, 
    const std::shared_ptr<Constraints>& constraints, const double T, 
    const int N, const int max_num_impulse, const int num_proc)
  : benchmark_name_(benchmark_name),
    ocp_solver_(robot, cost, constraints, T, N, max_num_impulse, num_proc),
    dimq_(robot.dimq()),
    dimv_(robot.dimv()),
    max_dimf_(robot.max_dimf()),
    N_(N),
    num_proc_(num_proc),
    T_(T) {
}


template <typename OCPSolverType>
inline OCPBenchmarker<OCPSolverType>::OCPBenchmarker()
  : benchmark_name_(),
    ocp_solver_(),
    dimq_(0),
    dimv_(0),
    max_dimf_(0),
    N_(0),
    num_proc_(0),
    T_(0) {
}


template <typename OCPSolverType>
inline OCPBenchmarker<OCPSolverType>::~OCPBenchmarker() {
}


template <>
inline void OCPBenchmarker<OCPSolver>::setInitialGuessSolution(
    const double t, const Eigen::VectorXd& q, const Eigen::VectorXd& v) {
  ocp_solver_.setStateTrajectory(t, q, v);
}


// template <>
// inline void OCPBenchmarker<ParNMPC>::setInitialGuessSolution(
//     const double t, const Eigen::VectorXd& q, const Eigen::VectorXd& v) {
//   ocp_.setStateTrajectory(q, v);
//   ocp_.setAuxiliaryMatrixGuessByTerminalCost(t);
// }


template <typename OCPSolverType>
inline OCPSolverType* OCPBenchmarker<OCPSolverType>::getSolverHandle() {
  return &ocp_solver_;
}


template <typename OCPSolverType>
inline void OCPBenchmarker<OCPSolverType>::testCPUTime(
    const double t, const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
    const int num_iteration, const bool line_search) {
  std::chrono::system_clock::time_point start_clock, end_clock;
  start_clock = std::chrono::system_clock::now();
  for (int i=0; i<num_iteration; ++i) {
    ocp_solver_.updateSolution(t, q, v, line_search);
  }
  end_clock = std::chrono::system_clock::now();
  std::cout << "---------- OCP benchmark ----------" << std::endl;
  std::cout << "Test CPU time of " << benchmark_name_ << std::endl;
  std::cout << "robot.dimq() = " << dimq_ << std::endl;
  std::cout << "robot.dimv() = " << dimv_ << std::endl;
  std::cout << "robot.max_dimf() = " << max_dimf_ << std::endl;
  std::cout << "N (number of stages) = " << N_ << std::endl;
  if (line_search) {
    std::cout << "Line search: enable" << std::endl;
  }
  else {
    std::cout << "Line search: disable" << std::endl;
  }
  std::cout << "number of threads = " << num_proc_ << std::endl;
  std::cout << "total CPU time: " 
            << 1e-03 * std::chrono::duration_cast<std::chrono::microseconds>(
                  end_clock-start_clock).count() 
            << "[ms]" << std::endl;
  std::cout << "CPU time per update: " 
            << 1e-03 * std::chrono::duration_cast<std::chrono::microseconds>(
                  end_clock-start_clock).count() / num_iteration 
            << "[ms]" << std::endl;
  std::cout << "-----------------------------------" << std::endl;
  std::cout << std::endl;
}


template <typename OCPSolverType>
inline void OCPBenchmarker<OCPSolverType>::testConvergence(
    const double t, const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
    const int num_iteration, const bool line_search) {
  std::cout << "---------- OCP benchmark ----------" << std::endl;
  std::cout << "Test convergence of " << benchmark_name_ << std::endl;
  std::cout << "robot.dimq() = " << dimq_ << std::endl;
  std::cout << "robot.dimv() = " << dimv_ << std::endl;
  std::cout << "robot.max_dimf() = " << max_dimf_ << std::endl;
  std::cout << "N (number of stages) = " << N_ << std::endl;
  std::cout << "q = " << q.transpose() << std::endl;
  std::cout << "v = " << v.transpose() << std::endl;
  if (line_search) {
    std::cout << "Line search: enable" << std::endl;
  }
  else {
    std::cout << "Line search: disable" << std::endl;
  }
  ocp_solver_.computeKKTResidual(t, q, v);
  std::cout << "Initial KKT error = " << ocp_solver_.KKTError() << std::endl;
  for (int i=0; i<num_iteration; ++i) {
    ocp_solver_.updateSolution(t, q, v, line_search);
    ocp_solver_.computeKKTResidual(t, q, v);
    std::cout << "KKT error at iteration " << i << " = " 
              << ocp_solver_.KKTError() << std::endl;
  }
  std::cout << "-----------------------------------" << std::endl;
  std::cout << std::endl;
}


template <typename OCPSolverType>
inline void OCPBenchmarker<OCPSolverType>::printSolution() {
  ocp_solver_.printSolution();
}

} // namespace idocp 

#endif // IDOCP_UTILS_OCP_BENCHMARKER_HXX_