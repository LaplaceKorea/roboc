#ifndef ROBOC_UTILS_OCP_BENCHMARKER_HPP_
#define ROBOC_UTILS_OCP_BENCHMARKER_HPP_ 

#include <memory>
#include <string>

#include "Eigen/Core"

#include "roboc/utils/logger.hpp"


namespace roboc {
namespace benchmark {

template <typename OCPSolverType>
void CPUTime(OCPSolverType& ocp_solver, const double t, 
             const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
             const int num_iteration=1000, const bool line_search=false);

template <typename OCPSolverType>
void Convergence(OCPSolverType& ocp_solver, const double t, 
                 const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
                 const int num_iteration=10, const bool line_search=false);

template <typename OCPSolverType>
void Convergence(OCPSolverType& ocp_solver, Logger& logger, 
                 const double t, const Eigen::VectorXd& q, 
                 const Eigen::VectorXd& v, const int num_iteration=10, 
                 const bool line_search=false);

} // namespace benchmark
} // namespace roboc 

#include "roboc/utils/ocp_benchmarker.hxx"

#endif // ROBOC_UTILS_OCP_BENCHMARKER_HPP_