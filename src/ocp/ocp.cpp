#include "idocp/ocp/ocp.hpp"

#include <omp.h>
#include <stdexcept>
#include <cassert>


namespace idocp {

OCP::OCP(const Robot& robot, const std::shared_ptr<CostFunction>& cost,
         const std::shared_ptr<Constraints>& constraints, const double T, 
         const int N, const int max_num_impulse, const int num_proc)
  : robots_(num_proc, robot),
    contact_sequence_(robot, T, N),
    ocp_linearizer_(T, N, max_num_impulse, num_proc),
    ocp_riccati_solver_(robot, T, N, max_num_impulse, num_proc),
    ocp_solution_integrator_(T, N, max_num_impulse, num_proc),
    split_ocps_(N, max_num_impulse, robot, cost, constraints),
    kkt_matrix_(N, max_num_impulse, robot),
    kkt_residual_(N, max_num_impulse, robot),
    s_(N, max_num_impulse, robot),
    d_(N, max_num_impulse, robot),
    filter_(),
    N_(N),
    num_proc_(num_proc),
    T_(T),
    dtau_(T/N) {
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
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<=N; ++i) {
    robot.normalizeConfiguration(s_[i].q);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    robot.normalizeConfiguration(s_.impulse[i].q);
    robot.normalizeConfiguration(s_.aux[i].q);
    robot.normalizeConfiguration(s_.lift[i].q);
  }
  initConstraints();
}


OCP::OCP() {
}


OCP::~OCP() {
}


void OCP::updateSolution(const double t, const Eigen::VectorXd& q, 
                         const Eigen::VectorXd& v, const bool use_line_search) {
  assert(q.size() == robots_[0].dimq());
  assert(v.size() == robots_[0].dimv());
  ocp_linearizer_.linearizeOCP(split_ocps_, robots_, contact_sequence_, 
                               t, q, v, s_, kkt_matrix_, kkt_residual_);
  ocp_riccati_solver_.computeDirection(split_ocps_, robots_, contact_sequence_, 
                                       q, v, s_, d_, kkt_matrix_, kkt_residual_);
  const double primal_step_size = ocp_riccati_solver_.maxPrimalStepSize(contact_sequence_);
  const double dual_step_size = ocp_riccati_solver_.maxDualStepSize(contact_sequence_);
  ocp_solution_integrator_.integrate(split_ocps_, robots_, contact_sequence_, 
                                     kkt_matrix_, kkt_residual_, 
                                     primal_step_size, dual_step_size, d_, s_);
} 


const SplitSolution& OCP::getSolution(const int stage) const {
  assert(stage >= 0);
  assert(stage <= N_);
  return s_[stage];
}


void OCP::getStateFeedbackGain(const int stage, Eigen::MatrixXd& Kq, 
                               Eigen::MatrixXd& Kv) const {
  assert(stage >= 0);
  assert(stage < N_);
  assert(Kq.rows() == robots_[0].dimv());
  assert(Kq.cols() == robots_[0].dimv());
  assert(Kv.rows() == robots_[0].dimv());
  assert(Kv.cols() == robots_[0].dimv());
  // split_ocps_[stage].getStateFeedbackGain(Kq, Kv);
}


bool OCP::setStateTrajectory(const Eigen::VectorXd& q, 
                             const Eigen::VectorXd& v) {
  assert(q.size() == robots_[0].dimq());
  assert(v.size() == robots_[0].dimv());
  Eigen::VectorXd q_normalized = q;
  robots_[0].normalizeConfiguration(q_normalized);
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<=N_; ++i) {
    s_[i].v = v;
    s_[i].q = q_normalized;
  }
  for (int i=0; i<contact_sequence_.totalNumImpulseStages(); ++i) {
    s_.impulse[i].v = v;
    s_.impulse[i].q = q_normalized;
    s_.aux[i].v = v;
    s_.aux[i].q = q_normalized;
  }
  for (int i=0; i<contact_sequence_.totalNumLiftStages(); ++i) {
    s_.lift[i].v = v;
    s_.lift[i].q = q_normalized;
  }
  initConstraints();
  const bool feasible = isCurrentSolutionFeasible();
  return feasible;
}


bool OCP::setStateTrajectory(const Eigen::VectorXd& q0, 
                             const Eigen::VectorXd& v0, 
                             const Eigen::VectorXd& qN, 
                             const Eigen::VectorXd& vN) {
  assert(q0.size() == robots_[0].dimq());
  assert(v0.size() == robots_[0].dimv());
  assert(qN.size() == robots_[0].dimq());
  assert(vN.size() == robots_[0].dimv());
  Eigen::VectorXd q0_normalized = q0;
  robots_[0].normalizeConfiguration(q0_normalized);
  Eigen::VectorXd qN_normalized = qN;
  robots_[0].normalizeConfiguration(qN_normalized);
  const Eigen::VectorXd a = (vN-v0) / N_;
  Eigen::VectorXd dqN = Eigen::VectorXd::Zero(robots_[0].dimv());
  robots_[0].subtractConfiguration(qN, q0, dqN);
  const Eigen::VectorXd v = dqN / N_;
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<=N_; ++i) {
    s_[i].a = a;
    s_[i].v = v0 + i * a;
    robots_[0].integrateConfiguration(q0, v, (double)i, s_[i].q);
  }
  initConstraints();
  const bool feasible = isCurrentSolutionFeasible();
  return feasible;
}


void OCP::setContactStatusUniformly(const ContactStatus& contact_status) {
  contact_sequence_.setContactStatusUniformly(contact_status);
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<N_; ++i) {
    s_[i].setContactStatus(contact_sequence_.contactStatus(i));
    d_[i].setContactStatus(contact_sequence_.contactStatus(i));
  }
}


void OCP::setDiscreteEvent(const DiscreteEvent& discrete_event) {
  contact_sequence_.setDiscreteEvent(discrete_event);
  for (int i=0; i<=N_; ++i) {
    s_[i].setContactStatus(contact_sequence_.contactStatus(i));
    d_[i].setContactStatus(contact_sequence_.contactStatus(i));
  }
  for (int i=0; i<contact_sequence_.totalNumImpulseStages(); ++i) {
    s_.impulse[i].setImpulseStatus(contact_sequence_.impulseStatus(i));
    d_.impulse[i].setImpulseStatus(contact_sequence_.impulseStatus(i));
    const int stage = contact_sequence_.timeStageAfterImpulse(i);
    s_.aux[i].setContactStatus(contact_sequence_.contactStatus(stage));
    d_.aux[i].setContactStatus(contact_sequence_.contactStatus(stage));
  }
  for (int i=0; i<contact_sequence_.totalNumLiftStages(); ++i) {
    const int stage = contact_sequence_.timeStageAfterLift(i);
    s_.lift[i].setContactStatus(contact_sequence_.contactStatus(stage));
    d_.lift[i].setContactStatus(contact_sequence_.contactStatus(stage));
  }
  ocp_riccati_solver_.setConstraintDimensions(contact_sequence_);
}


void OCP::setContactPoint(
    const std::vector<Eigen::Vector3d>& contact_points) {
  assert(contact_points.size() == robots_[0].max_point_contacts());
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<robots_.size(); ++i) {
    robots_[i].setContactPoints(contact_points);
  }
}


void OCP::setContactPointByKinematics(const Eigen::VectorXd& q) {
  assert(q.size() == robots_[0].dimq());
  const int dimv = robots_[0].dimv();
  #pragma omp parallel for num_threads(num_proc_)
  for (int i=0; i<robots_.size(); ++i) {
    robots_[i].updateKinematics(q, Eigen::VectorXd::Zero(dimv), 
                                Eigen::VectorXd::Zero(dimv));
    robots_[i].setContactPointsByCurrentKinematics();
  }
}


void OCP::clearLineSearchFilter() {
  filter_.clear();
}


double OCP::KKTError() {
  return ocp_linearizer_.KKTError(split_ocps_, contact_sequence_, kkt_residual_);
}


void OCP::computeKKTResidual(const double t, const Eigen::VectorXd& q, 
                             const Eigen::VectorXd& v) {
  ocp_linearizer_.computeKKTResidual(split_ocps_, robots_, contact_sequence_, 
                                     t, q, v, s_, kkt_matrix_, kkt_residual_);
}


void OCP::printSolution() const {
  for (int i=0; i<N_; ++i) {
    std::cout << "q[" << i << "] = " << s_[i].q.transpose() << std::endl;
    std::cout << "v[" << i << "] = " << s_[i].v.transpose() << std::endl;
    std::cout << "a[" << i << "] = " << s_[i].a.transpose() << std::endl;
    std::cout << "f[" << i << "] = ";
    for (int j=0; j<s_[i].f.size(); ++j) {
      std::cout << s_[i].f[j].transpose() << "; ";
    }
    std::cout << std::endl;
    std::cout << "u[" << i << "] = " << s_[i].u.transpose() << std::endl;
    std::cout << "mu[" << i << "] = " << s_[i].mu_stack().transpose() << std::endl;
  }
  std::cout << "q[" << N_ << "] = " << s_[N_].q.transpose() << std::endl;
  std::cout << "v[" << N_ << "] = " << s_[N_].v.transpose() << std::endl;
}


bool OCP::isCurrentSolutionFeasible() {
  for (int i=0; i<N_; ++i) {
    const bool feasible = split_ocps_[i].isFeasible(robots_[0], s_[i]);
    if (!feasible) {
      std::cout << "INFEASIBLE at time step " << i << std::endl;
      return false;
    }
  }
  return true;
}


void OCP::initConstraints() {
  ocp_linearizer_.initConstraints(split_ocps_, robots_, contact_sequence_, s_);
}

} // namespace idocp