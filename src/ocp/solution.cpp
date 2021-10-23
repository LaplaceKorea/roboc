#include "roboc/ocp/solution.hpp"


namespace roboc {

std::ostream& operator<<(std::ostream& os, const Solution& s) {
  os << "solution:" << std::endl;
  s.disp(os);
  return os;
}

} // namespace roboc 