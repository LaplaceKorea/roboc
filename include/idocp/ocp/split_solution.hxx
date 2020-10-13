#ifndef IDOCP_SPLIT_SOLUTION_HXX_
#define IDOCP_SPLIT_SOLUTION_HXX_

#include "idocp/ocp/split_solution.hpp"

namespace idocp {

inline SplitSolution::SplitSolution(const Robot& robot) 
  : lmd(Eigen::VectorXd::Zero(robot.dimv())),
    gmm(Eigen::VectorXd::Zero(robot.dimv())),
    mu_contact(robot.max_point_contacts(), Eigen::Vector3d::Zero()),
    a(Eigen::VectorXd::Zero(robot.dimv())),
    f(robot.max_point_contacts(), Eigen::Vector3d::Zero()),
    q(Eigen::VectorXd::Zero(robot.dimq())),
    v(Eigen::VectorXd::Zero(robot.dimv())),
    u(Eigen::VectorXd::Zero(robot.dimv())),
    beta(Eigen::VectorXd::Zero(robot.dimv())),
    mu_stack_(Eigen::VectorXd::Zero(robot.dim_passive()+robot.max_dimf())),
    f_stack_(Eigen::VectorXd::Zero(robot.max_dimf())),
    has_floating_base_(robot.has_floating_base()),
    dim_passive_(robot.dim_passive()),
    is_contact_active_(robot.max_point_contacts(), false),
    dimf_(0),
    dimc_(robot.dim_passive()) {
  robot.normalizeConfiguration(q);
}


inline SplitSolution::SplitSolution() 
  : lmd(),
    gmm(),
    mu_contact(),
    a(),
    f(),
    q(),
    v(),
    u(),
    beta(),
    mu_stack_(),
    f_stack_(),
    has_floating_base_(false),
    dim_passive_(0),
    is_contact_active_(),
    dimf_(0),
    dimc_(0) {
}


inline SplitSolution::~SplitSolution() {
}


inline void SplitSolution::setContactStatus(
    const ContactStatus& contact_status) {
  is_contact_active_ = contact_status.isContactActive();
  dimc_ = dim_passive_ + contact_status.dimf();
  dimf_ = contact_status.dimf();
}


inline Eigen::VectorBlock<Eigen::VectorXd> SplitSolution::mu_stack() {
  return mu_stack_.head(dimc_);
}


inline const Eigen::VectorBlock<const Eigen::VectorXd> 
SplitSolution::mu_stack() const {
  return mu_stack_.head(dimc_);
}


inline Eigen::VectorBlock<Eigen::VectorXd> SplitSolution::mu_floating_base() {
  return mu_stack_.head(dim_passive_);
}


inline const Eigen::VectorBlock<const Eigen::VectorXd> 
SplitSolution::mu_floating_base() const {
  return mu_stack_.head(dim_passive_);
}


inline Eigen::VectorBlock<Eigen::VectorXd> SplitSolution::mu_contacts() {
  return mu_stack_.segment(dim_passive_, dimf_);
}


inline const Eigen::VectorBlock<const Eigen::VectorXd> 
SplitSolution::mu_contacts() const {
  return mu_stack_.segment(dim_passive_, dimf_);
}


inline Eigen::VectorBlock<Eigen::VectorXd> SplitSolution::f_stack() {
  return f_stack_.head(dimf_);
}


inline const Eigen::VectorBlock<const Eigen::VectorXd> 
SplitSolution::f_stack() const {
  return f_stack_.head(dimf_);
}


inline void SplitSolution::set_mu_stack() {
  int contact_index = 0;
  int segment_start = dim_passive_;
  for (const auto is_contact_active : is_contact_active_) {
    if (is_contact_active) {
      mu_stack_.template segment<3>(segment_start) = mu_contact[contact_index];
      segment_start += 3;
    }
    ++contact_index;
  }
}


inline void SplitSolution::set_mu_contact() {
  int contact_index = 0;
  int segment_start = dim_passive_;
  for (const auto is_contact_active : is_contact_active_) {
    if (is_contact_active) {
      mu_contact[contact_index] = mu_stack_.template segment<3>(segment_start);
      segment_start += 3;
    }
    ++contact_index;
  }
}


inline void SplitSolution::set_f_stack() {
  int contact_index = 0;
  int segment_start = 0;
  for (const auto is_contact_active : is_contact_active_) {
    if (is_contact_active) {
      f_stack_.template segment<3>(segment_start) = f[contact_index];
      segment_start += 3;
    }
    ++contact_index;
  }
}


inline void SplitSolution::set_f() {
  int contact_index = 0;
  int segment_start = 0;
  for (const auto is_contact_active : is_contact_active_) {
    if (is_contact_active) {
      f[contact_index] = f_stack_.template segment<3>(segment_start);
      segment_start += 3;
    }
    ++contact_index;
  }
}


inline int SplitSolution::dimc() const {
  return dimc_;
}


inline int SplitSolution::dimf() const {
  return dimf_;
}


inline bool SplitSolution::isContactActive(const int contact_index) const {
  assert(!is_contact_active_.empty());
  assert(contact_index >= 0);
  assert(contact_index < is_contact_active_.size());
  return is_contact_active_[contact_index];
}


inline SplitSolution SplitSolution::Random(const Robot& robot) {
  SplitSolution s(robot);
  s.lmd = Eigen::VectorXd::Random(robot.dimv());
  s.gmm = Eigen::VectorXd::Random(robot.dimv());
  s.mu_stack() = Eigen::VectorXd::Random(robot.dim_passive());
  s.set_mu_contact();
  s.a = Eigen::VectorXd::Random(robot.dimv());
  s.q = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot.dimv());
  s.u = Eigen::VectorXd::Random(robot.dimv());
  s.beta = Eigen::VectorXd::Random(robot.dimv());
  return s;
}


inline SplitSolution SplitSolution::Random(
    const Robot& robot, const ContactStatus& contact_status) {
  SplitSolution s(robot);
  s.setContactStatus(contact_status);
  s.lmd = Eigen::VectorXd::Random(robot.dimv());
  s.gmm = Eigen::VectorXd::Random(robot.dimv());
  s.mu_stack() 
      = Eigen::VectorXd::Random(robot.dim_passive()+contact_status.dimf());
  s.set_mu_contact();
  s.a = Eigen::VectorXd::Random(robot.dimv());
  s.f_stack() = Eigen::VectorXd::Random(contact_status.dimf());
  s.set_f();
  s.q = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot.dimv());
  s.u = Eigen::VectorXd::Random(robot.dimv());
  s.beta = Eigen::VectorXd::Random(robot.dimv());
  return s;
}

} // namespace idocp 

#endif // IDOCP_SPLIT_SOLUTION_HXX_