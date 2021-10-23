#ifndef ROBOC_DIRECTION_HPP_
#define ROBOC_DIRECTION_HPP_

#include <iostream>

#include "roboc/hybrid/hybrid_container.hpp"
#include "roboc/ocp/split_direction.hpp"
#include "roboc/impulse/impulse_split_direction.hpp"


namespace roboc {

///
/// @typedef Direction
/// @brief Newton direction of the solution to the (hybrid) optimal control 
/// problem. 
///
using Direction = hybrid_container<SplitDirection, ImpulseSplitDirection>;

std::ostream& operator<<(std::ostream& os, const Direction& d);

} // namespace roboc

#endif // ROBOC_DIRECTION_HPP_ 