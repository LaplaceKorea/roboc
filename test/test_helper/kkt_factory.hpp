#ifndef ROBOC_TEST_HELPER_KKT_FACTORY_HPP_
#define ROBOC_TEST_HELPER_KKT_FACTORY_HPP_

#include "roboc/robot/robot.hpp"
#include "roboc/hybrid/contact_sequence.hpp"
#include "roboc/ocp/ocp.hpp"
#include "roboc/ocp/kkt_matrix.hpp"
#include "roboc/ocp/kkt_residual.hpp"


namespace roboc {
namespace testhelper {

SplitKKTMatrix CreateSplitKKTMatrix(const Robot& robot, const double dt);

ImpulseSplitKKTMatrix CreateImpulseSplitKKTMatrix(const Robot& robot);

SplitKKTResidual CreateSplitKKTResidual(const Robot& robot);

SplitKKTResidual CreateSplitKKTResidual(const Robot& robot);

ImpulseSplitKKTResidual CreateImpulseSplitKKTResidual(const Robot& robot);

KKTMatrix CreateKKTMatrix(const Robot& robot, const ContactSequence& contact_sequence, 
                          const int N, const int max_num_impulse);

KKTResidual CreateKKTResidual(const Robot& robot, const ContactSequence& contact_sequence, 
                              const int N, const int max_num_impulse);

} // namespace testhelper
} // namespace roboc

#endif // ROBOC_TEST_HELPER_KKT_FACTORY_HPP_