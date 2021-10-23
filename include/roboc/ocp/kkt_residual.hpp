#ifndef ROBOC_KKT_RESIDUAL_HPP_
#define ROBOC_KKT_RESIDUAL_HPP_

#include <iostream>

#include "roboc/hybrid/hybrid_container.hpp"
#include "roboc/ocp/split_kkt_residual.hpp"
#include "roboc/impulse/impulse_split_kkt_residual.hpp"
#include "roboc/ocp/split_switching_constraint_residual.hpp"


namespace roboc {

///
/// @typedef KKTResidual 
/// @brief The KKT residual of the (hybrid) optimal control problem. 
///
using KKTResidual = hybrid_container<SplitKKTResidual, ImpulseSplitKKTResidual, 
                                     SplitSwitchingConstraintResidual>;

std::ostream& operator<<(std::ostream& os, const KKTResidual& kkt_residual);

} // namespace roboc

#endif // ROBOC_KKT_RESIDUAL_HPP_ 