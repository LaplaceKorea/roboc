#ifndef ROBOC_TEST_HELPER_ROBOT_FACTORY_HPP_
#define ROBOC_TEST_HELPER_ROBOT_FACTORY_HPP_

#include "roboc/robot/robot.hpp"


namespace roboc {
namespace testhelper {

Robot CreateFixedBaseRobot();

Robot CreateFixedBaseRobot(const double time_step);

Robot CreateFloatingBaseRobot();

Robot CreateFloatingBaseRobot(const double time_step);

} // namespace testhelper
} // namespace roboc

#endif // ROBOC_TEST_HELPER_ROBOT_FACTORY_HPP_ 