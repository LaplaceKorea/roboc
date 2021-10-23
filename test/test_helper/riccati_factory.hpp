#ifndef ROBOC_TEST_RICCATI_FACTORY_HPP_
#define ROBOC_TEST_RICCATI_FACTORY_HPP_

#include "roboc/robot/robot.hpp"
#include "roboc/riccati/split_riccati_factorization.hpp"


namespace roboc {
namespace testhelper {

SplitRiccatiFactorization CreateSplitRiccatiFactorization(const Robot& robot);

} // namespace testhelper
} // namespace roboc

#endif // ROBOC_TEST_RICCATI_FACTORY_HPP_ 