#ifndef ROBOC_SOLUTION_HPP_
#define ROBOC_SOLUTION_HPP_

#include <iostream>

#include "roboc/hybrid/hybrid_container.hpp"
#include "roboc/ocp/split_solution.hpp"
#include "roboc/impulse/impulse_split_solution.hpp"


namespace roboc {

///
/// @typedef Solution
/// @brief Solution to the (hybrid) optimal control problem. 
///
using Solution = hybrid_container<SplitSolution, ImpulseSplitSolution>;

std::ostream& operator<<(std::ostream& os, const Solution& s);

} // namespace roboc

#endif // ROBOC_SOLUTION_HPP_ 