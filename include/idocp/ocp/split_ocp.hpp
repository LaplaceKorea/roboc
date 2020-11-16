#ifndef IDOCP_SPLIT_OCP_HPP_
#define IDOCP_SPLIT_OCP_HPP_

#include <memory>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/cost/cost_function.hpp"
#include "idocp/cost/cost_function_data.hpp"
#include "idocp/constraints/constraints.hpp"
#include "idocp/constraints/constraints_data.hpp"
#include "idocp/ocp/state_equation.hpp"
// #include "idocp/ocp/robot_dynamics.hpp"
#include "idocp/ocp/contact_dynamics.hpp"
#include "idocp/ocp/riccati_factorization.hpp"
#include "idocp/ocp/riccati_factorizer.hpp"
#include "idocp/ocp/state_constraint_riccati_factorization.hpp"


namespace idocp {

///
/// @class SplitOCP
/// @brief Split optimal control problem of a single stage. 
///
class SplitOCP {
public:
  ///
  /// @brief Construct a split optimal control problem.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] cost Shared ptr to the cost function.
  /// @param[in] constraints Shared ptr to the constraints.
  ///
  SplitOCP(const Robot& robot, const std::shared_ptr<CostFunction>& cost,
           const std::shared_ptr<Constraints>& constraints);

  ///
  /// @brief Default constructor.  
  ///
  SplitOCP();

  ///
  /// @brief Destructor. 
  ///
  ~SplitOCP();

  ///
  /// @brief Default copy constructor. 
  ///
  SplitOCP(const SplitOCP&) = default;

  ///
  /// @brief Default copy assign operator. 
  ///
  SplitOCP& operator=(const SplitOCP&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  SplitOCP(SplitOCP&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  SplitOCP& operator=(SplitOCP&&) noexcept = default;

  ///
  /// @brief Check whether the solution is feasible under inequality constraints.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] s Split solution of this stage.
  ///
  bool isFeasible(Robot& robot, const SplitSolution& s);

  ///
  /// @brief Initialize the constraints, i.e., set slack and dual variables. 
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] time_step Time step of this stage.
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] s Split solution of this stage.
  ///
  void initConstraints(Robot& robot, const int time_step, const double dtau, 
                       const SplitSolution& s);

  ///
  /// @brief Linearize the OCP for Newton's method around the current solution, 
  /// i.e., computes the KKT residual and Hessian.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] contact_status Contact status of robot at this stage. 
  /// @param[in] t Current time of this stage. 
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] q_prev Configuration of the previous stage.
  /// @param[in] s Split solution of this stage.
  /// @param[in] s_next Split solution of the next stage.
  ///
  template <typename SplitSolutionType>
  void linearizeOCP(Robot& robot, const ContactStatus& contact_status, 
                    const double t, const double dtau, 
                    const Eigen::VectorXd& q_prev, const SplitSolution& s, 
                    const SplitSolutionType& s_next);

  ///
  /// @brief Computes the Riccati factorization of this stage from the 
  /// factorization of the previous stage.
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] riccati_next Riccati factorization of the next stage.
  /// @param[out] riccati Riccati factorization of this stage.
  /// 
  void backwardRiccatiRecursion(const RiccatiFactorization& riccati_next,
                                const double dtau, 
                                RiccatiFactorization& riccati);

  void backwardRiccatiRecursionInitial(const double dtau,
                                       RiccatiFactorization& riccati);

  ///
  /// @brief Computes the Newton direction of the state of this stage from the 
  /// one of the previous stage.
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] d Split direction of this stage.
  /// @param[out] d_next Split direction of the next stage.
  /// 
  void forwardRiccatiRecursionParallel(const bool exist_state_constraint=false);

  ///
  /// @brief Computes the Newton direction of the state of this stage from the 
  /// one of the previous stage.
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] d Split direction of this stage.
  /// @param[out] d_next Split direction of the next stage.
  /// 
  void forwardRiccatiRecursionSerial(const RiccatiFactorization& riccati,
                                     const double dtau,
                                     RiccatiFactorization& riccati_next,
                                     const bool exist_state_constraint=false);

  template <typename MatrixType1, typename MatrixType2>
  void backwardStateConstraintFactorization(
      const Eigen::MatrixBase<MatrixType1>& T_next, const double dtau,
      const Eigen::MatrixBase<MatrixType2>& T) const;

  void forwardRiccatiRecursionUnconstrained(SplitDirection& d,  
                                            const double dtau, 
                                            SplitDirection& d_next) const;


  ///
  /// @brief Computes the Newton direction of the condensed variables of this 
  /// stage.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] riccati Riccati factorization of this stage.
  /// @param[in] s Split solution of this stage.
  /// @param[in, out] d Split direction of this stage.
  /// 
  void computePrimalDirection(Robot& robot, const double dtau, 
                              const RiccatiFactorization& riccati,
                              const SplitSolution& s, SplitDirection& d, 
                              const bool exist_state_constraint);

  template <typename VectorType>
  void computePrimalDirection(Robot& robot, const double dtau, 
                              const RiccatiFactorization& riccati,
                              const SplitSolution& s, 
                              const Eigen::MatrixBase<VectorType>& dx0, 
                              SplitDirection& d, 
                              const bool exist_state_constraint);

  ///
  /// @brief Computes the Newton direction of the condensed variables of this 
  /// stage.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] d_next Split direction of the next stage.
  /// @param[in, out] d Split direction of this stage.
  /// 
  template <typename SplitDirectionType>
  void computeDualDirection(Robot& robot, const double dtau, 
                            const SplitDirectionType& d_next,
                            SplitDirection& d);

  ///
  /// @brief Returns maximum stap size of the primal variables that satisfies 
  /// the inequality constraints.
  /// @return Maximum stap size of the primal variables that satisfies 
  /// the inequality constraints.
  ///
  double maxPrimalStepSize();

  ///
  /// @brief Returns maximum stap size of the dual variables that satisfies 
  /// the inequality constraints.
  /// @return Maximum stap size of the dual variables that satisfies 
  /// the inequality constraints.
  ///
  double maxDualStepSize();

  ///
  /// @brief Updates dual variables of the inequality constraints.
  /// @param[in] dual_step_size Dula step size of the OCP. 
  ///
  void updateDual(const double dual_step_size);

  ///
  /// @brief Updates primal variables of this stage.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] primal_step_size Primal step size of the OCP. 
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] d Split direction of this stage.
  /// @param[in, out] s Split solution of this stage.
  ///
  void updatePrimal(Robot& robot, const double primal_step_size, 
                    const double dtau, const SplitDirection& d, 
                    SplitSolution& s);

  ///
  /// @brief Computes the KKT residual of the OCP at this stage.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] contact_status Contact status of robot at this stage. 
  /// @param[in] t Current time of this stage. 
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] s Split solution of this stage.
  /// @param[in] q_prev Configuration of the previous stage.
  /// @param[in] s Split solution of this stage.
  /// @param[in] s_next Split solution of the next stage.
  ///
  template <typename SplitSolutionType>
  void computeKKTResidual(Robot& robot, const ContactStatus& contact_status,
                          const double t, const double dtau, 
                          const Eigen::VectorXd& q_prev, const SplitSolution& s, 
                          const SplitSolutionType& s_next);

  ///
  /// @brief Returns the KKT residual of the OCP at this stage. Before calling 
  /// this function, SplitOCP::linearizeOCP() or SplitOCP::computeKKTResidual()
  /// must be called.
  /// @return The squared norm of the kKT residual.
  ///
  double squaredNormKKTResidual(const double dtau) const;

  ///
  /// @brief Computes the stage cost of this stage for line search.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] t Current time of this stage. 
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] s Split solution of this stage.
  /// @param[in] primal_step_size Primal step size of the OCP. Default is 0.
  /// @return Stage cost of this stage.
  /// 
  double stageCost(Robot& robot, const double t, const double dtau, 
                   const SplitSolution& s, const double primal_step_size=0);

  ///
  /// @brief Computes and returns the constraint violation of the OCP at this 
  /// stage for line search.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] contact_status Contact status of robot at this stage. 
  /// @param[in] t Current time of this stage. 
  /// @param[in] dtau Length of the discretization of the horizon.
  /// @param[in] s Split solution of this stage.
  /// @param[in] q_next Configuration at the next stage.
  /// @param[in] v_next Generaized velocity at the next stage.
  /// @return Constraint violation of this stage.
  ///
  double constraintViolation(Robot& robot, const ContactStatus& contact_status, 
                             const double t, const double dtau, 
                             const SplitSolution& s, 
                             const Eigen::VectorXd& q_next,
                             const Eigen::VectorXd& v_next);

  ///
  /// @brief Getter of the state feedback gain of the LQR subproblem. 
  /// @param[in] K The state feedback gain. 
  ///
  template <typename MatrixType>
  void getStateFeedbackGain(const Eigen::MatrixBase<MatrixType>& K) const;

  ///
  /// @brief Getter of the state feedback gain of the LQR subproblem. 
  /// @param[in] Kq The state feedback gain with respect to the configuration. 
  /// @param[in] Kv The state feedback gain with respect to the velocity. 
  ///
  template <typename MatrixType1, typename MatrixType2>
  void getStateFeedbackGain(const Eigen::MatrixBase<MatrixType1>& Kq,
                            const Eigen::MatrixBase<MatrixType2>& Kv) const;

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  std::shared_ptr<CostFunction> cost_;
  CostFunctionData cost_data_;
  std::shared_ptr<Constraints> constraints_;
  ConstraintsData constraints_data_;
  KKTResidual kkt_residual_;
  KKTMatrix kkt_matrix_;
  // RobotDynamics robot_dynamics_;
  ContactDynamics contact_dynamics_;
  RiccatiFactorizer riccati_factorizer_;
  bool use_kinematics_, has_floating_base_, fd_like_elimination_;
  double stage_cost_, constraint_violation_;

  ///
  /// @brief Set contact status from robot model, i.e., set dimension of the 
  /// contacts and equality constraints.
  /// @param[in] contact_status Contact status.
  ///
  inline void setContactStatusForKKT(const ContactStatus& contact_status) {
    kkt_residual_.setContactStatus(contact_status);
    kkt_matrix_.setContactStatus(contact_status);
    if (contact_status.hasActiveContacts()) {
      fd_like_elimination_ = true;
    }
    else {
      if (!has_floating_base_) {
        fd_like_elimination_ = false;
      }
    }
  }

};

} // namespace idocp

#include "idocp/ocp/split_ocp.hxx"

#endif // IDOCP_SPLIT_OCP_HPP_