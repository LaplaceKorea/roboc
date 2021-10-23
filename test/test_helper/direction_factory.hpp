#ifndef ROBOC_TEST_HELPER_DIRECTION_FACTORY_HPP_
#define ROBOC_TEST_HELPER_DIRECTION_FACTORY_HPP_

#include "roboc/robot/robot.hpp"
#include "roboc/hybrid/contact_sequence.hpp"
#include "roboc/ocp/direction.hpp"


namespace roboc {
namespace testhelper {

Direction CreateDirection(const Robot& robot, const int N, 
                          const int max_num_impulse=0);

Direction CreateDirection(const Robot& robot, const ContactSequence& contact_sequence, 
                          const double T, const int N, const int max_num_impulse, const double t);


} // namespace testhelper
} // namespace roboc

#endif // ROBOC_TEST_HELPER_DIRECTION_FACTORY_HPP_ 