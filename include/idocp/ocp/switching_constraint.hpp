#ifndef IDOCP_SWITCHING_CONSTRAINT_HPP_ 
#define IDOCP_SWITCHING_CONSTRAINT_HPP_

#include "idocp/robot/robot.hpp"
#include "idocp/robot/impulse_status.hpp"
#include "idocp/ocp/split_kkt_matrix.hpp"
#include "idocp/ocp/split_kkt_residual.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_state_constraint_jacobian.hpp"


namespace idocp {

///
/// @class SwitchingConstraint
/// @brief Switching constraint expressed by the state-control equality 
/// constraint for the forward Euler. 
///
class SwitchingConstraint {
public:
  ///
  /// @brief Constructs a switching constraint.
  /// @param[in] robot Robot model. 
  ///
  SwitchingConstraint(const Robot& robot);

  ///
  /// @brief Default constructor. 
  ///
  SwitchingConstraint();

  ///
  /// @brief Destructor. 
  ///
  ~SwitchingConstraint();

  ///
  /// @brief Default copy constructor. 
  ///
  SwitchingConstraint(const SwitchingConstraint&) = default;

  ///
  /// @brief Default copy assign operator. 
  ///
  SwitchingConstraint& operator=(const SwitchingConstraint&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  SwitchingConstraint(SwitchingConstraint&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  SwitchingConstraint& operator=(SwitchingConstraint&&) noexcept = default;

  ///
  /// @brief Linearizes the switching constraint, i.e., the contact position
  /// constraint for the forward Euler.
  /// @param[in] robot Robot model. Kinematics must be updated.
  /// @param[in] impulse_status Impulse status. 
  /// @param[in] dt1 Time step of the time stage 2 stage before the impulse.
  /// @param[in] dt2 Time step of the time stage just before the impulse.
  /// @param[in] s Split solution of the time stage 2 stage before the impulse.
  /// @param[in, out] kkt_matrix Split KKT matrix of the time stage 2 stage 
  /// before the impulse.
  /// @param[in, out] kkt_residual Split KKT residual of the time stage 2 stage
  /// before the impulse.
  /// @param[in, out] jac Jacobian of the switching constraint. 
  ///
  void linearizeSwitchingConstraint(Robot& robot, 
                                    const ImpulseStatus& impulse_status, 
                                    const double dt1, const double dt2, 
                                    const SplitSolution& s, 
                                    SplitKKTMatrix& kkt_matrix, 
                                    SplitKKTResidual& kkt_residual, 
                                    SplitStateConstraintJacobian& jac);

  ///
  /// @brief Computes the residual in the switching constraint, i.e., the 
  /// contact position constraint for the forward Euler.
  /// @param[in] robot Robot model. Kinematics must be updated.
  /// @param[in] impulse_status Impulse status. 
  /// @param[in] dt1 Time step of the time stage 2 stage before the impulse.
  /// @param[in] dt2 Time step of the time stage just before the impulse.
  /// @param[in] s Split solution of the time stage 2 stage before the impulse.
  /// @param[in, out] kkt_residual Split KKT residual of the time stage 2 stage
  /// before the impulse.
  ///
  void computeSwitchingConstraintResidual(Robot& robot, 
                                          const ImpulseStatus& impulse_status, 
                                          const double dt1, const double dt2, 
                                          const SplitSolution& s, 
                                          SplitKKTResidual& kkt_residual);

  ///
  /// @brief Returns l1-norm of the residual in the switching constraint. 
  /// @param[in] kkt_residual Split KKT residual of the time stage just before
  /// the impulse.
  /// @return l1-norm of the residual in the switching constraint.
  ///
  static double l1NormSwitchingConstraintResidual(
      const SplitKKTResidual& kkt_residual);

  ///
  /// @brief Returns squared norm of the residual in the switching constraint. 
  /// @param[in] kkt_residual Split KKT residual of the time stage just before
  /// the impulse.
  /// @return Squared norm of the residual in the switching constraint.
  ///
  static double squaredNormSwitchingConstraintResidual(
      const SplitKKTResidual& kkt_residual);

private:
  Eigen::VectorXd q_, dq_;

};

} // namespace idocp

#include "idocp/ocp/switching_constraint.hxx"

#endif // IDOCP_SWITCHING_CONSTRAINT_HPP_ 