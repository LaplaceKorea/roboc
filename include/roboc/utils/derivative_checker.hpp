#ifndef ROBOC_DERIVATIVE_CHECKER_HPP_
#define ROBOC_DERIVATIVE_CHECKER_HPP_

#include <memory>

#include "roboc/robot/robot.hpp"
#include "roboc/robot/contact_status.hpp"
#include "roboc/robot/impulse_status.hpp"
#include "roboc/cost/cost_function_component_base.hpp"


namespace roboc {

class DerivativeChecker {
public:
  explicit DerivativeChecker(const Robot& robot, 
                             const double finite_diff=1.0e-08, 
                             const double test_tol=1.0e-04);

  ~DerivativeChecker();

  void setFiniteDifference(const double finite_diff=1.0e-08);

  void setTestTolerance(const double test_tol=1.0e-04);

  bool checkFirstOrderStageCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost);

  bool checkSecondOrderStageCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost);

  bool checkFirstOrderStageCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost, 
      const ContactStatus& contact_status);

  bool checkSecondOrderStageCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost,
      const ContactStatus& contact_status);

  bool checkFirstOrderTerminalCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost);

  bool checkSecondOrderTerminalCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost);

  bool checkFirstOrderImpulseCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost);

  bool checkSecondOrderImpulseCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost);

  bool checkFirstOrderImpulseCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost, 
      const ImpulseStatus& impulse_status);

  bool checkSecondOrderImpulseCostDerivatives(
      const std::shared_ptr<CostFunctionComponentBase>& cost,
      const ImpulseStatus& impulse_status);

private:  
  Robot robot_;
  double finite_diff_, test_tol_;

};
  
} // namespace roboc 

#endif // ROBOC_DERIVATIVE_CHECKER_HPP_