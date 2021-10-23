#include "roboc/ocp/kkt_matrix.hpp"


namespace roboc {

std::ostream& operator<<(std::ostream& os, const KKTMatrix& kkt_matrix) {
  os << "KKT matrix:" << std::endl;
  kkt_matrix.disp(os);
  return os;
}

} // namespace roboc 