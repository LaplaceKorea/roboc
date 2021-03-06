#include "robot_factory.hpp"

#include <string>

#include "idocp/robot/robot.hpp"


namespace idocp {
namespace testhelper {

Robot CreateFixedBaseRobot() {
  const std::string fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
  return Robot(fixed_base_urdf);
}


Robot CreateFixedBaseRobot(const double time_step) {
  assert(time_step >= 0);
  const std::string fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
  const std::vector<int> contact_frames = {18};
  return Robot(fixed_base_urdf, BaseJointType::FixedBase, contact_frames, time_step);
}


Robot CreateFloatingBaseRobot() {
  const std::string floating_base_urdf = "../urdf/anymal/anymal.urdf";
  return Robot(floating_base_urdf, BaseJointType::FloatingBase);
}


Robot CreateFloatingBaseRobot(const double time_step) {
  assert(time_step >= 0);
  const std::string floating_base_urdf = "../urdf/anymal/anymal.urdf";
  const std::vector<int> contact_frames = {12, 22, 32, 42};
  return Robot(floating_base_urdf, BaseJointType::FloatingBase, contact_frames, time_step);
}

} // namespace testhelper
} // namespace idocp