#ifndef IDOCP_MANIPULATOR_COST_FUNCTION_HPP_
#define IDOCP_MANIPULATOR_COST_FUNCTION_HPP_

#include "Eigen/Core"

#include "robot/robot.hpp"
#include "cost/cost_function_interface.hpp"
#include "cost/joint_space_cost.hpp"


namespace idocp {
namespace manipulator {

class CostFunction final : public CostFunctionInterface {
public:
  CostFunction(const Robot& robot);

  ~CostFunction();

  void set_q_ref(const Eigen::VectorXd& q_ref);

  void set_v_ref(const Eigen::VectorXd& v_ref);

  void set_a_ref(const Eigen::VectorXd& a_ref);

  void set_u_ref(const Eigen::VectorXd& u_ref);

  void set_q_weight(const Eigen::VectorXd& q_weight);

  void set_v_weight(const Eigen::VectorXd& v_weight);

  void set_a_weight(const Eigen::VectorXd& a_weight);

  void set_u_weight(const Eigen::VectorXd& u_weight);

  void set_qf_weight(const Eigen::VectorXd& qf_weight);

  void set_vf_weight(const Eigen::VectorXd& vf_weight);

  double l(const Robot& robot, const double t, const double dtau,
           const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
           const Eigen::VectorXd& u, const Eigen::VectorXd& a) override; 

  double phi(const Robot& robot, const double t, const Eigen::VectorXd& q, 
             const Eigen::VectorXd& v) override;

  void lq(const Robot& robot, const double t, const double dtau,
          const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
          const Eigen::VectorXd& a, Eigen::VectorXd& lq) override;

  void lv(const Robot& robot, const double t, const double dtau,
          const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
          const Eigen::VectorXd& a, Eigen::VectorXd& lv) override;

  void la(const Robot& robot, const double t, const double dtau,
          const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
          const Eigen::VectorXd& a, Eigen::VectorXd& la) override;

  void lu(const Robot& robot, const double t, const double dtau,
          const Eigen::VectorXd& u, Eigen::VectorXd& lu) override;

  void lqq(const Robot& robot, const double t, const double dtau,
           const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
           const Eigen::VectorXd& a, Eigen::MatrixXd& lqq) override;

  void lvv(const Robot& robot, const double t, const double dtau,
           const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
           const Eigen::VectorXd& a, Eigen::MatrixXd& lvv) override;

  void laa(const Robot& robot, const double t, const double dtau,
           const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
           const Eigen::VectorXd& a, Eigen::MatrixXd& laa) override;

  void luu(const Robot& robot, const double t, const double dtau,
           const Eigen::VectorXd& u, Eigen::MatrixXd& luu) override;

  void phiq(const Robot& robot, const double t, const Eigen::VectorXd& q, 
            const Eigen::VectorXd& v, Eigen::VectorXd& phiq) override;

  void phiv(const Robot& robot, const double t, const Eigen::VectorXd& q, 
            const Eigen::VectorXd& v, Eigen::VectorXd& phiv) override;

  void phiqq(const Robot& robot, const double t, const Eigen::VectorXd& q, 
             const Eigen::VectorXd& v, Eigen::MatrixXd& phiqq) override;

  void phivv(const Robot& robot, const double t, const Eigen::VectorXd& q, 
             const Eigen::VectorXd& v, Eigen::MatrixXd& phivv) override;

private:
  JointSpaceCost joint_space_cost_;  

};

} // namespace manipulator
} // namespace idocp

#endif // IDOCP_MANIPULATOR_COST_FUNCTION_HPP_