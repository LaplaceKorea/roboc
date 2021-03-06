#ifndef IDOCP_PERIODIC_FOOT_TRACK_REF2_HPP_
#define IDOCP_PERIODIC_FOOT_TRACK_REF2_HPP_

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/cost/time_varying_task_space_3d_cost.hpp"


namespace idocp {

///
/// @class PeriodicFootTrackRef2
/// @brief Periodic reference of the foot position. 
///
class PeriodicFootTrackRef2 : public TimeVaryingTaskSpace3DRefBase {
public:
  ///
  /// @brief Constructor. 
  /// @param[in] p0 Initial foot position reference.
  /// @param[in] step_length The step length of the gait.
  /// @param[in] step_height The step height of the gait.
  /// @param[in] t0 Start time of the reference tracking.
  /// @param[in] period_swing Period where the foot is swinging.
  /// @param[in] period_stance Period where the foot is stancing.
  /// @param[in] is_first_step_half If true, the length ofh te first reference 
  /// foot step is half. 
  ///
  PeriodicFootTrackRef2(const Eigen::Vector3d p0, const double step_length, 
                        const double step_height, const double t0, 
                        const double period_swing, const double period_stance, 
                        const bool is_first_step_half);

  ///
  /// @brief Destructor. 
  ///
  ~PeriodicFootTrackRef2();

  void update_q_3d_ref(const double t, Eigen::VectorXd& q_3d_ref) const override;

  bool isActive(const double t) const override;

private:
  Eigen::Vector3d p0_;
  double step_length_, step_height_, t0_, period_swing_, period_stance_,
         period_;
  bool is_first_step_half_;

};

} // namespace idocp

#endif // IDOCP_PERIODIC_FOOT_TRACK_REF2_HPP_ 