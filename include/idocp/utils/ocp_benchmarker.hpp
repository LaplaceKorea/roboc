#ifndef IDOCP_UTILS_OCP_BENCHMARKER_HPP_
#define IDOCP_UTILS_OCP_BENCHMARKER_HPP_ 

#include <memory>
#include <string>

#include "Eigen/Core"

#include "idocp/ocp/ocp.hpp"
// #include "idocp/ocp/parnmpc.hpp"
#include "idocp/robot/robot.hpp"
#include "idocp/cost/cost_function.hpp"
#include "idocp/constraints/constraints.hpp"


namespace idocp {

template <typename OCPType>
class OCPBenchmarker {
public:
  OCPBenchmarker(const std::string& benchmark_name, const Robot& robot, 
                 const std::shared_ptr<CostFunction>& cost,
                 const std::shared_ptr<Constraints>& constraints, 
                 const double T, const int N, const int max_num_impulse,
                 const int num_proc);

  OCPBenchmarker();

  ~OCPBenchmarker();

  // Use default copy constructor.
  OCPBenchmarker(const OCPBenchmarker&) = default;

  // Use default copy operator.
  OCPBenchmarker& operator=(const OCPBenchmarker&) = default;

  // Use default move constructor.
  OCPBenchmarker(OCPBenchmarker&&) noexcept = default;

  // Use default move operator.
  OCPBenchmarker& operator=(OCPBenchmarker&&) noexcept = default;

  void setInitialGuessSolution(const double t, const Eigen::VectorXd& q, 
                               const Eigen::VectorXd& v);

  OCPType* getSolverHandle();

  void testCPUTime(const double t, const Eigen::VectorXd& q, 
                   const Eigen::VectorXd& v, const int num_iteration=1000,
                   const bool line_search=false);

  void testConvergence(const double t, const Eigen::VectorXd& q, 
                       const Eigen::VectorXd& v, const int num_iteration=10,
                       const bool line_search=false);

  void printSolution();

private:
  std::string benchmark_name_;
  OCPType ocp_;
  int dimq_, dimv_, max_dimf_, N_, num_proc_;
  double T_, dtau_;

};

} // namespace idocp 

#include "ocp_benchmarker.hxx"

#endif // IDOCP_UTILS_OCP_BENCHMARKER_HPP_