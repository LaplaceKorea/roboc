#ifndef ROBOC_KKT_MATRIX_HPP_
#define ROBOC_KKT_MATRIX_HPP_

#include <iostream>

#include "roboc/hybrid/hybrid_container.hpp"
#include "roboc/ocp/split_kkt_matrix.hpp"
#include "roboc/impulse/impulse_split_kkt_matrix.hpp"
#include "roboc/ocp/split_switching_constraint_jacobian.hpp"


namespace roboc {

///
/// @typedef KKTMatrix 
/// @brief The KKT matrix of the (hybrid) optimal control problem. 
///
using KKTMatrix = hybrid_container<SplitKKTMatrix, ImpulseSplitKKTMatrix, 
                                   SplitSwitchingConstraintJacobian>;

std::ostream& operator<<(std::ostream& os, const KKTMatrix& kkt_matrix);

} // namespace roboc

#endif // ROBOC_KKT_MATRIX_HPP_ 