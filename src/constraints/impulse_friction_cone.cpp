#include "idocp/constraints/impulse_friction_cone.hpp"

#include <iostream>
#include <cassert>


namespace idocp {

ImpulseFrictionCone::ImpulseFrictionCone(const Robot& robot, 
                                         const double barrier,
                                         const double fraction_to_boundary_rate)
  : ImpulseConstraintComponentBase(barrier, fraction_to_boundary_rate),
    dimc_(robot.maxPointContacts()) {
}


ImpulseFrictionCone::ImpulseFrictionCone()
  : ImpulseConstraintComponentBase(),
    dimc_(0) {
}


ImpulseFrictionCone::~ImpulseFrictionCone() {
}


KinematicsLevel ImpulseFrictionCone::kinematicsLevel() const {
  return KinematicsLevel::AccelerationLevel;
}


bool ImpulseFrictionCone::isFeasible(
    Robot& robot, ConstraintComponentData& data, 
    const ImpulseSplitSolution& s) const {
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isImpulseActive(i)) {
      const double mu = robot.frictionCoefficient(i);
      if (frictionConeResidual(mu, s.f[i]) > 0) {
        return false;
      }
    }
  }
  return true;
}


void ImpulseFrictionCone::setSlackAndDual(
    Robot& robot, ConstraintComponentData& data, 
    const ImpulseSplitSolution& s) const {
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    const double mu = robot.frictionCoefficient(i);
    data.slack.coeffRef(i) = - frictionConeResidual(mu, s.f[i]);
  }
  setSlackAndDualPositive(data);
}


void ImpulseFrictionCone::augmentDualResidual(
    Robot& robot, ConstraintComponentData& data, 
    const ImpulseSplitSolution& s, ImpulseSplitKKTResidual& kkt_residual) const {
  int dimf_stack = 0;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isImpulseActive(i)) {
      const double mu = robot.frictionCoefficient(i);
      const double gx = 2 * s.f[i].coeff(0);
      const double gy = 2 * s.f[i].coeff(1);
      const double gz = - 2 * mu * mu * s.f[i].coeff(2);
      kkt_residual.lf().coeffRef(dimf_stack  ) += gx * data.dual.coeff(i);
      kkt_residual.lf().coeffRef(dimf_stack+1) += gy * data.dual.coeff(i);
      kkt_residual.lf().coeffRef(dimf_stack+2) += gz * data.dual.coeff(i);
      dimf_stack += 3;
    }
  }
}


void ImpulseFrictionCone::condenseSlackAndDual(
    Robot& robot, ConstraintComponentData& data, 
    const ImpulseSplitSolution& s, ImpulseSplitKKTMatrix& kkt_matrix, 
    ImpulseSplitKKTResidual& kkt_residual) const {
  int dimf_stack = 0;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isImpulseActive(i)) {
      const double mu = robot.frictionCoefficient(i);
      const double gx = 2 * s.f[i].coeff(0);
      const double gy = 2 * s.f[i].coeff(1);
      const double gz = - 2 * mu * mu * s.f[i].coeff(2);
      const double dual_per_slack = data.dual.coeff(i) / data.slack.coeff(i);
      kkt_matrix.Qff().coeffRef(dimf_stack  , dimf_stack  ) += dual_per_slack * gx * gx;
      kkt_matrix.Qff().coeffRef(dimf_stack  , dimf_stack+1) += dual_per_slack * gx * gy;
      kkt_matrix.Qff().coeffRef(dimf_stack  , dimf_stack+2) += dual_per_slack * gx * gz;
      kkt_matrix.Qff().coeffRef(dimf_stack+1, dimf_stack  ) += dual_per_slack * gy * gx;
      kkt_matrix.Qff().coeffRef(dimf_stack+1, dimf_stack+1) += dual_per_slack * gy * gy;
      kkt_matrix.Qff().coeffRef(dimf_stack+1, dimf_stack+2) += dual_per_slack * gy * gz;
      kkt_matrix.Qff().coeffRef(dimf_stack+2, dimf_stack  ) += dual_per_slack * gz * gx;
      kkt_matrix.Qff().coeffRef(dimf_stack+2, dimf_stack+1) += dual_per_slack * gz * gy;
      kkt_matrix.Qff().coeffRef(dimf_stack+2, dimf_stack+2) += dual_per_slack * gz * gz;
      data.residual.coeffRef(i) = frictionConeResidual(mu, s.f[i]) 
                                  + data.slack.coeff(i);
      data.duality.coeffRef(i) = computeDuality(data.slack.coeff(i), 
                                                data.dual.coeff(i));
      const double coeff 
          = (data.dual.coeff(i)*data.residual.coeff(i)-data.duality.coeff(i)) 
            / data.slack.coeff(i);
      kkt_residual.lf().coeffRef(dimf_stack  ) += gx * coeff;
      kkt_residual.lf().coeffRef(dimf_stack+1) += gy * coeff; 
      kkt_residual.lf().coeffRef(dimf_stack+2) += gz * coeff;
      dimf_stack += 3;
    }
  }
}


void ImpulseFrictionCone::computeSlackAndDualDirection(
    Robot& robot, ConstraintComponentData& data, 
    const ImpulseSplitSolution& s, const ImpulseSplitDirection& d) const {
  int dimf_stack = 0;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isImpulseActive(i)) {
      const double mu = robot.frictionCoefficient(i);
      const double gx = 2 * s.f[i].coeff(0);
      const double gy = 2 * s.f[i].coeff(1);
      const double gz = - 2 * mu * mu * s.f[i].coeff(2);
      data.dslack.coeffRef(i) = - gx * d.df().coeff(dimf_stack  )
                                - gy * d.df().coeff(dimf_stack+1)
                                - gz * d.df().coeff(dimf_stack+2)
                                - data.residual.coeff(i);
      data.ddual.coeffRef(i) = computeDualDirection(data.slack.coeff(i), 
                                                    data.dual.coeff(i), 
                                                    data.dslack.coeff(i), 
                                                    data.duality.coeff(i));
      dimf_stack += 3;
    }
    else {
      // Set 1.0 to make the fraction-to-boundary rule easy.
      data.slack.coeffRef(i) = 1.0;
      data.dslack.coeffRef(i) = 1.0;
      data.dual.coeffRef(i) = 1.0;
      data.ddual.coeffRef(i) = 1.0;
    }
  }
}


void ImpulseFrictionCone::computePrimalAndDualResidual(
    Robot& robot, ConstraintComponentData& data, 
    const ImpulseSplitSolution& s) const {
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    if (s.isImpulseActive(i)) {
      const double mu = robot.frictionCoefficient(i);
      data.residual.coeffRef(i) = frictionConeResidual(mu, s.f[i]) 
                                  + data.slack.coeff(i);
      data.duality.coeffRef(i) = computeDuality(data.slack.coeff(i), 
                                                data.dual.coeff(i));
    }
  }
}


int ImpulseFrictionCone::dimc() const {
  return dimc_;
}

} // namespace idocp