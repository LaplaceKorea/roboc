#ifndef IDOCP_JOINT_VELOCITY_UPPER_LIMITS_PDIPM_HPP_
#define IDOCP_JOINT_VELOCITY_UPPER_LIMITS_PDIPM_HPP_

#include "Eigen/Core"

#include "robot/robot.hpp"


namespace idocp {
namespace pdipm {

class JointVelocityUpperLimits {
public:
  JointVelocityUpperLimits(const Robot& robot, const double barrier);

  // Use default copy constructor.
  JointVelocityUpperLimits(const JointVelocityUpperLimits&) = default;

  // Use default copy operator.
  JointVelocityUpperLimits& operator=(const JointVelocityUpperLimits&) = default;

  bool isFeasible(const Robot& robot, const Eigen::VectorXd& v);

  void setSlackAndDual(const Robot& robot, const double dtau,
                       const Eigen::VectorXd& v);

  void condenseSlackAndDual(const Robot& robot, const double dtau, 
                            const Eigen::VectorXd& v, Eigen::MatrixXd& Cvv, 
                            Eigen::VectorXd& Cv);

  void computeSlackAndDualDirection(const Robot& robot, const double dtau,
                                    const Eigen::VectorXd& dq);

  double maxSlackStepSize(const double margin_rate);

  double maxDualStepSize(const double margin_rate);

  void updateSlack(const double step_size);

  void updateDual(const double step_size);

  double costSlackBarrier();

  double costSlackBarrier(const double step_size);

  void augmentDualResidual(const Robot& robot, const double dtau,
                           Eigen::VectorXd& Cv);

  double residualL1Nrom(const Robot& robot, const double dtau,
                        const Eigen::VectorXd& u);

  double residualSquaredNrom(const Robot& robot, const double dtau, 
                             const Eigen::VectorXd& u);

private:
  unsigned int dimq_, dimv_, dimc_;
  double barrier_;
  Eigen::VectorXd vmax_, slack_, dual_, residual_, duality_, dslack_, ddual_;
};

} // namespace pdipm
} // namespace idocp


#endif // IDOCP_JOINT_VELOCITY_UPPER_LIMITS_PDIPM_HPP_