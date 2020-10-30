#ifndef IDOCP_SPLIT_SOLUTION_HXX_
#define IDOCP_SPLIT_SOLUTION_HXX_

#include "idocp/ocp/split_solution.hpp"

namespace idocp {

inline SplitSolution::SplitSolution(const Robot& robot) 
  : lmd(Eigen::VectorXd::Zero(robot.dimv())),
    gmm(Eigen::VectorXd::Zero(robot.dimv())),
    mu(robot.max_point_contacts(), Eigen::Vector3d::Zero()),
    a(Eigen::VectorXd::Zero(robot.dimv())),
    f(robot.max_point_contacts(), Eigen::Vector3d::Zero()),
    q(Eigen::VectorXd::Zero(robot.dimq())),
    v(Eigen::VectorXd::Zero(robot.dimv())),
    u(Eigen::VectorXd::Zero(robot.dimu())),
    beta(Eigen::VectorXd::Zero(robot.dimv())),
    u_passive(Vector6d::Zero()),
    nu_passive(Vector6d::Zero()),
    mu_stack_(Eigen::VectorXd::Zero(robot.max_dimf())),
    f_stack_(Eigen::VectorXd::Zero(robot.max_dimf())),
    has_floating_base_(robot.has_floating_base()),
    has_active_contacts_(false),
    dim_passive_(robot.dim_passive()),
    is_contact_active_(robot.max_point_contacts(), false),
    dimf_(0) {
  robot.normalizeConfiguration(q);
}


inline SplitSolution::SplitSolution() 
  : lmd(),
    gmm(),
    mu(),
    a(),
    f(),
    q(),
    v(),
    u(),
    beta(),
    u_passive(Vector6d::Zero()),
    nu_passive(Vector6d::Zero()),
    mu_stack_(),
    f_stack_(),
    has_floating_base_(false),
    has_active_contacts_(false),
    dim_passive_(0),
    is_contact_active_(),
    dimf_(0) {
}


inline SplitSolution::~SplitSolution() {
}


inline void SplitSolution::setContactStatus(
    const ContactStatus& contact_status) {
  assert(contact_status.max_point_contacts()==is_contact_active_.size());
  is_contact_active_ = contact_status.isContactActive();
  dimf_ = contact_status.dimf();
  has_active_contacts_ = contact_status.hasActiveContacts();
}


inline Eigen::VectorBlock<Eigen::VectorXd> SplitSolution::mu_stack() {
  return mu_stack_.head(dimf_);
}


inline const Eigen::VectorBlock<const Eigen::VectorXd> 
SplitSolution::mu_stack() const {
  return mu_stack_.head(dimf_);
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
  int segment_start = 0;
  for (const auto is_contact_active : is_contact_active_) {
    if (is_contact_active) {
      mu_stack_.template segment<3>(segment_start) = mu[contact_index];
      segment_start += 3;
    }
    ++contact_index;
  }
}


inline void SplitSolution::set_mu_vector() {
  int contact_index = 0;
  int segment_start = 0;
  for (const auto is_contact_active : is_contact_active_) {
    if (is_contact_active) {
      mu[contact_index] = mu_stack_.template segment<3>(segment_start);
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


inline void SplitSolution::set_f_vector() {
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


inline int SplitSolution::dimf() const {
  return dimf_;
}


inline bool SplitSolution::isContactActive(const int contact_index) const {
  assert(!is_contact_active_.empty());
  assert(contact_index >= 0);
  assert(contact_index < is_contact_active_.size());
  return is_contact_active_[contact_index];
}


inline bool SplitSolution::hasActiveContacts() const {
  return has_active_contacts_;
}


inline bool SplitSolution::isApprox(const SplitSolution& other) const {
  if (!lmd.isApprox(other.lmd)) {
    return false;
  }
  if (!gmm.isApprox(other.gmm)) {
    return false;
  }
  if (!q.isApprox(other.q)) {
    return false;
  }
  if (!v.isApprox(other.v)) {
    return false;
  }
  if (!a.isApprox(other.a)) {
    return false;
  }
  if (!u.isApprox(other.u)) {
    return false;
  }
  if (!beta.isApprox(other.beta)) {
    return false;
  }
  if (has_active_contacts_) {
    if (!f_stack().isApprox(other.f_stack())) {
      return false;
    }
    if (!mu_stack().isApprox(other.mu_stack())) {
      return false;
    }
    for (int i=0; i<is_contact_active_.size(); ++i) {
      if (is_contact_active_[i]) {
        if (!other.isContactActive(i)) {
          return false;
        }
        if (!f[i].isApprox(other.f[i])) {
          return false;
        }
        if (!mu[i].isApprox(other.mu[i])) {
          return false;
        }
      }
      else {
        if (other.isContactActive(i)) {
          return false;
        }
      }
    }
  }
  if (has_floating_base_) {
    if (!u_passive.isApprox(other.u_passive)) {
      return false;
    }
    if (!nu_passive.isApprox(other.nu_passive)) {
      return false;
    }
  }
  return true;
}


inline void SplitSolution::setRandom(const Robot& robot) {
  lmd.setRandom();
  gmm.setRandom(); 
  q.setRandom(); 
  robot.normalizeConfiguration(q);
  v.setRandom();
  a.setRandom(); 
  u.setRandom(); 
  beta.setRandom(); 
  if (robot.has_floating_base()) {
    u_passive.setRandom();
    nu_passive.setRandom();
  }
}


inline void SplitSolution::setRandom(const Robot& robot, 
                                     const ContactStatus& contact_status) {
  setContactStatus(contact_status);
  setRandom(robot);
  if (contact_status.hasActiveContacts()) {
    f_stack().setRandom();
    mu_stack().setRandom();
    set_f_vector();
    set_mu_vector();
  }
}


inline SplitSolution SplitSolution::Random(const Robot& robot) {
  SplitSolution s(robot);
  s.setRandom(robot);
  return s;
}


inline SplitSolution SplitSolution::Random(
    const Robot& robot, const ContactStatus& contact_status) {
  SplitSolution s(robot);
  s.setRandom(robot, contact_status);
  return s;
}

} // namespace idocp 

#endif // IDOCP_SPLIT_SOLUTION_HXX_