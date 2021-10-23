#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "roboc/constraints/joint_acceleration_lower_limit.hpp"


namespace roboc {
namespace python {

namespace py = pybind11;

PYBIND11_MODULE(joint_acceleration_lower_limit, m) {
  py::class_<JointAccelerationLowerLimit, ConstraintComponentBase, 
             std::shared_ptr<JointAccelerationLowerLimit>>(m, "JointAccelerationLowerLimit")
    .def(py::init<const Robot&, const Eigen::VectorXd&, const double, const double>(),
         py::arg("robot"), py::arg("amin"), py::arg("barrier")=1.0e-04,
         py::arg("fraction_to_boundary_rule")=0.995);
}

} // namespace python
} // namespace roboc