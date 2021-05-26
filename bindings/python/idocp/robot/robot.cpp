#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "idocp/robot/robot.hpp"


namespace idocp {
namespace python {

namespace py = pybind11;

std::vector<int> empty_contact_frames = {};
std::pair<double, double> zero_baumgarte_weights = {0., 0.};

PYBIND11_MODULE(robot, m) {
  py::enum_<BaseJointType>(m, "BaseJointType", py::arithmetic())
    .value("FixedBase",  BaseJointType::FixedBase)
    .value("FloatingBase", BaseJointType::FloatingBase)
    .export_values();

  py::class_<Robot>(m, "Robot")
    .def(py::init<const std::string&, const BaseJointType&, 
                  const std::vector<int>&, 
                  const std::pair<double, double>&>(),
         py::arg("path_to_urdf"),
         py::arg("base_joint_type")=BaseJointType::FixedBase,
         py::arg("contact_frames")=empty_contact_frames,
         py::arg("baumgarte_weights")=zero_baumgarte_weights)
    .def(py::init<const std::string&, const BaseJointType&, 
                  const std::vector<int>&, const double>())
    .def("forward_kinematics", [](Robot& self, const Eigen::VectorXd& q) {
        self.updateFrameKinematics(q);
      })
    .def("frame_position", &Robot::framePosition)
    .def("frame_rotation", &Robot::frameRotation)
    .def("com", &Robot::CoM)
    .def("generate_feasible_configuration", &Robot::generateFeasibleConfiguration)
    .def("normalize_configuration", [](const Robot& self, Eigen::VectorXd& q) {
        self.normalizeConfiguration(q);
      })
    .def("create_contact_status", &Robot::createContactStatus)
    .def("get_contact_points", [](const Robot& self, 
                                  ContactStatus& contact_status) {
        self.getContactPoints(contact_status);
      })
    .def("set_joint_effort_limit", &Robot::setJointEffortLimit)
    .def("set_joint_velocity_limit", &Robot::setJointVelocityLimit)
    .def("set_lower_joint_position_limit", &Robot::setLowerJointPositionLimit)
    .def("set_upper_joint_position_limit", &Robot::setUpperJointPositionLimit)
    .def("print_robot_model", &Robot::printRobotModel);
}

} // namespace python
} // namespace idocp