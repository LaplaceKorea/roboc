#include "idocp/ocp/riccati_solver.hpp"

#include <stdexcept>


namespace idocp {

RiccatiSolver::RiccatiSolver(const Robot& robot, const double T, const int N, 
                             const int max_num_impulse, const int num_proc) 
  : riccati_recursion_(robot, N, num_proc),
    riccati_factorizer_(N, max_num_impulse, robot),
    riccati_factorization_(N, max_num_impulse, robot),
    constraint_factorizer_(robot, N, max_num_impulse, num_proc),
    constraint_factorization_(robot, N, max_num_impulse),
    direction_calculator_(N, max_num_impulse, num_proc),
    ocp_discretizer_(T, N, max_num_impulse) {
  try {
    if (T <= 0) {
      throw std::out_of_range("invalid value: T must be positive!");
    }
    if (N <= 0) {
      throw std::out_of_range("invalid value: N must be positive!");
    }
    if (max_num_impulse < 0) {
      throw std::out_of_range("invalid value: max_num_impulse must be non-negative!");
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


RiccatiSolver::RiccatiSolver() 
  : riccati_recursion_(),
    riccati_factorizer_(),
    riccati_factorization_(),
    constraint_factorizer_(),
    constraint_factorization_(),
    direction_calculator_(),
    ocp_discretizer_() {
}


RiccatiSolver::~RiccatiSolver() {
}


void RiccatiSolver::computeNewtonDirection(
    OCP& ocp, std::vector<Robot>& robots, 
    const ContactSequence& contact_sequence, const double t, 
    const Eigen::VectorXd& q, const Eigen::VectorXd& v, const Solution& s, 
    Direction& d, KKTMatrix& kkt_matrix, KKTResidual& kkt_residual) {
  ocp_discretizer_.discretizeOCP(contact_sequence, t);
  riccati_recursion_.backwardRiccatiRecursionTerminal(kkt_matrix, kkt_residual, 
                                                      riccati_factorization_);
  riccati_recursion_.backwardRiccatiRecursion(riccati_factorizer_, 
                                              ocp_discretizer_, kkt_matrix, 
                                              kkt_residual, 
                                              riccati_factorization_);
  constraint_factorization_.setConstraintStatus(contact_sequence);
  riccati_recursion_.forwardRiccatiRecursionParallel(riccati_factorizer_, 
                                                     ocp_discretizer_,
                                                     kkt_matrix, kkt_residual,
                                                     constraint_factorization_);
  if (ocp_discretizer_.existImpulse()) {
    riccati_recursion_.forwardStateConstraintFactorization(
        riccati_factorizer_, ocp_discretizer_, kkt_matrix, kkt_residual, 
        riccati_factorization_);
    riccati_recursion_.backwardStateConstraintFactorization(
        riccati_factorizer_, ocp_discretizer_, kkt_matrix, 
        constraint_factorization_);
  }
  direction_calculator_.computeInitialStateDirection(robots, q, v, s, d);
  if (ocp_discretizer_.existImpulse()) {
    constraint_factorizer_.computeLagrangeMultiplierDirection(
        ocp_discretizer_, riccati_factorization_, constraint_factorization_, d);
    constraint_factorizer_.aggregateLagrangeMultiplierDirection(
       constraint_factorization_,  ocp_discretizer_, d, riccati_factorization_);
  }
  riccati_recursion_.forwardRiccatiRecursion(
      riccati_factorizer_, ocp_discretizer_, kkt_matrix, kkt_residual, 
      riccati_factorization_, d);
  direction_calculator_.computeNewtonDirectionFromRiccatiFactorization(
      ocp, robots, ocp_discretizer_, riccati_factorizer_, 
      riccati_factorization_, s, d);
}


double RiccatiSolver::maxPrimalStepSize() const {
  return direction_calculator_.maxPrimalStepSize();
}


double RiccatiSolver::maxDualStepSize() const {
  return direction_calculator_.maxDualStepSize();
}


void RiccatiSolver::getStateFeedbackGain(const int time_stage, 
                                         Eigen::MatrixXd& Kq, 
                                         Eigen::MatrixXd& Kv) const {
  riccati_factorizer_[time_stage].getStateFeedbackGain(Kq, Kv);
}

} // namespace idocp