#include <string>
#include <memory>

#include "Eigen/Core"

#include "roboc/solver/unconstr_parnmpc_solver.hpp"
#include "roboc/robot/robot.hpp"
#include "roboc/cost/cost_function.hpp"
#include "roboc/cost/configuration_space_cost.hpp"
#include "roboc/constraints/constraints.hpp"
#include "roboc/utils/joint_constraints_factory.hpp"
#include "roboc/utils/ocp_benchmarker.hpp"


int main() {
  // Create a robot.
  const std::string path_to_urdf = "../iiwa_description/urdf/iiwa14.urdf";
  roboc::Robot robot(path_to_urdf);

  // Create a cost function.
  robot.setJointEffortLimit(Eigen::VectorXd::Constant(robot.dimu(), 200));
  auto cost = std::make_shared<roboc::CostFunction>();
  auto config_cost = std::make_shared<roboc::ConfigurationSpaceCost>(robot);
  config_cost->set_q_ref(Eigen::VectorXd::Constant(robot.dimv(), -5));
  config_cost->set_v_ref(Eigen::VectorXd::Constant(robot.dimv(), -9));
  config_cost->set_q_weight(Eigen::VectorXd::Constant(robot.dimv(), 10));
  config_cost->set_qf_weight(Eigen::VectorXd::Constant(robot.dimv(), 10));
  config_cost->set_v_weight(Eigen::VectorXd::Constant(robot.dimv(), 0.1));
  config_cost->set_vf_weight(Eigen::VectorXd::Constant(robot.dimv(), 0.1));
  config_cost->set_a_weight(Eigen::VectorXd::Constant(robot.dimv(), 0.01));
  config_cost->set_u_weight(Eigen::VectorXd::Constant(robot.dimv(), 0.0));
  cost->push_back(config_cost);

  // Create joint constraints.
  roboc::JointConstraintsFactory constraints_factory(robot);
  auto constraints = constraints_factory.create();

  // Create the ParNMPC solver for unconstrained rigid-body systems.
  const double T = 1;
  const int N = 20;
  // Please set nthreads by the number of the processors of your PC to enjoy ParNMPC!
  const int nthreads = 8;
  const double t = 0;
  const Eigen::VectorXd q = Eigen::VectorXd::Constant(robot.dimq(), 2);
  const Eigen::VectorXd v = Eigen::VectorXd::Zero(robot.dimv());
  roboc::UnconstrParNMPCSolver parnmpc_solver(robot, cost, constraints, T, N, nthreads);

  // Solves the OCP.
  parnmpc_solver.setSolution("q", q);
  parnmpc_solver.setSolution("v", v);
  parnmpc_solver.initBackwardCorrection(t);
  const int num_iteration = 100;
  const bool line_search = false;
  roboc::benchmark::convergence(parnmpc_solver, t, q, v, num_iteration, line_search);
  const int num_iteration_CPU = 10000;
  roboc::benchmark::CPUTime(parnmpc_solver, t, q, v, num_iteration_CPU, line_search);

  return 0;
}