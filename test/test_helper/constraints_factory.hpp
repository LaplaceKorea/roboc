#ifndef ROBOC_TEST_HELPER_CONSTRAINTS_FACTORY_HPP_
#define ROBOC_TEST_HELPER_CONSTRAINTS_FACTORY_HPP_

#include <memory>

#include "roboc/constraints/constraints.hpp"


namespace roboc {
namespace testhelper {

std::shared_ptr<Constraints> CreateConstraints(const Robot& robot);

} // namespace testhelper
} // namespace roboc

#endif // ROBOC_TEST_HELPER_CONSTRAINTS_FACTORY_HPP_