#include "idocp/cost/contact_force_cost.hpp"

#include <iostream>
#include <stdexcept>


namespace idocp {

ContactForceCost::ContactForceCost(const Robot& robot)
  : CostFunctionComponentBase(),
    max_point_contacts_(robot.maxPointContacts()),
    max_dimf_(robot.max_dimf()),
    f_ref_(robot.maxPointContacts(), Eigen::Vector3d::Zero()),
    f_weight_(robot.maxPointContacts(), Eigen::Vector3d::Zero()) {
}


ContactForceCost::ContactForceCost()
  : CostFunctionComponentBase(),
    max_point_contacts_(0),
    max_dimf_(0),
    f_ref_(),
    f_weight_() {
}


ContactForceCost::~ContactForceCost() {
}


bool ContactForceCost::useKinematics() const {
  return false;
}


void ContactForceCost::set_f_ref(const std::vector<Eigen::Vector3d>& f_ref) {
  try {
    if (f_ref.size() != max_point_contacts_) {
      throw std::invalid_argument(
          "invalid size: f_ref.size() must be " 
          + std::to_string(max_point_contacts_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  f_ref_ = f_ref;
}


void ContactForceCost::set_f_weight(
    const std::vector<Eigen::Vector3d>& f_weight) {
  try {
    if (f_weight.size() != max_point_contacts_) {
      throw std::invalid_argument(
          "invalid size: f_weight.size() must be " 
          + std::to_string(max_point_contacts_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  f_weight_ = f_weight;
}


double ContactForceCost::l(Robot& robot, CostFunctionData& data, const double t, 
                           const double dtau, const SplitSolution& s) const {
  double l = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isContactActive(i)) {
      l += (f_weight_[i].array() * (s.f[i].array()-f_ref_[i].array()) 
                                 * (s.f[i].array()-f_ref_[i].array())).sum();
    }
  }
  return 0.5 * dtau * l;
}


double ContactForceCost::phi(Robot& robot, CostFunctionData& data, 
                             const double t, const SplitSolution& s) const {
  return 0;
}


void ContactForceCost::lf(Robot& robot, CostFunctionData& data, const double t, 
                          const double dtau, const SplitSolution& s, 
                          SplitKKTResidual& kkt_residual) const {
  int dimf_stack = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isContactActive(i)) {
      kkt_residual.lf().template segment<3>(dimf_stack).array()
          += dtau * f_weight_[i].array() * (s.f[i].array()-f_ref_[i].array());
      dimf_stack += 3;
    }
  }
}


void ContactForceCost::lff(Robot& robot, CostFunctionData& data, const double t, 
                           const double dtau, const SplitSolution& s, 
                           SplitKKTMatrix& kkt_matrix) const {
  int dimf_stack = 0;
  for (int i=0; i<max_point_contacts_; ++i) {
    if (s.isContactActive(i)) {
      kkt_matrix.Qff().diagonal().template segment<3>(dimf_stack).noalias() 
          += dtau * f_weight_[i];
      dimf_stack += 3;
    }
  }
}

} // namespace idocp