#ifndef IDOCP_ROBOT_HXX_
#define IDOCP_ROBOT_HXX_

#include "idocp/robot/robot.hpp"

#include "pinocchio/parsers/urdf.hpp"
#include "pinocchio/algorithm/joint-configuration.hpp"
#include "pinocchio/algorithm/kinematics-derivatives.hpp"
#include "pinocchio/algorithm/frames.hpp"
#include "pinocchio/algorithm/frames-derivatives.hpp"
#include "pinocchio/algorithm/crba.hpp"
#include "pinocchio/algorithm/rnea.hpp"
#include "pinocchio/algorithm/rnea-derivatives.hpp"
#include "pinocchio/algorithm/aba.hpp"
#include "pinocchio/algorithm/cholesky.hpp"
#include "pinocchio/algorithm/contact-dynamics.hpp"

#include <stdexcept>
#include <cassert>

namespace idocp {

template <typename TangentVectorType, typename ConfigVectorType>
inline void Robot::integrateConfiguration(
    const Eigen::MatrixBase<TangentVectorType>& v, 
    const double integration_length, 
    const Eigen::MatrixBase<ConfigVectorType>& q) const {
  assert(v.size() == dimv_);
  assert(q.size() == dimq_);
  if (has_floating_base_) {
    const Eigen::VectorXd q_tmp = q;
    pinocchio::integrate(model_, q_tmp, integration_length*v, 
                         const_cast<Eigen::MatrixBase<ConfigVectorType>&>(q));
  }
  else {
    (const_cast<Eigen::MatrixBase<ConfigVectorType>&>(q)).noalias() 
        += integration_length * v;
  }
}


template <typename ConfigVectorType1, typename TangentVectorType,  
          typename ConfigVectorType2>
inline void Robot::integrateConfiguration(
    const Eigen::MatrixBase<ConfigVectorType1>& q, 
    const Eigen::MatrixBase<TangentVectorType>& v, 
    const double integration_length, 
    const Eigen::MatrixBase<ConfigVectorType2>& q_integrated) const {
  assert(q.size() == dimq_);
  assert(v.size() == dimv_);
  assert(q_integrated.size() == dimq_);
  pinocchio::integrate(
      model_, q, integration_length*v, 
      const_cast<Eigen::MatrixBase<ConfigVectorType2>&>(q_integrated));
}


template <typename ConfigVectorType, typename TangentVectorType, 
          typename MatrixType1, typename MatrixType2>
inline void Robot::dIntegrateTransport_dq(
    const Eigen::MatrixBase<ConfigVectorType>& q,
    const Eigen::MatrixBase<TangentVectorType>& v,
    const Eigen::MatrixBase<MatrixType1>& Jin,
    const Eigen::MatrixBase<MatrixType2>& Jout) const {
  assert(q.size() == dimq_);
  assert(v.size() == dimv_);
  assert(Jin.cols() == dimv_);
  assert(Jout.rows() == Jin.rows());
  assert(Jout.cols() == Jin.cols());
  pinocchio::dIntegrateTransport(
      model_, q, v, Jin.transpose(), 
      const_cast<Eigen::MatrixBase<MatrixType2>&>(Jout).transpose(),
      pinocchio::ARG0);
}


template <typename ConfigVectorType, typename TangentVectorType, 
          typename MatrixType1, typename MatrixType2>
inline void Robot::dIntegrateTransport_dv(
    const Eigen::MatrixBase<ConfigVectorType>& q,
    const Eigen::MatrixBase<TangentVectorType>& v,
    const Eigen::MatrixBase<MatrixType1>& Jin,
    const Eigen::MatrixBase<MatrixType2>& Jout) const {
  assert(q.size() == dimq_);
  assert(v.size() == dimv_);
  assert(Jin.cols() == dimv_);
  assert(Jout.rows() == Jin.rows());
  assert(Jout.cols() == Jin.cols());
  pinocchio::dIntegrateTransport(
      model_, q, v, Jin.transpose(), 
      const_cast<Eigen::MatrixBase<MatrixType2>&>(Jout).transpose(),
      pinocchio::ARG1);
}


template <typename ConfigVectorType1, typename ConfigVectorType2, 
          typename TangentVectorType>
inline void Robot::subtractConfiguration(
    const Eigen::MatrixBase<ConfigVectorType1>& qf, 
    const Eigen::MatrixBase<ConfigVectorType2>& q0,
    const Eigen::MatrixBase<TangentVectorType>& qdiff) const {
  assert(qf.size() == dimq_);
  assert(q0.size() == dimq_);
  assert(qdiff.size() == dimv_);
  pinocchio::difference(
      model_, q0, qf, 
      const_cast<Eigen::MatrixBase<TangentVectorType>&>(qdiff));
}


template <typename ConfigVectorType1, typename ConfigVectorType2, 
          typename MatrixType>
inline void Robot::dSubtractConfiguration_dqf(
    const Eigen::MatrixBase<ConfigVectorType1>& qf,
    const Eigen::MatrixBase<ConfigVectorType2>& q0,
    const Eigen::MatrixBase<MatrixType>& dqdiff_dqf) const {
  assert(qf.size() == dimq_);
  assert(q0.size() == dimq_);
  assert(dqdiff_dqf.rows() == dimv_);
  assert(dqdiff_dqf.cols() == dimv_);
  pinocchio::dDifference(model_, q0, qf, 
                         const_cast<Eigen::MatrixBase<MatrixType>&>(dqdiff_dqf),
                         pinocchio::ARG1);
}


template <typename ConfigVectorType1, typename ConfigVectorType2, 
          typename MatrixType>
inline void Robot::dSubtractConfiguration_dq0(
    const Eigen::MatrixBase<ConfigVectorType1>& qf,
    const Eigen::MatrixBase<ConfigVectorType2>& q0,
    const Eigen::MatrixBase<MatrixType>& dqdiff_dq0) const {
  assert(qf.size() == dimq_);
  assert(q0.size() == dimq_);
  assert(dqdiff_dq0.rows() == dimv_);
  assert(dqdiff_dq0.cols() == dimv_);
  pinocchio::dDifference(model_, q0, qf, 
                         const_cast<Eigen::MatrixBase<MatrixType>&>(dqdiff_dq0),
                         pinocchio::ARG0);
}


template <typename ConfigVectorType, typename TangentVectorType1, 
          typename TangentVectorType2>
inline void Robot::updateKinematics(
    const Eigen::MatrixBase<ConfigVectorType>& q, 
    const Eigen::MatrixBase<TangentVectorType1>& v, 
    const Eigen::MatrixBase<TangentVectorType2>& a) {
  assert(q.size() == dimq_);
  assert(v.size() == dimv_);
  assert(a.size() == dimv_);
  pinocchio::forwardKinematics(model_, data_, q, v, a);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q, v, a);
  pinocchio::jacobianCenterOfMass(model_, data_, false);
}


template <typename ConfigVectorType, typename TangentVectorType>
inline void Robot::updateKinematics(
    const Eigen::MatrixBase<ConfigVectorType>& q, 
    const Eigen::MatrixBase<TangentVectorType>& v) {
  assert(q.size() == dimq_);
  assert(v.size() == dimv_);
  pinocchio::forwardKinematics(model_, data_, q, v);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q, v, 
                                                 Eigen::VectorXd::Zero(dimv_));
  pinocchio::jacobianCenterOfMass(model_, data_, false);
}


template <typename ConfigVectorType>
inline void Robot::updateKinematics(
    const Eigen::MatrixBase<ConfigVectorType>& q) {
  assert(q.size() == dimq_);
  pinocchio::framesForwardKinematics(model_, data_, q);
  pinocchio::computeJointJacobians(model_, data_, q);
  pinocchio::jacobianCenterOfMass(model_, data_, false);
}


template <typename ConfigVectorType>
inline void Robot::updateFrameKinematics(
    const Eigen::MatrixBase<ConfigVectorType>& q) {
  assert(q.size() == dimq_);
  pinocchio::framesForwardKinematics(model_, data_, q);
  pinocchio::centerOfMass(model_, data_, q, false);
}


inline const Eigen::Vector3d& Robot::framePosition(const int frame_id) const {
  return data_.oMf[frame_id].translation();
}


inline const Eigen::Matrix3d& Robot::frameRotation(const int frame_id) const {
  return data_.oMf[frame_id].rotation();
}


inline const SE3& Robot::framePlacement(const int frame_id) const {
  return data_.oMf[frame_id];
}


inline const Eigen::Vector3d& Robot::CoM() const {
  return data_.com[0];
}


template <typename MatrixType>
inline void Robot::getFrameJacobian(const int frame_id, 
                                    const Eigen::MatrixBase<MatrixType>& J) {
  assert(J.rows() == 6);
  assert(J.cols() == dimv_);
  pinocchio::getFrameJacobian(model_, data_, frame_id, pinocchio::LOCAL, 
                              const_cast<Eigen::MatrixBase<MatrixType>&>(J));
}


template <typename MatrixType>
inline void Robot::getCoMJacobian(const Eigen::MatrixBase<MatrixType>& J) const {
  assert(J.rows() == 3);
  assert(J.cols() == dimv_);
  const_cast<Eigen::MatrixBase<MatrixType>&>(J) = data_.Jcom;
}


template <typename VectorType>
inline void Robot::computeBaumgarteResidual(
    const ContactStatus& contact_status, 
    const std::vector<Eigen::Vector3d>& contact_points,
    const Eigen::MatrixBase<VectorType>& baumgarte_residual) const {
  assert(contact_points.size() == maxPointContacts());
  assert(baumgarte_residual.size() == contact_status.dimf());
  int num_active_contacts = 0;
  for (int i=0; i<point_contacts_.size(); ++i) {
    if (contact_status.isContactActive(i)) {
      point_contacts_[i].computeBaumgarteResidual(
          model_, data_, contact_points[i],
          (const_cast<Eigen::MatrixBase<VectorType>&>(baumgarte_residual))
              .template segment<3>(3*num_active_contacts));
      ++num_active_contacts;
    }
  }
}


template <typename MatrixType1, typename MatrixType2, typename MatrixType3>
inline void Robot::computeBaumgarteDerivatives(
    const ContactStatus& contact_status,
    const Eigen::MatrixBase<MatrixType1>& baumgarte_partial_dq, 
    const Eigen::MatrixBase<MatrixType2>& baumgarte_partial_dv, 
    const Eigen::MatrixBase<MatrixType3>& baumgarte_partial_da) {
  assert(baumgarte_partial_dq.rows() == contact_status.dimf());
  assert(baumgarte_partial_dq.cols() == dimv_);
  assert(baumgarte_partial_dv.rows() == contact_status.dimf());
  assert(baumgarte_partial_dv.cols() == dimv_);
  assert(baumgarte_partial_da.rows() == contact_status.dimf());
  assert(baumgarte_partial_da.cols() == dimv_);
  int num_active_contacts = 0;
  for (int i=0; i<point_contacts_.size(); ++i) {
    if (contact_status.isContactActive(i)) {
      point_contacts_[i].computeBaumgarteDerivatives(
          model_, data_, 
          (const_cast<Eigen::MatrixBase<MatrixType1>&>(baumgarte_partial_dq))
              .block(3*num_active_contacts, 0, 3, dimv_),
          (const_cast<Eigen::MatrixBase<MatrixType2>&>(baumgarte_partial_dv))
              .block(3*num_active_contacts, 0, 3, dimv_),
          (const_cast<Eigen::MatrixBase<MatrixType3>&>(baumgarte_partial_da))
              .block(3*num_active_contacts, 0, 3, dimv_));
      ++num_active_contacts;
    }
  }
}


template <typename VectorType>
inline void Robot::computeImpulseVelocityResidual(
    const ImpulseStatus& impulse_status, 
    const Eigen::MatrixBase<VectorType>& velocity_residual) const {
  assert(velocity_residual.size() == impulse_status.dimf());
  int num_active_impulse = 0;
  for (int i=0; i<point_contacts_.size(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      point_contacts_[i].computeContactVelocityResidual(
          model_, data_,
          (const_cast<Eigen::MatrixBase<VectorType>&>(velocity_residual))
              .template segment<3>(3*num_active_impulse));
      ++num_active_impulse;
    }
  }
}


template <typename MatrixType1, typename MatrixType2>
inline void Robot::computeImpulseVelocityDerivatives(
    const ImpulseStatus& impulse_status, 
    const Eigen::MatrixBase<MatrixType1>& velocity_partial_dq, 
    const Eigen::MatrixBase<MatrixType2>& velocity_partial_dv) {
  assert(velocity_partial_dq.rows() == impulse_status.dimf());
  assert(velocity_partial_dq.cols() == dimv_);
  assert(velocity_partial_dv.rows() == impulse_status.dimf());
  assert(velocity_partial_dv.cols() == dimv_);
  int num_active_impulse = 0;
  for (int i=0; i<point_contacts_.size(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      point_contacts_[i].computeContactVelocityDerivatives(
          model_, data_, 
          (const_cast<Eigen::MatrixBase<MatrixType1>&>(velocity_partial_dq))
              .block(3*num_active_impulse, 0, 3, dimv_),
          (const_cast<Eigen::MatrixBase<MatrixType2>&>(velocity_partial_dv))
              .block(3*num_active_impulse, 0, 3, dimv_));
      ++num_active_impulse;
    }
  }
}


template <typename VectorType>
inline void Robot::computeContactPositionResidual(
    const ImpulseStatus& impulse_status, 
    const std::vector<Eigen::Vector3d>& contact_points,
    const Eigen::MatrixBase<VectorType>& contact_residual) const {
  assert(contact_points.size() == point_contacts_.size());
  assert(contact_residual.size() == impulse_status.dimf());
  int num_active_impulse = 0;
  for (int i=0; i<point_contacts_.size(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      point_contacts_[i].computeContactPositionResidual(
          model_, data_, contact_points[i],
          (const_cast<Eigen::MatrixBase<VectorType>&>(contact_residual))
              .template segment<3>(3*num_active_impulse));
      ++num_active_impulse;
    }
  }
}


template <typename MatrixType>
inline void Robot::computeContactPositionDerivative(
    const ImpulseStatus& impulse_status, 
    const Eigen::MatrixBase<MatrixType>& contact_partial_dq) {
  assert(contact_partial_dq.rows() == impulse_status.dimf());
  assert(contact_partial_dq.cols() == dimv_);
  int num_active_impulse = 0;
  for (int i=0; i<point_contacts_.size(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      point_contacts_[i].computeContactPositionDerivative(
          model_, data_, 
          (const_cast<Eigen::MatrixBase<MatrixType>&>(contact_partial_dq))
              .block(3*num_active_impulse, 0, 3, dimv_));
      ++num_active_impulse;
    }
  }
}


inline void Robot::setContactForces(const ContactStatus& contact_status, 
                                    const std::vector<Eigen::Vector3d>& f) {
  assert(f.size() == maxPointContacts());
  for (int i=0; i<point_contacts_.size(); ++i) {
    if (contact_status.isContactActive(i)) {
      point_contacts_[i].computeJointForceFromContactForce(f[i], fjoint_);
    }
    else {
      point_contacts_[i].computeJointForceFromContactForce(
          Eigen::Vector3d::Zero(), fjoint_);
    }
  }
}


inline void Robot::setImpulseForces(const ImpulseStatus& impulse_status, 
                                    const std::vector<Eigen::Vector3d>& f) {
  assert(f.size() == maxPointContacts());
  for (int i=0; i<point_contacts_.size(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      point_contacts_[i].computeJointForceFromContactForce(f[i], fjoint_);
    }
    else {
      point_contacts_[i].computeJointForceFromContactForce(
          Eigen::Vector3d::Zero(), fjoint_);
    }
  }
}


template <typename ConfigVectorType, typename TangentVectorType1, 
          typename TangentVectorType2, typename TangentVectorType3>
inline void Robot::RNEA(const Eigen::MatrixBase<ConfigVectorType>& q, 
                        const Eigen::MatrixBase<TangentVectorType1>& v, 
                        const Eigen::MatrixBase<TangentVectorType2>& a, 
                        const Eigen::MatrixBase<TangentVectorType3>& tau) {
  assert(q.size() == dimq_);
  assert(v.size() == dimv_);
  assert(a.size() == dimv_);
  assert(tau.size() == dimv_);
  if (point_contacts_.empty()) {
    const_cast<Eigen::MatrixBase<TangentVectorType3>&>(tau)
        = pinocchio::rnea(model_, data_, q, v, a);
  }
  else {
    const_cast<Eigen::MatrixBase<TangentVectorType3>&>(tau)
        = pinocchio::rnea(model_, data_, q, v, a, fjoint_);
  }
}


template <typename ConfigVectorType, typename TangentVectorType1, 
          typename TangentVectorType2, typename MatrixType1, 
          typename MatrixType2, typename MatrixType3>
inline void Robot::RNEADerivatives(
    const Eigen::MatrixBase<ConfigVectorType>& q, 
    const Eigen::MatrixBase<TangentVectorType1>& v, 
    const Eigen::MatrixBase<TangentVectorType2>& a, 
    const Eigen::MatrixBase<MatrixType1>& dRNEA_partial_dq, 
    const Eigen::MatrixBase<MatrixType2>& dRNEA_partial_dv, 
    const Eigen::MatrixBase<MatrixType3>& dRNEA_partial_da) {
  assert(q.size() == dimq_);
  assert(v.size() == dimv_);
  assert(a.size() == dimv_);
  assert(dRNEA_partial_dq.cols() == dimv_);
  assert(dRNEA_partial_dq.rows() == dimv_);
  assert(dRNEA_partial_dv.cols() == dimv_);
  assert(dRNEA_partial_dv.rows() == dimv_);
  assert(dRNEA_partial_da.cols() == dimv_);
  assert(dRNEA_partial_da.rows() == dimv_);
  if (point_contacts_.empty()) {
      pinocchio::computeRNEADerivatives(
          model_, data_, q, v, a, 
          const_cast<Eigen::MatrixBase<MatrixType1>&>(dRNEA_partial_dq),
          const_cast<Eigen::MatrixBase<MatrixType2>&>(dRNEA_partial_dv),
          const_cast<Eigen::MatrixBase<MatrixType3>&>(dRNEA_partial_da));
  }
  else {
      pinocchio::computeRNEADerivatives(
          model_, data_, q, v, a, fjoint_,
          const_cast<Eigen::MatrixBase<MatrixType1>&>(dRNEA_partial_dq),
          const_cast<Eigen::MatrixBase<MatrixType2>&>(dRNEA_partial_dv),
          const_cast<Eigen::MatrixBase<MatrixType3>&>(dRNEA_partial_da));
  }
  (const_cast<Eigen::MatrixBase<MatrixType3>&>(dRNEA_partial_da)) 
      .template triangularView<Eigen::StrictlyLower>() 
      = (const_cast<Eigen::MatrixBase<MatrixType3>&>(dRNEA_partial_da)).transpose()
          .template triangularView<Eigen::StrictlyLower>();
}


template <typename ConfigVectorType, typename TangentVectorType1, 
          typename TangentVectorType2>
inline void Robot::RNEAImpulse(
    const Eigen::MatrixBase<ConfigVectorType>& q, 
    const Eigen::MatrixBase<TangentVectorType1>& dv,
    const Eigen::MatrixBase<TangentVectorType2>& res) {
  assert(q.size() == dimq_);
  assert(dv.size() == dimv_);
  assert(res.size() == dimv_);
  const_cast<Eigen::MatrixBase<TangentVectorType2>&>(res)
      = pinocchio::rnea(impulse_model_, impulse_data_, q, 
                        Eigen::VectorXd::Zero(dimv_),  dv, fjoint_);
}


template <typename ConfigVectorType, typename TangentVectorType, 
          typename MatrixType1, typename MatrixType2>
inline void Robot::RNEAImpulseDerivatives(
    const Eigen::MatrixBase<ConfigVectorType>& q, 
    const Eigen::MatrixBase<TangentVectorType>& dv, 
    const Eigen::MatrixBase<MatrixType1>& dRNEA_partial_dq, 
    const Eigen::MatrixBase<MatrixType2>& dRNEA_partial_ddv) {
  assert(q.size() == dimq_);
  assert(dv.size() == dimv_);
  assert(dRNEA_partial_dq.cols() == dimv_);
  assert(dRNEA_partial_dq.rows() == dimv_);
  assert(dRNEA_partial_ddv.cols() == dimv_);
  assert(dRNEA_partial_ddv.rows() == dimv_);
  pinocchio::computeRNEADerivatives(
      impulse_model_, impulse_data_, q, Eigen::VectorXd::Zero(dimv_), dv, 
      fjoint_, const_cast<Eigen::MatrixBase<MatrixType1>&>(dRNEA_partial_dq),
      dimpulse_dv_,
      const_cast<Eigen::MatrixBase<MatrixType2>&>(dRNEA_partial_ddv));
  (const_cast<Eigen::MatrixBase<MatrixType2>&>(dRNEA_partial_ddv)) 
      .template triangularView<Eigen::StrictlyLower>() 
      = (const_cast<Eigen::MatrixBase<MatrixType2>&>(dRNEA_partial_ddv)).transpose()
          .template triangularView<Eigen::StrictlyLower>();
}


template <typename MatrixType1, typename MatrixType2>
inline void Robot::computeMinv(const Eigen::MatrixBase<MatrixType1>& M, 
                               const Eigen::MatrixBase<MatrixType2>& Minv) {
  assert(M.rows() == dimv_);
  assert(M.cols() == dimv_);
  assert(Minv.rows() == dimv_);
  assert(Minv.cols() == dimv_);
  data_.M = M;
  pinocchio::cholesky::decompose(model_, data_);
  pinocchio::cholesky::computeMinv(
      model_, data_, const_cast<Eigen::MatrixBase<MatrixType2>&>(Minv));
}


template <typename MatrixType1, typename MatrixType2, typename MatrixType3>
inline void Robot::computeMJtJinv(
    const Eigen::MatrixBase<MatrixType1>& M, 
    const Eigen::MatrixBase<MatrixType2>& J, 
    const Eigen::MatrixBase<MatrixType3>& MJtJinv) {
  assert(M.rows() == dimv_);
  assert(M.cols() == dimv_);
  assert(J.rows() <= max_dimf_);
  assert(J.cols() == dimv_);
  assert(MJtJinv.rows() == M.rows()+J.rows());
  assert(MJtJinv.cols() == M.rows()+J.rows());
  const int dimf = J.rows();
  data_.M = M;
  pinocchio::cholesky::decompose(model_, data_);
  data_.sDUiJt.leftCols(dimf) = J.transpose();
  pinocchio::cholesky::Uiv(model_, data_, data_.sDUiJt.leftCols(dimf));
  for (Eigen::DenseIndex k=0; k<dimv_; ++k) {
    data_.sDUiJt.leftCols(dimf).row(k) /= std::sqrt(data_.D[k]);
  }
  data_.JMinvJt.topLeftCorner(dimf, dimf).noalias() 
      = data_.sDUiJt.leftCols(dimf).transpose() * data_.sDUiJt.leftCols(dimf);
  data_.llt_JMinvJt.compute(data_.JMinvJt.topLeftCorner(dimf, dimf));
  assert(data_.llt_JMinvJt.info() == Eigen::Success);
  Eigen::Block<MatrixType3> topLeft 
      = const_cast<Eigen::MatrixBase<MatrixType3>&>(MJtJinv).topLeftCorner(dimv_, dimv_);
  Eigen::Block<MatrixType3> topRight 
      = const_cast<Eigen::MatrixBase<MatrixType3>&>(MJtJinv).topRightCorner(dimv_, dimf);
  Eigen::Block<MatrixType3> bottomLeft 
      = const_cast<Eigen::MatrixBase<MatrixType3>&>(MJtJinv).bottomLeftCorner(dimf, dimv_);
  Eigen::Block<MatrixType3> bottomRight 
      = const_cast<Eigen::MatrixBase<MatrixType3>&>(MJtJinv).bottomRightCorner(dimf, dimf);
  bottomRight = - pinocchio::Data::MatrixXs::Identity(dimf, dimf);
  topLeft.setIdentity();
  data_.llt_JMinvJt.solveInPlace(bottomRight);
  pinocchio::cholesky::solve(model_, data_, topLeft);
  bottomLeft.noalias() = J * topLeft;
  topRight.noalias() = bottomLeft.transpose() * (-bottomRight);
  topLeft.noalias() -= topRight*bottomLeft;
  bottomLeft = topRight.transpose();
  assert(!MJtJinv.hasNaN());
}


inline Eigen::VectorXd Robot::generateFeasibleConfiguration() const {
  Eigen::VectorXd q_min(dimq_), q_max(dimq_);
  if (has_floating_base_) {
    q_min.head(7) = - Eigen::VectorXd::Ones(7);
    q_max.head(7) = Eigen::VectorXd::Ones(7);
  }
  q_min.tail(dimu_) = lower_joint_position_limit_;
  q_max.tail(dimu_) = upper_joint_position_limit_;
  return pinocchio::randomConfiguration(model_, q_min, q_max);
}


template <typename ConfigVectorType>
inline void Robot::normalizeConfiguration(
    const Eigen::MatrixBase<ConfigVectorType>& q) const {
  assert(q.size() == dimq_);
  if (has_floating_base_) {
    if (q.template segment<4>(3).squaredNorm() 
          <= std::numeric_limits<double>::epsilon()) {
      (const_cast<Eigen::MatrixBase<ConfigVectorType>&> (q)).coeffRef(3) = 1;
    }
    pinocchio::normalize(model_, 
                         const_cast<Eigen::MatrixBase<ConfigVectorType>&>(q));
  }
}


inline Eigen::VectorXd Robot::jointEffortLimit() const {
  return joint_effort_limit_;
}


inline Eigen::VectorXd Robot::jointVelocityLimit() const {
  return joint_velocity_limit_;
}


inline Eigen::VectorXd Robot::lowerJointPositionLimit() const {
  return lower_joint_position_limit_;
}


inline Eigen::VectorXd Robot::upperJointPositionLimit() const {
  return upper_joint_position_limit_;
}


inline double Robot::totalWeight() const {
  return (- pinocchio::computeTotalMass(model_) * model_.gravity981.coeff(2));
}


inline int Robot::dimq() const {
  return dimq_;
}


inline int Robot::dimv() const {
  return dimv_;
}


inline int Robot::dimu() const {
  return dimu_;
}


inline int Robot::max_dimf() const {
  return max_dimf_;
}


inline int Robot::dim_passive() const {
  return dim_passive_;
}


inline bool Robot::hasFloatingBase() const {
  return has_floating_base_;
}


inline int Robot::maxPointContacts() const {
  return point_contacts_.size();
}


inline std::vector<int> Robot::contactFrames() const {
  std::vector<int> contact_frames_indices;
  for (const auto& e : point_contacts_) {
    contact_frames_indices.push_back(e.contact_frame_id());
  }
  return contact_frames_indices;
}


inline ContactStatus Robot::createContactStatus() const {
  return ContactStatus(maxPointContacts());
}


inline ImpulseStatus Robot::createImpulseStatus() const {
  return ImpulseStatus(maxPointContacts());
}


template <typename ContactStatusType>
inline void Robot::getContactPoints(ContactStatusType& contact_status) const {
  assert(contact_status.maxPointContacts() == maxPointContacts());
  for (int i=0; i<point_contacts_.size(); ++i) {
    contact_status.setContactPoint(i, point_contacts_[i].contactPoint(data_));
  }
}


inline void Robot::getContactPoints(
    std::vector<Eigen::Vector3d>& contact_points) const {
  assert(contact_points.size() == maxPointContacts());
  for (int i=0; i<point_contacts_.size(); ++i) {
    contact_points[i] = point_contacts_[i].contactPoint(data_);
  }
}

} // namespace idocp

#endif // IDOCP_ROBOT_HXX_ 