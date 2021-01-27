#include "idocp/constraints/linearized_friction_cone.hpp"

#include <iostream>
#include <cassert>
#include <stdexcept>


namespace idocp {

LinearizedFrictionCone::LinearizedFrictionCone(
    const Robot& robot, const double mu, const double barrier,
    const double fraction_to_boundary_rate)
  : ConstraintComponentBase(barrier, fraction_to_boundary_rate),
    dimc_(3*robot.maxPointContacts()),
    mu_(mu),
    Jac(Eigen::Matrix3d::Zero()) {
  try {
    if (mu <= 0) {
      throw std::out_of_range("invalid value: mu must be positive!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  Jac << 0, 0, -1,
         1, 0, -(mu/std::sqrt(2)),
         0, 1, -(mu/std::sqrt(2));
}


LinearizedFrictionCone::LinearizedFrictionCone()
  : ConstraintComponentBase(),
    dimc_(0),
    mu_(0),
    Jac() {
}


LinearizedFrictionCone::~LinearizedFrictionCone() {
}


void LinearizedFrictionCone::setFrictionCoefficient(const double mu) {
  try {
    if (mu <= 0) {
      throw std::out_of_range("invalid value: mu must be positive!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  mu_ = mu;
  Jac << 0, 0, -1,
         1, 0, -(mu/std::sqrt(2)),
         0, 1, -(mu/std::sqrt(2));
}


bool LinearizedFrictionCone::useKinematics() const {
  return true;
}


KinematicsLevel LinearizedFrictionCone::kinematicsLevel() const {
  return KinematicsLevel::AccelerationLevel;
}


void LinearizedFrictionCone::allocateExtraData(
    ConstraintComponentData& data) const {
  data.r.clear();
  for (int i=0; i<dimc_; ++i) {
    data.r.push_back(Eigen::VectorXd::Zero(3));
  }
  data.J.clear();
  for (int i=0; i<dimc_; ++i) {
    data.J.push_back(Eigen::MatrixXd::Zero(3, 3));
  }
}


bool LinearizedFrictionCone::isFeasible(Robot& robot, 
                                        ConstraintComponentData& data, 
                                        const SplitSolution& s) const {
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isContactActive(i)) {
      if (frictionConeResidual(mu_, s.f[i]).minCoeff() > 0) {
        return false;
      }
      if (normalForceResidual(s.f[i]) > 0) {
        return false;
      }
    }
  }
  return true;
}


void LinearizedFrictionCone::setSlackAndDual(Robot& robot, 
                                             ConstraintComponentData& data, 
                                             const SplitSolution& s) const {
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    const int idx = 3*i;
    data.slack.coeffRef(idx) = - normalForceResidual(s.f[i]);
    data.slack.template segment<2>(idx+1) 
        = - frictionConeResidual(mu_, s.f[i]);
  }
  setSlackAndDualPositive(data);
}


void LinearizedFrictionCone::augmentDualResidual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  assert(dtau >= 0);
  int dimf_stack = 0;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isContactActive(i)) {
      kkt_residual.lf().template segment<3>(dimf_stack).noalias()
          += dtau * Jac.transpose() * data.dual.template segment<3>(3*i);
      dimf_stack += 3;
    }
  }
}


void LinearizedFrictionCone::condenseSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) const {
  assert(dtau >= 0);
  computePrimalAndDualResidual(robot, data, s);
  int dimf_stack = 0;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isContactActive(i)) {
      const int idx = 3*i;
      data.r[i].array() 
          = (data.dual.template segment<3>(idx).array()
              *data.residual.template segment<3>(idx).array()
              -data.duality.template segment<3>(idx).array())
              / data.slack.template segment<3>(idx).array();
      kkt_residual.lf().template segment<3>(dimf_stack).noalias()
          += dtau * Jac.transpose() * data.r[i];
      data.r[i].array() = data.dual.template segment<3>(idx).array() 
                          / data.slack.template segment<3>(idx).array();
      data.J[i].noalias() = data.r[i].asDiagonal() * Jac;
      kkt_matrix.Qff().template block<3, 3>(dimf_stack, dimf_stack).noalias()
          += dtau * Jac.transpose() * data.J[i];
      dimf_stack += 3;
    }
  }
}


void LinearizedFrictionCone::computeSlackAndDualDirection(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s, 
    const SplitDirection& d) const {
  // Because data.slack(i) and data.dual(i) are always positive,  
  // - fraction_rate * (slack.coeff(i)/dslack.coeff(i)) and 
  // - fraction_rate * (dual.coeff(i)/ddual.coeff(i))  
  // at the inactive constraint index i are always negative, 
  // and therefore do not affect to step size.
  data.dslack.fill(1.0);
  data.ddual.fill(1.0);
  int dimf_stack = 0;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isContactActive(i)) {
      const int idx = 3*i;
      data.dslack.template segment<3>(idx).noalias()
          = - Jac * d.df().segment<3>(dimf_stack) 
              - data.residual.template segment<3>(idx);
      for (int j=0; j<3; ++j) {
        data.ddual.coeffRef(idx+j) = computeDualDirection(data.slack.coeff(idx+j), 
                                                          data.dual.coeff(idx+j), 
                                                          data.dslack.coeff(idx+j), 
                                                          data.duality.coeff(idx+j));
      }
      dimf_stack += 3;
    }
  }
}


void LinearizedFrictionCone::computePrimalAndDualResidual(
    Robot& robot, ConstraintComponentData& data, const SplitSolution& s) const {
  data.residual.setZero();
  data.duality.setZero();
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isContactActive(i)) {
      const int idx = 3*i;
      data.residual.coeffRef(idx) = normalForceResidual(s.f[i]) 
                                      + data.slack.coeff(idx);
      data.residual.template segment<2>(idx+1) 
          = frictionConeResidual(mu_, s.f[i]) 
              + data.slack.template segment<2>(idx+1);
      for (int j=0; j<3; ++j) {
        data.duality.coeffRef(idx+j) = computeDuality(data.slack.coeff(idx+j), 
                                                      data.dual.coeff(idx+j));
      }
    }
  }
}


int LinearizedFrictionCone::dimc() const {
  return dimc_;
}

} // namespace idocp