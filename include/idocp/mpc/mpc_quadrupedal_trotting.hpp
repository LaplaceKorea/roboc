#ifndef IDOCP_MPC_QUADRUPEDAL_TROTTING_HPP_
#define IDOCP_MPC_QUADRUPEDAL_TROTTING_HPP_

#include <vector>
#include <memory>
#include <limits>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/solver/ocp_solver.hpp"
#include "idocp/cost/cost_function.hpp"
#include "idocp/constraints/constraints.hpp"


namespace idocp {

///
/// @class MPCQuadrupedalTrotting
/// @brief MPC solver for the trotting gait of quadrupeds. 
///
class MPCQuadrupedalTrotting {
public:
  ///
  /// @brief Construct MPC solver.
  /// @param[in] robot Robot model. 
  /// @param[in] cost Shared ptr to the cost function.
  /// @param[in] constraints Shared ptr to the constraints.
  /// @param[in] T Length of the horizon. Must be positive.
  /// @param[in] N Number of discretization of the horizon. Must be more than 1. 
  /// @param[in] max_num_steps Maximum number of the steps on the horizon. 
  /// Must be positive. 
  /// @param[in] nthreads Number of the threads in solving the optimal control 
  /// problem. Must be positive. 
  ///
  MPCQuadrupedalTrotting(const Robot& robot, 
                         const std::shared_ptr<CostFunction>& cost, 
                         const std::shared_ptr<Constraints>& constraints, 
                         const double T, const int N, const int max_num_steps, 
                         const int nthreads);

  ///
  /// @brief Default constructor. 
  ///
  MPCQuadrupedalTrotting();

  ///
  /// @brief Destructor. 
  ///
  ~MPCQuadrupedalTrotting();

  ///
  /// @brief Default copy constructor. 
  ///
  MPCQuadrupedalTrotting(const MPCQuadrupedalTrotting&) = default;

  ///
  /// @brief Default copy assign operator. 
  ///
  MPCQuadrupedalTrotting& operator=(const MPCQuadrupedalTrotting&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  MPCQuadrupedalTrotting(MPCQuadrupedalTrotting&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  MPCQuadrupedalTrotting& operator=(MPCQuadrupedalTrotting&&) noexcept = default;

  ///
  /// @brief Sets the gait pattern. 
  /// @param[in] step_length Step length of the gait. 
  /// @param[in] step_height Step height of the gait. 
  /// @param[in] swing_time Swing time of the gait. 
  /// @param[in] t0 Start time of the gait. 
  ///
  void setGaitPattern(const double step_length, const double step_height,
                      const double swing_time, const double t0);

  ///
  /// @brief Initializes the optimal control problem solover. 
  /// @param[in] t Initial time of the horizon. 
  /// @param[in] q Initial configuration. Size must be Robot::dimq().
  /// @param[in] v Initial velocity. Size must be Robot::dimv().
  /// @param[in] num_iteration Number of the iterations. Must be non-negative.
  /// If num_iteration is zero, only the slack and dual variables of the 
  /// interior point method are initialized.
  ///
  void init(const double t, const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
            const int num_iteration);

  ///
  /// @brief Updates the solution by iterationg the Newton-type method.
  /// @param[in] t Initial time of the horizon. 
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] v Velocity. Size must be Robot::dimv().
  /// @param[in] num_iteration Number of the Newton-type iterations.
  ///
  void updateSolution(const double t, const Eigen::VectorXd& q, 
                      const Eigen::VectorXd& v, const int num_iteration);

  ///
  /// @brief Get the initial control input.
  /// @return Const reference to the control input.
  ///
  const Eigen::VectorXd& getInitialControlInput() const;

  ///
  /// @brief Computes the KKT residual of the optimal control problem. 
  /// @param[in] t Initial time of the horizon. 
  /// @param[in] q Initial configuration. Size must be Robot::dimq().
  /// @param[in] v Initial velocity. Size must be Robot::dimv().
  ///
  double KKTError(const double t, const Eigen::VectorXd& q, 
                  const Eigen::VectorXd& v);

  ///
  /// @brief Returns the l2-norm of the KKT residuals.
  /// MPCQuadrupedalTrotting::updateSolution() must be computed.  
  /// @return The l2-norm of the KKT residual.
  ///
  double KKTError();

  ///
  /// @brief Shows the information of the discretized optimal control problem
  /// onto console.
  ///
  void showInfo() const;

  static constexpr double min_dt 
      = std::sqrt(std::numeric_limits<double>::epsilon());

private:
  Robot robot_;
  OCPSolver ocp_solver_;
  ContactStatus cs_standing_, cs_lfrh_, cs_rflh_;
  std::vector<Eigen::Vector3d> contact_points_;
  double step_length_, step_height_, swing_time_, t0_, T_, dt_, dtm_, ts_last_;
  int N_, current_step_, predict_step_;

  bool addStep(const double t);

  void resetContactPoints(const Eigen::VectorXd& q);

};

} // namespace idocp 


#endif // IDOCP_MPC_QUADRUPEDAL_TROTTING_HPP_ 