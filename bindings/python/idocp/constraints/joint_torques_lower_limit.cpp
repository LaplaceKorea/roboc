#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "idocp/constraints/joint_torques_lower_limit.hpp"


namespace idocp {
namespace python {

namespace py = pybind11;

PYBIND11_MODULE(joint_torques_lower_limit, m) {
  py::class_<JointTorquesLowerLimit, ConstraintComponentBase, 
             std::shared_ptr<JointTorquesLowerLimit>>(m, "JointTorquesLowerLimit")
    .def(py::init<const Robot&, const double, const double>(),
         py::arg("robot"), py::arg("barrier")=1.0e-04,
         py::arg("fraction_to_boundary_rate")=0.995);

  m.def("create_joint_torques_lower_limit", [](const Robot& robot) {
    return std::make_shared<JointTorquesLowerLimit>(robot);
  });
}

} // namespace python
} // namespace idocp