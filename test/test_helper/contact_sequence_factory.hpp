#ifndef ROBOC_TEST_HELPER_CONTACT_SEQUENCE_FACTORY_HPP_
#define ROBOC_TEST_HELPER_CONTACT_SEQUENCE_FACTORY_HPP_

#include "roboc/robot/robot.hpp"
#include "roboc/hybrid/contact_sequence.hpp"


namespace roboc {
namespace testhelper {

ContactSequence CreateContactSequence(const Robot& robot, const int N, 
                                      const int max_num_impulse,
                                      const double t0,
                                      const double event_period);

} // namespace testhelper
} // namespace roboc

#endif // ROBOC_TEST_HELPER_CONTACT_SEQUENCE_FACTORY_HPP_ 