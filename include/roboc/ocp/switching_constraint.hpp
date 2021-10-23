#ifndef ROBOC_SWITCHING_CONSTRAINT_HPP_ 
#define ROBOC_SWITCHING_CONSTRAINT_HPP_

#include "roboc/robot/robot.hpp"
#include "roboc/robot/impulse_status.hpp"
#include "roboc/ocp/split_solution.hpp"
#include "roboc/ocp/split_kkt_matrix.hpp"
#include "roboc/ocp/split_kkt_residual.hpp"
#include "roboc/ocp/split_switching_constraint_residual.hpp"
#include "roboc/ocp/split_switching_constraint_jacobian.hpp"


namespace roboc {

namespace switchingconstraint {

///
/// @brief Linearizes the switching constraint, i.e., the contact position 
/// constraint.
/// @param[in] robot Robot model. Kinematics must be updated.
/// @param[in] impulse_status Impulse status. 
/// @param[in] dt1 Time step of the time stage 2 stage before the impulse.
/// @param[in] dt2 Time step of the time stage just before the impulse.
/// @param[in] s Split solution of the time stage 2 stage before the impulse.
/// @param[in, out] kkt_matrix Split KKT matrix of the time stage 2 stage
/// before the impulse.
/// @param[in, out] kkt_residual Split KKT residual of the time stage 2 stage
/// before the impulse.
/// @param[in, out] sc_jacobian Jacobian of the switching constraint. 
/// @param[in, out] sc_residual Residual of the switching constraint. 
///
void linearizeSwitchingConstraint(
    Robot& robot, const ImpulseStatus& impulse_status, const double dt1, 
    const double dt2, const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual, 
    SplitSwitchingConstraintJacobian& sc_jacobian,
    SplitSwitchingConstraintResidual& sc_residual);

///
/// @brief Computes the residual in the switching constraint, i.e., the 
/// contact position constraint.
/// @param[in] robot Robot model. Kinematics must be updated.
/// @param[in] impulse_status Impulse status. 
/// @param[in] dt1 Time step of the time stage 2 stage before the impulse.
/// @param[in] dt2 Time step of the time stage just before the impulse.
/// @param[in] s Split solution of the time stage 2 stage before the impulse.
/// @param[in, out] sc_residual Residual of the switching constraint. 
///
void computeSwitchingConstraintResidual(
    Robot& robot, const ImpulseStatus& impulse_status, const double dt1, 
    const double dt2, const SplitSolution& s, 
    SplitSwitchingConstraintResidual& sc_residual);

} // namespace switchingconstraint 

} // namespace roboc

#include "roboc/ocp/switching_constraint.hxx"

#endif // ROBOC_SWITCHING_CONSTRAINT_HPP_ 