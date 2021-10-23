#include "roboc/ocp/direction.hpp"


namespace roboc {

std::ostream& operator<<(std::ostream& os, const Direction& d) {
  os << "direction:" << std::endl;
  d.disp(os);
  return os;
}

} // namespace roboc 