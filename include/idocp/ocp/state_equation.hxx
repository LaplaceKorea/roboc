#ifndef IDOCP_STATE_EQUATION_HXX_
#define IDOCP_STATE_EQUATION_HXX_

#include "idocp/ocp/state_equation.hpp"

#include <assert.h>

namespace idocp {
namespace stateequation {

template <typename ConfigVectorType>
inline void LinearizeForwardEuler(
    const Robot& robot, const double dtau, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, const SplitSolution& s, 
    const SplitSolution& s_next, KKTMatrix& kkt_matrix, 
    KKTResidual& kkt_residual) {
  assert(dtau > 0);
  ComputeForwardEulerResidual(robot, dtau, s, s_next.q, s_next.v, kkt_residual);
  if (robot.has_floating_base()) {
    robot.dSubtractdConfigurationPlus(s.q, s_next.q, kkt_matrix.Fqq());
    robot.dSubtractdConfigurationMinus(q_prev, s.q, kkt_matrix.Fqq_prev);
    kkt_residual.lq().noalias() += kkt_matrix.Fqq().transpose() * s_next.lmd 
                                    + kkt_matrix.Fqq_prev.transpose() * s.lmd;
  }
  else {
    kkt_residual.lq().noalias() += s_next.lmd - s.lmd;
  }
  kkt_residual.lv().noalias() += dtau * s_next.lmd + s_next.gmm - s.gmm;
  kkt_residual.la.noalias() += dtau * s_next.gmm;
}


template <typename ConfigVectorType, typename TangentVectorType>
inline void LinearizeBackwardEuler(
    const Robot& robot, const double dtau, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, 
    const Eigen::MatrixBase<TangentVectorType>& v_prev, 
    const SplitSolution& s, const SplitSolution& s_next,
    KKTMatrix& kkt_matrix, KKTResidual& kkt_residual) {
  assert(dtau > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  ComputeBackwardEulerResidual(robot, dtau, q_prev, v_prev, s, kkt_residual);
  if (robot.has_floating_base()) {
    robot.dSubtractdConfigurationMinus(q_prev, s.q, kkt_matrix.Fqq());
    robot.dSubtractdConfigurationPlus(s.q, s_next.q, kkt_matrix.Fqq_prev);
    kkt_residual.lq().noalias() 
        += kkt_matrix.Fqq_prev.transpose() * s_next.lmd
            + kkt_matrix.Fqq().transpose() * s.lmd;
  }
  else {
    kkt_residual.lq().noalias() += s_next.lmd - s.lmd;
  }
  kkt_residual.lv().noalias() += dtau * s.lmd - s.gmm + s_next.gmm;
  kkt_residual.la.noalias() += dtau * s.gmm;
}


template <typename ConfigVectorType, typename TangentVectorType>
inline void LinearizeBackwardEulerTerminal(
    const Robot& robot, const double dtau, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, 
    const Eigen::MatrixBase<TangentVectorType>& v_prev, 
    const SplitSolution& s, KKTMatrix& kkt_matrix, 
    KKTResidual& kkt_residual) {
  assert(dtau > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  ComputeBackwardEulerResidual(robot, dtau, q_prev, v_prev, s, kkt_residual);
  if (robot.has_floating_base()) {
    robot.dSubtractdConfigurationMinus(q_prev, s.q, kkt_matrix.Fqq());
    kkt_residual.lq().noalias() 
        += kkt_matrix.Fqq().transpose() * s.lmd;
  }
  else {
    kkt_residual.lq().noalias() -= s.lmd;
  }
  kkt_residual.lv().noalias() += dtau * s.lmd - s.gmm;
  kkt_residual.la.noalias() += dtau * s.gmm;
}


template <typename ConfigVectorType, typename TangentVectorType>
inline void ComputeForwardEulerResidual(
    const Robot& robot, const double dtau, const SplitSolution& s, 
    const Eigen::MatrixBase<ConfigVectorType>& q_next, 
    const Eigen::MatrixBase<TangentVectorType>& v_next, 
    KKTResidual& kkt_residual) {
  assert(dtau > 0);
  assert(q_next.size() == robot.dimq());
  assert(v_next.size() == robot.dimv());
  robot.subtractConfiguration(s.q, q_next, kkt_residual.Fq());
  kkt_residual.Fq().noalias() += dtau * s.v;
  kkt_residual.Fv() = s.v + dtau * s.a - v_next;
}


template <typename ConfigVectorType, typename TangentVectorType1, 
          typename TangentVectorType2, typename TangentVectorType3>
inline void ComputeForwardEulerResidual(
    const Robot& robot, const double step_size, const double dtau, 
    const SplitSolution& s, const Eigen::MatrixBase<ConfigVectorType>& q_next, 
    const Eigen::MatrixBase<TangentVectorType1>& v_next, 
    const Eigen::MatrixBase<TangentVectorType2>& dq_next, 
    const Eigen::MatrixBase<TangentVectorType3>& dv_next, 
    KKTResidual& kkt_residual) {
  assert(dtau > 0);
  assert(q_next.size() == robot.dimq());
  assert(v_next.size() == robot.dimv());
  assert(dq_next.size() == robot.dimv());
  assert(dv_next.size() == robot.dimv());
  robot.subtractConfiguration(s.q, q_next, kkt_residual.Fq());
  kkt_residual.Fq().noalias() += dtau * s.v - step_size * dq_next;
  kkt_residual.Fv() = s.v + dtau * s.a - v_next - step_size * dv_next;
}


template <typename ConfigVectorType, typename TangentVectorType>
inline void ComputeBackwardEulerResidual(
    const Robot& robot, const double dtau, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, 
    const Eigen::MatrixBase<TangentVectorType>& v_prev, const SplitSolution& s, 
    KKTResidual& kkt_residual) {
  assert(dtau > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  robot.subtractConfiguration(q_prev, s.q, kkt_residual.Fq());
  kkt_residual.Fq().noalias() += dtau * s.v;
  kkt_residual.Fv() = v_prev - s.v + dtau * s.a;
}


template <typename ConfigVectorType, typename TangentVectorType1, 
          typename TangentVectorType2, typename TangentVectorType3>
inline void ComputeBackwardEulerResidual(
    const Robot& robot, const double step_size, const double dtau, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, 
    const Eigen::MatrixBase<TangentVectorType1>& v_prev, 
    const Eigen::MatrixBase<TangentVectorType2>& dq_prev, 
    const Eigen::MatrixBase<TangentVectorType3>& dv_prev, 
    const SplitSolution& s, KKTResidual& kkt_residual) {
  assert(dtau > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  assert(dq_prev.size() == robot.dimv());
  assert(dv_prev.size() == robot.dimv());
  robot.subtractConfiguration(q_prev, s.q, kkt_residual.Fq());
  kkt_residual.Fq().noalias() += dtau * s.v + step_size * dq_prev;
  kkt_residual.Fv() = v_prev - s.v + dtau * s.a + step_size * dv_prev;
}


inline double L1NormStateEuqationResidual(const KKTResidual& kkt_residual) {
  return kkt_residual.Fx().lpNorm<1>();
}


inline double SquaredNormStateEuqationResidual(
    const KKTResidual& kkt_residual) {
  return kkt_residual.Fx().squaredNorm();
}

} // namespace stateequation
} // namespace idocp 

#endif // IDOCP_STATE_EQUATION_HXX_