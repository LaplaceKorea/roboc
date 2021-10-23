#ifndef ROBOC_UTILS_JOINT_CONSTRAINTS_FACTORY_HPP_
#define ROBOC_UTILS_JOINT_CONSTRAINTS_FACTORY_HPP_

#include <memory>

#include "roboc/robot/robot.hpp"
#include "roboc/constraints/constraints.hpp"

namespace roboc {

class JointConstraintsFactory {
public:
  JointConstraintsFactory(const Robot& robot);

  ~JointConstraintsFactory();

  std::shared_ptr<roboc::Constraints> create() const;

private:
  Robot robot_;

};

} // namespace roboc

#endif // ROBOC_UTILS_JOINT_CONSTRAINTS_FACTORY_HPP_