#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "idocp/cost/time_varying_configuration_space_cost.hpp"


namespace idocp {
namespace python {

namespace py = pybind11;

PYBIND11_MODULE(time_varying_configuration_space_cost, m) {
  py::class_<TimeVaryingConfigurationSpaceCost, 
             CostFunctionComponentBase,
             std::shared_ptr<TimeVaryingConfigurationSpaceCost>>(m, "TimeVaryingConfigurationSpaceCost")
    .def(py::init<const Robot&, 
                  const std::shared_ptr<TimeVaryingConfigurationRefBase>&>())
    .def("set_ref", &TimeVaryingConfigurationSpaceCost::set_ref)
    .def("set_q_weight", &TimeVaryingConfigurationSpaceCost::set_q_weight)
    .def("set_qf_weight", &TimeVaryingConfigurationSpaceCost::set_qf_weight)
    .def("set_qi_weight", &TimeVaryingConfigurationSpaceCost::set_qi_weight);

  m.def("create_time_varying_configuration_space_cost", [](const Robot& robot, 
                                                           const std::shared_ptr<TimeVaryingConfigurationRefBase>& ref) {
    return std::make_shared<TimeVaryingConfigurationSpaceCost>(robot, ref);
  });
}

} // namespace python
} // namespace idocp