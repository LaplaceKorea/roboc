#include "roboc/ocp/kkt_residual.hpp"


namespace roboc {

std::ostream& operator<<(std::ostream& os, const KKTResidual& kkt_residual) {
  os << "KKT residual:" << std::endl;
  kkt_residual.disp(os);
  return os;
}

} // namespace roboc 