#ifndef IDOCP_ROBOT_HPP_
#define IDOCP_ROBOT_HPP_

#include <string>
#include <vector>

#include "Eigen/Core"
#include "pinocchio/multibody/model.hpp"
#include "pinocchio/multibody/data.hpp"
#include "pinocchio/container/aligned-vector.hpp"
#include "pinocchio/spatial/force.hpp"

#include "idocp/robot/point_contact.hpp"
#include "idocp/robot/floating_base.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/robot/impulse_status.hpp"


namespace idocp {

///
/// @class Robot
/// @brief Dynamics and kinematics model of robots. Wraps pinocchio::Model and 
/// pinocchio::Data. Includes point contacts.
///
class Robot {
public:
  ///
  /// @brief Build the robot model and data from URDF. The model is assumed to
  /// have no contacts with the environment.
  /// @param[in] path_to_urdf Path to the URDF file.
  ///
  Robot(const std::string& path_to_urdf);

  ///
  /// @brief Build the robot model and data from URDF. The model is assumed to
  /// have no contacts with the environment.
  /// @param[in] path_to_urdf Path to the URDF file.
  /// @param[in] contact_frames Collection of the frames that can have contacts 
  /// with the environments.
  ///
  Robot(const std::string& path_to_urdf, 
        const std::vector<int>& contact_frames);

  ///
  /// @brief Default constructor. 
  ///
  Robot();

  ///
  /// @brief Destructor. 
  ///
  ~Robot();

  ///
  /// @brief Use default copy constructor. 
  ///
  Robot(const Robot&) = default;

  ///
  /// @brief Use default copy assign operator. 
  ///
  Robot& operator=(const Robot&) = default;

  ///
  /// @brief Use default move constructor. 
  ///
  Robot(Robot&&) noexcept = default;

  ///
  /// @brief Use default move assign operator. 
  ///
  Robot& operator=(Robot&&) noexcept = default;

  ///
  /// @brief Integrates the generalized velocity by integration_length * v. 
  // The configuration q is then incremented.
  /// @param[in, out] q Configuration. Size must be Robot::dimq().
  /// @param[in] v Generalized velocity. Size must be Robot::dimv().
  /// @param[in] integration_length The length of the integration.
  ///
  template <typename TangentVectorType, typename ConfigVectorType>
  void integrateConfiguration(
      const Eigen::MatrixBase<TangentVectorType>& v, 
      const double integration_length, 
      const Eigen::MatrixBase<ConfigVectorType>& q) const;

  ///
  /// @brief Integrates the generalized velocity by integration_length * v. 
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] v Generalized velocity. Size must be Robot::dimv().
  /// @param[in] integration_length The length of the integration.
  /// @param[out] q_integrated Resultant configuration. Size must be 
  /// Robot::dimq().
  ///
  template <typename ConfigVectorType1, typename TangentVectorType,  
            typename ConfigVectorType2>
  void integrateConfiguration(
      const Eigen::MatrixBase<ConfigVectorType1>& q, 
      const Eigen::MatrixBase<TangentVectorType>& v, 
      const double integration_length, 
      const Eigen::MatrixBase<ConfigVectorType2>& q_integrated) const;

  ///
  /// @brief Computes q_plus - q_minus at the tangent space. 
  /// @param[in] q_plus Configuration. Size must be Robot::dimq().
  /// @param[in] q_minus Configuration. Size must be Robot::dimq().
  /// @param[out] difference Result. Size must be Robot::dimv().
  ///
  template <typename ConfigVectorType1, typename ConfigVectorType2, 
            typename TangentVectorType>
  void subtractConfiguration(
      const Eigen::MatrixBase<ConfigVectorType1>& q_plus, 
      const Eigen::MatrixBase<ConfigVectorType2>& q_minus,
      const Eigen::MatrixBase<TangentVectorType>& difference) const;

  ///
  /// @brief Computes the partial derivative of the function of q_plus - q_minus 
  /// with respect to q_plus at the tangent space. 
  /// @param[in] q_plus Configuration. Size must be Robot::dimq().
  /// @param[in] q_minus Configuration. Size must be Robot::dimq().
  /// @param[out] dSubtract_dqplus The resultant partial derivative. 
  /// Size must be Robot::dimv() x Robot::dimv().
  ///
  template <typename ConfigVectorType1, typename ConfigVectorType2, 
            typename MatrixType>
  void dSubtractdConfigurationPlus(
      const Eigen::MatrixBase<ConfigVectorType1>& q_plus,
      const Eigen::MatrixBase<ConfigVectorType2>& q_minus,
      const Eigen::MatrixBase<MatrixType>& dSubtract_dqplus) const;

  ///
  /// @brief Computes the partial derivative of the function of q_plus - q_minus 
  /// with respect to q_minus at the tangent space. 
  /// @param[in] q_plus Configuration. Size must be Robot::dimq().
  /// @param[in] q_minus Configuration. Size must be Robot::dimq().
  /// @param[out] dSubtract_dqminus The resultant partial derivative. 
  /// Size must be Robot::dimv() x Robot::dimv().
  ///
  template <typename ConfigVectorType1, typename ConfigVectorType2, 
            typename MatrixType>
  void dSubtractdConfigurationMinus(
      const Eigen::MatrixBase<ConfigVectorType1>& q_plus,
      const Eigen::MatrixBase<ConfigVectorType2>& q_minus,
      const Eigen::MatrixBase<MatrixType>& dSubtract_dqminus) const;

  ///
  /// @brief Updates the kinematics of the robot. The frame placements, frame 
  /// velocity, frame acceleration, and the relevant Jacobians are calculated. 
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] v Generalized velocity. Size must be Robot::dimv().
  /// @param[in] a Generalized acceleration. Size must be Robot::dimv().
  ///
  template <typename ConfigVectorType, typename TangentVectorType1, 
            typename TangentVectorType2>
  void updateKinematics(const Eigen::MatrixBase<ConfigVectorType>& q, 
                        const Eigen::MatrixBase<TangentVectorType1>& v, 
                        const Eigen::MatrixBase<TangentVectorType2>& a);

  ///
  /// @brief Updates the kinematics of the robot. The frame placements, frame 
  /// velocity, and the relevant Jacobians are calculated. 
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] v Generalized velocity. Size must be Robot::dimv().
  ///
  template <typename ConfigVectorType, typename TangentVectorType>
  void updateKinematics(const Eigen::MatrixBase<ConfigVectorType>& q, 
                        const Eigen::MatrixBase<TangentVectorType>& v);

  ///
  /// @brief Returns the position of the frame. Before calling this function, 
  /// updateKinematics() must be called.
  /// @param[in] frame_id Index of the frame.
  /// @return Const reference to the position of the frame.
  ///
  const Eigen::Vector3d& framePosition(const int frame_id) const;

  ///
  /// @brief Returns the rotation matrix of the frame. Before calling this  
  /// function, updateKinematics() must be called.
  /// @param[in] frame_id Index of the frame.
  /// @return Const reference to the rotation matrix of the frame.
  ///
  const Eigen::Matrix3d& frameRotation(const int frame_id) const;

  ///
  /// @brief Returns the SE(3) of the frame. Before calling this function, 
  /// updateKinematics() must be called.
  /// @param[in] frame_id Index of the frame.
  /// @return Const reference to the SE(3) of the frame.
  ///
  const pinocchio::SE3& framePlacement(const int frame_id) const;

  ///
  /// @brief Computes the frame Jacobian of the position. Before calling this  
  /// function, updateKinematics() must be called.
  /// @param[in] frame_id Index of the frame.
  /// @param[out] J Jacobian. Size must be 6 x Robot::dimv().
  ///
  template <typename MatrixType>
  void getFrameJacobian(const int frame_id, 
                        const Eigen::MatrixBase<MatrixType>& J);

  ///
  /// @brief Computes the residual of the contact constriants represented by 
  /// Baumgarte's stabilization method. Before calling this function, 
  /// updateKinematics() must be called.
  /// @param[in] contact_status Current contact status.
  /// @param[in] time_step Time step of the Baumgarte's stabilization method. 
  /// Must be positive.
  /// @param[out] baumgarte_residual 3-dimensional vector where the result is 
  /// stored. Size must be at least 3.
  ///
  template <typename VectorType>
  void computeBaumgarteResidual(
      const ContactStatus& contact_status, const double time_step,
      const Eigen::MatrixBase<VectorType>& baumgarte_residual) const;

  ///
  /// @brief Computes the partial derivatives of the contact constriants 
  /// represented by the Baumgarte's stabilization method. 
  /// Before calling this function, updateKinematics() must be called. 
  /// @param[in] contact_status Current contact status.
  /// @param[in] time_step Time step of the Baumgarte's stabilization method. 
  /// Must be positive.
  /// @param[out] baumgarte_partial_dq The result of the partial derivative  
  /// with respect to the configuaration. Rows must be at least 3. Cols must 
  /// be Robot::dimv().
  /// @param[out] baumgarte_partial_dv The result of the partial derivative  
  /// with respect to the velocity. Rows must be at least 3. Cols must 
  /// be Robot::dimv().
  /// @param[out] baumgarte_partial_da The result of the partial derivative  
  /// with respect to the acceleration. Rows must be at least 3. Cols must 
  /// be Robot::dimv().
  ///
  template <typename MatrixType1, typename MatrixType2, typename MatrixType3>
  void computeBaumgarteDerivatives(
      const ContactStatus& contact_status,  const double time_step,
      const Eigen::MatrixBase<MatrixType1>& baumgarte_partial_dq, 
      const Eigen::MatrixBase<MatrixType2>& baumgarte_partial_dv, 
      const Eigen::MatrixBase<MatrixType3>& baumgarte_partial_da);

  ///
  /// @brief Computes the residual of the impulse velocity. Before calling this  
  /// function, updateKinematics() must be called.
  /// @param[in] impulse_status Impulse status.
  /// @param[out] velocity_residual 3-dimensional vector where the result is 
  /// stored. Size must be at least 3.
  ///
  template <typename VectorType>
  void computeImpulseVelocityResidual(
      const ImpulseStatus& impulse_status, 
      const Eigen::MatrixBase<VectorType>& velocity_residual) const;

  ///
  /// @brief Computes the partial derivatives of the impulse velocity.
  /// Before calling this function, updateKinematics() must be called. 
  /// @param[in] impulse_status Impulse status.
  /// @param[out] velocity_partial_dq The result of the partial derivative  
  /// with respect to the configuaration. Rows must be at least 3. Cols must 
  /// be Robot::dimv().
  /// @param[out] velocity_partial_dv The result of the partial derivative  
  /// with respect to the velocity. Rows must be at least 3. Cols must 
  /// be Robot::dimv().
  ///
  template <typename MatrixType1, typename MatrixType2>
  void computeImpulseVelocityDerivatives(
      const ImpulseStatus& impulse_status, 
      const Eigen::MatrixBase<MatrixType1>& velocity_partial_dq, 
      const Eigen::MatrixBase<MatrixType2>& velocity_partial_dv);

  ///
  /// @brief Computes the residual of the implse condition, i.e, contact 
  /// position constriants at the impulse. Before calling this function, 
  /// updateKinematics() must be called.
  /// @param[in] impulse_status Impulse status.
  /// @param[out] contact_residual 3-dimensional vector where the result is 
  /// stored. Size must be at least 3.
  ///
  template <typename VectorType>
  void computeImpulseConditionResidual(
      const ImpulseStatus& impulse_status, 
      const Eigen::MatrixBase<VectorType>& contact_residual) const;

  ///
  /// @brief Computes the residual of the implse condition, i.e, contact 
  /// position constriants at the impulse. Before calling this function, 
  /// updateKinematics() must be called.
  /// @param[in] impulse_status Impulse status.
  /// @param[in] coeff The coefficient that is multiplied to the result.
  /// @param[out] contact_residual 3-dimensional vector where the result is 
  /// stored. Size must be at least 3.
  ///
  template <typename VectorType>
  void computeImpulseConditionResidual(
      const ImpulseStatus& impulse_status, const double coeff,
      const Eigen::MatrixBase<VectorType>& contact_residual) const;

  ///
  /// @brief Computes the partial derivative of the implse condition, i.e, 
  /// contact  position constriants at the impulse. Before calling this  
  /// function, updateKinematics() must be called.
  /// @param[in] impulse_status Impulse status.
  /// @param[out] contact_partial_dq The result of the partial derivative  
  /// with respect to the configuaration. Rows must be at least 3. Cols must 
  /// be Robot::dimv().
  ///
  template <typename MatrixType>
  void computeImpulseConditionDerivative(
      const ImpulseStatus& impulse_status, 
      const Eigen::MatrixBase<MatrixType>& contact_partial_dq);

  ///
  /// @brief Computes the partial derivative of the implse condition, i.e, 
  /// contact  position constriants at the impulse. Before calling this  
  /// function, updateKinematics() must be called.
  /// @param[in] impulse_status Impulse status.
  /// @param[in] coeff The coefficient that is multiplied to the result.
  /// @param[out] contact_partial_dq The result of the partial derivative  
  /// with respect to the configuaration. Rows must be at least 3. Cols must 
  /// be Robot::dimv().
  ///
  template <typename MatrixType>
  void computeImpulseConditionDerivative(
      const ImpulseStatus& impulse_status, const double coeff,
      const Eigen::MatrixBase<MatrixType>& contact_partial_dq);

  ///
  /// @brief Sets the contact points.
  /// @param[in] contact_points Collection of contact points. Size must be 
  /// Robot::max_point_contacts().
  /// 
  void setContactPoints(const std::vector<Eigen::Vector3d>& contact_points);

  ///
  /// @brief Sets all the contact points by using current kinematics.
  /// 
  void setContactPointsByCurrentKinematics();

  ///
  /// @brief Sets the friction coefficient.
  /// @param[in] friction_coefficient Friction coefficient of each contact.  
  /// Size must be Robot::max_point_contacts(). Each element must be positive.
  /// 
  void setFrictionCoefficient(const std::vector<double>& friction_coefficient);

  ///
  /// @brief Returns the friction coefficient of a contact.
  /// @param[in] contact_index Index of the contact. Must be nonnegative.
  /// Must be less than Robot::max_point_contacts().
  /// @return Friction coefficient of the contact of contact_index.
  /// 
  double frictionCoefficient(const int contact_index) const;

  ///
  /// @brief Sets the coefficient of the restitution of each contact.
  /// @param[in] restitution_coefficient Coefficient of the restitution of 
  /// each contact. Size must be Robot::max_point_contacts(). 
  /// Each element must be positive.
  /// 
  void setRestitutionCoefficient(
      const std::vector<double>& restitution_coefficient);

  ///
  /// @brief Returns the coefficient of the restitution of each contact.
  /// @param[in] contact_index Index of the contact. Must be nonnegative.
  /// Must be less than Robot::max_point_contacts().
  /// @return Coefficient of the restitution of the contact of contact_index.
  /// 
  double restitutionCoefficient(const int contact_index) const;

  ///
  /// @brief Set contact forces for each active contacts.  
  /// @param[in] contact_status Current contact status.
  /// @param[in] f The stack of the contact forces represented in the local 
  /// coordinate of the contact frame. Size must be Robot::max_point_contacts(). 
  /// 
  void setContactForces(const ContactStatus& contact_status, 
                        const std::vector<Eigen::Vector3d>& f);

  ///
  /// @brief Set impulse forces for each active impulse. 
  /// @param[in] impulse_status Current contact status.
  /// @param[in] f The stack of the impulse forces represented in the local 
  /// coordinate of the contact frame. Size must be Robot::max_point_contacts(). 
  /// 
  void setImpulseForces(const ImpulseStatus& impulse_status, 
                        const std::vector<Eigen::Vector3d>& f);

  ///
  /// @brief Computes inverse dynamics, i.e., generalized torques corresponding 
  /// to the given configuration, velocity, acceleration, and contact forces. 
  /// If the robot has contacts, update contact forces by calling 
  /// setContactForces().
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] v Generalized velocity. Size must be Robot::dimv().
  /// @param[in] a Generalized acceleration. Size must be Robot::dimv().
  /// @param[out] tau Generalized torques for fully actuated system. Size must 
  /// be Robot::dimv().
  ///
  template <typename ConfigVectorType, typename TangentVectorType1, 
            typename TangentVectorType2, typename TangentVectorType3>
  void RNEA(const Eigen::MatrixBase<ConfigVectorType>& q, 
            const Eigen::MatrixBase<TangentVectorType1>& v, 
            const Eigen::MatrixBase<TangentVectorType2>& a, 
            const Eigen::MatrixBase<TangentVectorType3>& tau);

  ///
  /// @brief Computes Computes the partial dervatives of the function of 
  /// inverse dynamics with respect to the given configuration, velocity, and
  /// acceleration. If the robot has contacts, update contact forces by 
  /// calling setContactForces().
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] v Generalized velocity. Size must be Robot::dimv().
  /// @param[in] a Generalized acceleration. Size must be Robot::dimv().
  /// @param[out] dRNEA_partial_dq The partial derivative of inverse dynamics 
  /// with respect to the configuration. The size must be 
  /// Robot::dimv() x Robot::dimv().
  /// @param[out] dRNEA_partial_dv The partial derivative of inverse dynamics 
  /// with respect to the velocity. The size must be 
  /// Robot::dimv() x Robot::dimv().
  /// @param[out] dRNEA_partial_da The partial derivative of inverse dynamics 
  /// with respect to the acceleration. The size must be 
  /// Robot::dimv() x Robot::dimv().
  ///   
  template <typename ConfigVectorType, typename TangentVectorType1, 
            typename TangentVectorType2, typename MatrixType1, 
            typename MatrixType2, typename MatrixType3>
  void RNEADerivatives(const Eigen::MatrixBase<ConfigVectorType>& q, 
                       const Eigen::MatrixBase<TangentVectorType1>& v, 
                       const Eigen::MatrixBase<TangentVectorType2>& a,
                       const Eigen::MatrixBase<MatrixType1>& dRNEA_partial_dq, 
                       const Eigen::MatrixBase<MatrixType2>& dRNEA_partial_dv, 
                       const Eigen::MatrixBase<MatrixType3>& dRNEA_partial_da);

  ///
  /// @brief Computes the residual of the impulse dynamics for the given 
  /// configuration and the change in the generalized velocity, and contact 
  /// forces by using RNEA. Before call this function,   
  /// update contact forces by calling setContactForces().
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] dv Change in velocity. Size must be Robot::dimv().
  /// @param[out] res Residual of impulse dynamics constraint. Size must be
  /// Robot::dimv(). 
  ///
  template <typename ConfigVectorType, typename TangentVectorType1, 
            typename TangentVectorType2>
  void RNEAImpulse(const Eigen::MatrixBase<ConfigVectorType>& q, 
                   const Eigen::MatrixBase<TangentVectorType1>& dv,
                   const Eigen::MatrixBase<TangentVectorType2>& res);

  ///
  /// @brief Computes the partial dervatives of the impuse dynamics constraint 
  /// with respect to configuration and changes in velocity. Before calling
  /// this function, update contact forces by calling setContactForces().
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] dv Change in velocity. Size must be Robot::dimv().
  /// @param[out] dRNEA_partial_dq The partial derivative of impulse dynamics 
  /// with respect to the configuration. The size must be 
  /// Robot::dimv() x Robot::dimv().
  /// @param[out] dRNEA_partial_ddv The partial derivative of impulse dynamics 
  /// with respect to the change in velocity. The size must be 
  /// Robot::dimv() x Robot::dimv().
  ///   
  template <typename ConfigVectorType, typename TangentVectorType, 
            typename MatrixType1, typename MatrixType2>
  void RNEAImpulseDerivatives(
      const Eigen::MatrixBase<ConfigVectorType>& q, 
      const Eigen::MatrixBase<TangentVectorType>& dv, 
      const Eigen::MatrixBase<MatrixType1>& dRNEA_partial_dq, 
      const Eigen::MatrixBase<MatrixType2>& dRNEA_partial_ddv);

  ///
  /// @brief Computes the partial dervative of the function of inverse dynamics 
  /// with respect to the contact forces. Before calling this function, call 
  /// updateKinematics().
  /// @param[in] contact_status Current contact status.
  /// @param[out] dRNEA_partial_dfext The partial derivative of inverse dynamics 
  /// with respect to the contact forces. Rows must be at least Robot::dimf(). 
  /// Cols must be Robot::dimv().
  ///   
  template <typename MatrixType>
  void dRNEAPartialdFext(
      const ContactStatus& contact_status,
      const Eigen::MatrixBase<MatrixType>& dRNEA_partial_dfext);

  ///
  /// @brief Computes the inverse of the joint inertia matrix M.
  /// @param[in] M Joint inertia matrix. Size must be 
  /// Robot::dimv() x Robot::dimv().
  /// @param[out] Minv Inverse of the joint inertia matrix M. Size must be 
  /// Robot::dimv() x Robot::dimv().
  ///   
  template <typename MatrixType1, typename MatrixType2>
  void computeMinv(const Eigen::MatrixBase<MatrixType1>& M, 
                   const Eigen::MatrixBase<MatrixType2>& Minv);

  ///
  /// @brief Computes the inverse of the joint inertia matrix M.
  /// @param[in] M Joint inertia matrix. Size must be 
  /// Robot::dimv() x Robot::dimv().
  /// @param[in] J Contact Jacobian. Size must be 
  /// ContactStatus::dimf() x Robot::dimv().
  /// @param[out] MJtJinv Inverse of the matrix [[M J^T], [J O]]. Size must be 
  /// (Robot::dimv() + ContactStatus::dimf()) x 
  /// (Robot::dimv() + ContactStatus::dimf()).
  ///   
  template <typename MatrixType1, typename MatrixType2, typename MatrixType3>
  void computeMJtJinv(const Eigen::MatrixBase<MatrixType1>& M, 
                      const Eigen::MatrixBase<MatrixType2>& J,
                      const Eigen::MatrixBase<MatrixType3>& MJtJinv);

  ///
  /// @brief Computes the state equation, i.e., the velocity and forward 
  /// dynamics.
  /// @param[in] q Configuration. Size must be Robot::dimq().
  /// @param[in] v Generalized velocity. Size must be Robot::dimv().
  /// @param[in] tau Generalized acceleration. Size must be Robot::dimv().
  /// @param[out] dq The resultant generalized velocity. Size must be 
  /// Robot::dimv().
  /// @param[out] dv The resultant generalized acceleration. Size must be 
  /// Robot::dimv().
  ///
  template <typename ConfigVectorType, typename TangentVectorType1, 
            typename TangentVectorType2, typename TangentVectorType3,
            typename TangentVectorType4>
  void stateEquation(const Eigen::MatrixBase<ConfigVectorType>& q, 
                     const Eigen::MatrixBase<TangentVectorType1>& v, 
                     const Eigen::MatrixBase<TangentVectorType2>& tau, 
                     const Eigen::MatrixBase<TangentVectorType3>& dq,
                     const Eigen::MatrixBase<TangentVectorType4>& dv);

  ///
  /// @brief Generates feasible configuration randomly.
  /// @param[out] q The random configuration. Size must be Robot::dimq().
  ///
  template <typename ConfigVectorType>
  void generateFeasibleConfiguration(
      const Eigen::MatrixBase<ConfigVectorType>& q) const;

  ///
  /// @brief Normalizes a configuration vector.
  /// @param[in, out] q The normalized configuration. Size must be Robot::dimq().
  ///
  template <typename ConfigVectorType>
  void normalizeConfiguration(
      const Eigen::MatrixBase<ConfigVectorType>& q) const;

  ///
  /// @brief Returns the effort limit of each joints.
  /// @return The effort limit of each joints.
  /// 
  Eigen::VectorXd jointEffortLimit() const;

  ///
  /// @brief Returns the joint velocity limit of each joints.
  /// @return The joint velocity limit of each joints.
  /// 
  Eigen::VectorXd jointVelocityLimit() const;

  ///
  /// @brief Returns the lower limit of the position of each joints.
  /// @return The lower limit of the position of each joints.
  /// 
  Eigen::VectorXd lowerJointPositionLimit() const;

  ///
  /// @brief Returns the upper limit of the position of each joints.
  /// @return The upper limit of the position of each joints.
  ///
  Eigen::VectorXd upperJointPositionLimit() const;

  ///
  /// @brief Sets the effort limit of each joints.
  /// @param[in] joint_effort_limit The effort limit of each joints.
  /// 
  void setJointEffortLimit(const Eigen::VectorXd& joint_effort_limit);

  ///
  /// @brief Sets the joint velocity limit of each joints.
  /// @param[in] joint_velocity_limit Sets the joint velocity limit of each 
  /// joints.
  ///
  void setJointVelocityLimit(const Eigen::VectorXd& joint_velocity_limit);

  ///
  /// @brief Sets the lower limit of the position of each joints.
  /// @param[in] lower_joint_position_limit The lower limit of the position of 
  /// each joints.
  /// 
  void setLowerJointPositionLimit(
      const Eigen::VectorXd& lower_joint_position_limit);

  ///
  /// @brief Sets the upper limit of the position of each joints.
  /// @param[in] upper_joint_position_limit The upper limit of the position of 
  /// each joints.
  ///
  void setUpperJointPositionLimit(
      const Eigen::VectorXd& upper_joint_position_limit);

  ///
  /// @brief Returns the dimensiton of the configuration.
  /// @return The dimensiton of the configuration.
  /// 
  int dimq() const;

  ///
  /// @brief Returns the dimensiton of the velocity, i.e, tangent space.
  /// @return The dimensiton of the velocity, i.e, the dimension of the tangent 
  /// space of the configuration space.
  /// 
  int dimv() const;

  ///
  /// @brief Returns the dimensiton of the velocity, i.e, tangent space.
  /// @return The dimensiton of the velocity, i.e, the dimension of the tangent 
  /// space of the configuration space.
  /// 
  int dimu() const;

  ///
  /// @brief Returns the maximum dimension of the contacts.
  /// @return The maximum dimension of the contacts.
  /// 
  int max_dimf() const;

  ///
  /// @brief Returns the dimensiton of the generalized torques corresponding to 
  /// the passive joints.
  /// @return The dimensiton of the generalized torques corresponding to the 
  //// passive joints.
  /// 
  int dim_passive() const;

  ///
  /// @brief Returns true if the robot has a floating base and false if not.
  /// @returns true if the robot has a floating base and false if not.
  /// 
  bool has_floating_base() const;

  ///
  /// @brief Returns the maximum number of the contacts.
  /// @return The maximum number of the contacts.
  /// 
  int max_point_contacts() const;

  ///
  /// @brief Prints the robot model into console.
  ///
  void printRobotModel() const;

  ///
  /// @brief Initializes joint_effort_limit_, joint_velocity_limit_, 
  /// lower_joint_position_limit_, upper_joint_position_limit_, by the URDF.
  /// 
  void initializeJointLimits();

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  pinocchio::Model model_, impulse_model_;
  pinocchio::Data data_, impulse_data_;
  FloatingBase floating_base_;
  std::vector<PointContact> point_contacts_;
  pinocchio::container::aligned_vector<pinocchio::Force> fjoint_;
  int dimq_, dimv_, dimu_, max_dimf_, dimf_, num_active_contacts_;
  bool has_active_contacts_;
  std::vector<bool> is_each_contact_active_;
  Eigen::MatrixXd dimpulse_dv_; /// @brief used in RNEAImpulseDerivatives
  Eigen::VectorXd joint_effort_limit_, joint_velocity_limit_,
                  lower_joint_position_limit_, upper_joint_position_limit_;
};

} // namespace idocp

#include "idocp/robot/robot.hxx"

#endif // IDOCP_ROBOT_HPP_ 