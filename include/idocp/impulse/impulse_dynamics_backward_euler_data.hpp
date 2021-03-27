#ifndef IDOCP_IMPULSE_DYNAMICS_BACKWARD_EULER_DATA_HPP_ 
#define IDOCP_IMPULSE_DYNAMICS_BACKWARD_EULER_DATA_HPP_

#include "Eigen/Core"
#include "idocp/robot/robot.hpp"
#include "idocp/robot/impulse_status.hpp"


namespace idocp {

///
/// @class ImpulseDynamicsBackwardEulerData
/// @brief Data used in ImpulseDynamicsBackwardEuler.
///
class ImpulseDynamicsBackwardEulerData {
public:
  ///
  /// @brief Constructs a data.
  /// @param[in] robot Robot model. 
  ///
  ImpulseDynamicsBackwardEulerData(const Robot& robot);

  ///
  /// @brief Default constructor. 
  ///
  ImpulseDynamicsBackwardEulerData();

  ///
  /// @brief Destructor. 
  ///
  ~ImpulseDynamicsBackwardEulerData();

  ///
  /// @brief Default copy constructor. 
  ///
  ImpulseDynamicsBackwardEulerData(
      const ImpulseDynamicsBackwardEulerData&) = default;

  ///
  /// @brief Default copy operator. 
  ///
  ImpulseDynamicsBackwardEulerData& operator=(
      const ImpulseDynamicsBackwardEulerData&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  ImpulseDynamicsBackwardEulerData(
      ImpulseDynamicsBackwardEulerData&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  ImpulseDynamicsBackwardEulerData& operator=(
      ImpulseDynamicsBackwardEulerData&&) noexcept = default;

  ///
  /// @brief Set the impulse status, i.e., set dimension of the impulses.
  /// @param[in] impulse_status Impulse status.
  ///
  void setImpulseStatus(const ImpulseStatus& Impulse_status);

  Eigen::Block<Eigen::MatrixXd> Qdvf();

  const Eigen::Block<const Eigen::MatrixXd> Qdvf() const;

  bool checkDimensions() const;

  Eigen::MatrixXd dImDdq;

  Eigen::MatrixXd dImDddv;

  Eigen::MatrixXd Minv;

  Eigen::MatrixXd Qdvq;

  Eigen::VectorXd ImD;

  Eigen::VectorXd Minv_ImD;

  Eigen::VectorXd ldv;

private:
  Eigen::MatrixXd Qdvf_full_;
  int dimv_, dimf_;

};

} // namespace idocp 

#include "idocp/impulse/impulse_dynamics_backward_euler_data.hxx"

#endif // IDOCP_IMPULSE_DYNAMICS_BACKWARD_EULER_DATA_HPP_ 