#ifndef INVDYNOCP_IIWA14_COST_FUNCTION_HPP_
#define INVDYNOCP_IIWA14_COST_FUNCTION_HPP_

#include "Eigen/Core"

#include "robot/robot.hpp"
#include "cost/cost_function_interface.hpp"
#include "cost/configuration_space_cost.hpp"


namespace invdynocp {
namespace iiwa14 {

class CostFunction final : public CostFunctionInterface {
public:
  CostFunction(const Robot* robot_ptr, const Eigen::VectorXd& q_ref);

  ~CostFunction();

  void lq(const Robot* robot_ptr, const double t, const double dtau,
          const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
          const Eigen::VectorXd& a, const Eigen::VectorXd& u, 
          Eigen::VectorXd& lq) override;

  void lv(const Robot* robot_ptr, const double t, const double dtau,
          const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
          const Eigen::VectorXd& a, const Eigen::VectorXd& u, 
          Eigen::VectorXd& lv) override;

  void la(const Robot* robot_ptr, const double t, const double dtau,
          const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
          const Eigen::VectorXd& a, const Eigen::VectorXd& u, 
          Eigen::VectorXd& la) override;

  void lu(const Robot* robot_ptr, const double t, const double dtau,
          const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
          const Eigen::VectorXd& a, const Eigen::VectorXd& u, 
          Eigen::VectorXd& lu) override;

  void lqq(const Robot* robot_ptr, const double t, const double dtau,
           const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
           const Eigen::VectorXd& a, const Eigen::VectorXd& u, 
           Eigen::MatrixXd& lqq) override;

  void lvv(const Robot* robot_ptr, const double t, const double dtau,
           const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
           const Eigen::VectorXd& a, const Eigen::VectorXd& u, 
           Eigen::MatrixXd& lvv) override;

  void laa(const Robot* robot_ptr, const double t, const double dtau,
           const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
           const Eigen::VectorXd& a, const Eigen::VectorXd& u, 
           Eigen::MatrixXd& laa) override;

  void luu(const Robot* robot_ptr, const double t, const double dtau,
           const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
           const Eigen::VectorXd& a, const Eigen::VectorXd& u, 
           Eigen::MatrixXd& luu) override;

  void phiq(const Robot* robot_ptr, const double t, const Eigen::VectorXd& q, 
            const Eigen::VectorXd& v, Eigen::VectorXd& phiq) override;

  void phiv(const Robot* robot_ptr, const double t, const Eigen::VectorXd& q, 
            const Eigen::VectorXd& v, Eigen::VectorXd& phiv) override;

  void phiqq(const Robot* robot_ptr, const double t, const Eigen::VectorXd& q, 
             const Eigen::VectorXd& v, Eigen::MatrixXd& phiqq) override;

  void phivv(const Robot* robot_ptr, const double t, const Eigen::VectorXd& q, 
             const Eigen::VectorXd& v, Eigen::MatrixXd& phivv) override;

private:
  ConfigurationSpaceCost configuration_space_cost_;  

};

} // namespace iiwa14
} // namespace invdynocp

#endif // INVDYNOCP_IIWA14_COST_FUNCTION_HPP_