#ifndef IDOCP_STATE_EQUATION_HXX_
#define IDOCP_STATE_EQUATION_HXX_

#include "idocp/ocp/state_equation.hpp"

#include <cassert>

namespace idocp {
namespace stateequation {

template <typename ConfigVectorType, typename SplitSolutionType>
inline void linearizeForwardEuler(
    const Robot& robot, const double dt, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, const SplitSolution& s, 
    const SplitSolutionType& s_next, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  assert(q_prev.size() == robot.dimq());
  computeForwardEulerResidual(robot, dt, s, s_next.q, s_next.v, kkt_residual);
  if (robot.hasFloatingBase()) {
    robot.dSubtractdConfigurationPlus(s.q, s_next.q, kkt_matrix.Fqq());
    robot.dSubtractdConfigurationMinus(q_prev, s.q, kkt_matrix.Fqq_prev);
    kkt_residual.lq().template head<6>().noalias() 
        += kkt_matrix.Fqq().template topLeftCorner<6, 6>().transpose() 
              * s_next.lmd.template head<6>();
    kkt_residual.lq().template head<6>().noalias() 
        += kkt_matrix.Fqq_prev.template topLeftCorner<6, 6>().transpose() 
              * s.lmd.template head<6>();
    kkt_residual.lq().tail(robot.dimv()-6).noalias() 
        += s_next.lmd.tail(robot.dimv()-6) - s.lmd.tail(robot.dimv()-6);
  }
  else {
    kkt_residual.lq().noalias() += s_next.lmd - s.lmd;
  }
  kkt_residual.lv().noalias() += dt * s_next.lmd + s_next.gmm - s.gmm;
  kkt_residual.la.noalias() += dt * s_next.gmm;
}


template <typename ConfigVectorType>
inline void condenseForwardEuler(
    Robot& robot, const double dt, const SplitSolution& s, 
    const Eigen::MatrixBase<ConfigVectorType>& q_next, 
    SplitKKTMatrix& kkt_matrix, SplitKKTResidual& kkt_residual) {
  if (robot.hasFloatingBase()) {
    assert(dt > 0);
    robot.dSubtractdConfigurationInverse(kkt_matrix.Fqq_prev, 
                                         kkt_matrix.Fqq_prev_inv);
    robot.dSubtractdConfigurationMinus(s.q, q_next, kkt_matrix.Fqq_prev);
    robot.dSubtractdConfigurationInverse(kkt_matrix.Fqq_prev, 
                                         kkt_matrix.Fqq_inv);
    kkt_matrix.Fqq_prev.template topLeftCorner<6, 6>() 
        = kkt_matrix.Fqq().template topLeftCorner<6, 6>();
    kkt_residual.Fq_tmp.template head<6>() 
        = kkt_residual.Fq().template head<6>();
    kkt_matrix.Fqq().template topLeftCorner<6, 6>().noalias()
        = - kkt_matrix.Fqq_inv 
            * kkt_matrix.Fqq_prev.template topLeftCorner<6, 6>();
    kkt_matrix.Fqv().template topLeftCorner<6, 6>()
        = - dt * kkt_matrix.Fqq_inv;
    kkt_residual.Fq().template head<6>().noalias()
        = - kkt_matrix.Fqq_inv * kkt_residual.Fq_tmp.template head<6>();
  }
}


template <typename ConfigVectorType>
inline void linearizeForwardEulerTerminal(
    const Robot& robot, const Eigen::MatrixBase<ConfigVectorType>& q_prev, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) {
  assert(q_prev.size() == robot.dimq());
  if (robot.hasFloatingBase()) {
    robot.dSubtractdConfigurationMinus(q_prev, s.q, kkt_matrix.Fqq_prev);
    kkt_residual.lq().template head<6>().noalias() 
        += kkt_matrix.Fqq_prev.template topLeftCorner<6, 6>().transpose() 
              * s.lmd.template head<6>();
    kkt_residual.lq().tail(robot.dimv()-6).noalias() 
        -= s.lmd.tail(robot.dimv()-6);
  }
  else {
    kkt_residual.lq().noalias() -= s.lmd;
  }
  kkt_residual.lv().noalias() -= s.gmm;
}


inline void condenseForwardEulerTerminal(Robot& robot, 
                                         SplitKKTMatrix& kkt_matrix) {
  if (robot.hasFloatingBase()) {
    robot.dSubtractdConfigurationInverse(kkt_matrix.Fqq_prev, 
                                         kkt_matrix.Fqq_prev_inv);
  }
}


template <typename SplitKKTMatrixType, typename SplitKKTResidualType, 
          typename VectorType>
inline void correctCostateDirectionForwardEuler(
    const Robot& robot, const SplitKKTMatrixType& kkt_matrix, 
    SplitKKTResidualType& kkt_residual,
    const Eigen::MatrixBase<VectorType>& dlmd) {
  if (robot.hasFloatingBase()) {
    kkt_residual.Fq_tmp.template head<6>().noalias() 
        = kkt_matrix.Fqq_prev_inv.transpose() * dlmd.template head<6>();
    const_cast<Eigen::MatrixBase<VectorType>&> (dlmd).template head<6>() 
        = - kkt_residual.Fq_tmp.template head<6>();
  }
}


template <typename ConfigVectorType, typename TangentVectorType, 
          typename SplitSolutionType>
inline void linearizeBackwardEuler(
    const Robot& robot, const double dt, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, 
    const Eigen::MatrixBase<TangentVectorType>& v_prev, 
    const SplitSolution& s, const SplitSolutionType& s_next,
    SplitKKTMatrix& kkt_matrix, SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  computeBackwardEulerResidual(robot, dt, q_prev, v_prev, s, kkt_residual);
  if (robot.hasFloatingBase()) {
    robot.dSubtractdConfigurationMinus(q_prev, s.q, kkt_matrix.Fqq());
    robot.dSubtractdConfigurationPlus(s.q, s_next.q, kkt_matrix.Fqq_prev);
    kkt_residual.lq().template head<6>().noalias() 
        += kkt_matrix.Fqq_prev.template topLeftCorner<6, 6>().transpose() 
              * s_next.lmd.template head<6>();
    kkt_residual.lq().template head<6>().noalias() 
        += kkt_matrix.Fqq().template topLeftCorner<6, 6>().transpose() 
              * s.lmd.template head<6>();
    kkt_residual.lq().tail(robot.dimv()-6).noalias() 
        += s_next.lmd.tail(robot.dimv()-6) - s.lmd.tail(robot.dimv()-6);
  }
  else {
    kkt_residual.lq().noalias() += s_next.lmd - s.lmd;
  }
  kkt_residual.lv().noalias() += dt * s.lmd - s.gmm + s_next.gmm;
  kkt_residual.la.noalias() += dt * s.gmm;
}


template <typename ConfigVectorType, typename TangentVectorType>
inline void linearizeBackwardEulerTerminal(
    const Robot& robot, const double dt, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, 
    const Eigen::MatrixBase<TangentVectorType>& v_prev, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix, 
    SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  computeBackwardEulerResidual(robot, dt, q_prev, v_prev, s, kkt_residual);
  if (robot.hasFloatingBase()) {
    robot.dSubtractdConfigurationMinus(q_prev, s.q, kkt_matrix.Fqq());
    kkt_residual.lq().template head<6>().noalias() 
        += kkt_matrix.Fqq().template topLeftCorner<6, 6>().transpose() 
              * s.lmd.template head<6>();
    kkt_residual.lq().tail(robot.dimv()-6).noalias() 
        -= s.lmd.tail(robot.dimv()-6);
  }
  else {
    kkt_residual.lq().noalias() -= s.lmd;
  }
  kkt_residual.lv().noalias() += dt * s.lmd - s.gmm;
  kkt_residual.la.noalias() += dt * s.gmm;
}


template <typename ConfigVectorType>
inline void condenseBackwardEuler(
    Robot& robot, const double dt, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, const SplitSolution& s, 
    SplitKKTMatrix& kkt_matrix, SplitKKTResidual& kkt_residual) {
  if (robot.hasFloatingBase()) {
    assert(dt > 0);
    robot.dSubtractdConfigurationPlus(q_prev, s.q, kkt_matrix.Fqq_prev);
    robot.dSubtractdConfigurationInverse(kkt_matrix.Fqq_prev, 
                                         kkt_matrix.Fqq_inv);
    kkt_matrix.Fqq_prev.template topLeftCorner<6, 6>() 
        = kkt_matrix.Fqq().template topLeftCorner<6, 6>();
    kkt_residual.Fq_tmp.template head<6>() 
        = kkt_residual.Fq().template head<6>();
    kkt_matrix.Fqq().template topLeftCorner<6, 6>().noalias()
        = kkt_matrix.Fqq_inv 
            * kkt_matrix.Fqq_prev.template topLeftCorner<6, 6>();
    kkt_matrix.Fqv().template topLeftCorner<6, 6>()
        = dt * kkt_matrix.Fqq_inv;
    kkt_residual.Fq().template head<6>().noalias()
        = kkt_matrix.Fqq_inv * kkt_residual.Fq_tmp.template head<6>();
  }
}


template <typename SplitKKTMatrixType, typename SplitKKTResidualType, 
          typename VectorType>
inline void correctCostateDirectionBackwardEuler(
    const Robot& robot, const SplitKKTMatrixType& kkt_matrix, 
    SplitKKTResidualType& kkt_residual,
    const Eigen::MatrixBase<VectorType>& dlmd) {
  if (robot.hasFloatingBase()) {
    kkt_residual.Fq_tmp.template head<6>().noalias() 
        = kkt_matrix.Fqq_inv.transpose() * dlmd.template head<6>();
    const_cast<Eigen::MatrixBase<VectorType>&> (dlmd).template head<6>() 
        = kkt_residual.Fq_tmp.template head<6>();
  }
}


template <typename ConfigVectorType, typename TangentVectorType>
inline void computeForwardEulerResidual(
    const Robot& robot, const double dt, const SplitSolution& s, 
    const Eigen::MatrixBase<ConfigVectorType>& q_next, 
    const Eigen::MatrixBase<TangentVectorType>& v_next, 
    SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  assert(q_next.size() == robot.dimq());
  assert(v_next.size() == robot.dimv());
  robot.subtractConfiguration(s.q, q_next, kkt_residual.Fq());
  kkt_residual.Fq().noalias() += dt * s.v;
  kkt_residual.Fv() = s.v + dt * s.a - v_next;
}


template <typename ConfigVectorType, typename TangentVectorType>
inline void computeBackwardEulerResidual(
    const Robot& robot, const double dt, 
    const Eigen::MatrixBase<ConfigVectorType>& q_prev, 
    const Eigen::MatrixBase<TangentVectorType>& v_prev, const SplitSolution& s, 
    SplitKKTResidual& kkt_residual) {
  assert(dt > 0);
  assert(q_prev.size() == robot.dimq());
  assert(v_prev.size() == robot.dimv());
  robot.subtractConfiguration(q_prev, s.q, kkt_residual.Fq());
  kkt_residual.Fq().noalias() += dt * s.v;
  kkt_residual.Fv() = v_prev - s.v + dt * s.a;
}


inline double l1NormStateEuqationResidual(
    const SplitKKTResidual& kkt_residual) {
  return kkt_residual.Fx.lpNorm<1>();
}


inline double squaredNormStateEuqationResidual(
    const SplitKKTResidual& kkt_residual) {
  return kkt_residual.Fx.squaredNorm();
}

} // namespace stateequation 
} // namespace idocp 

#endif // IDOCP_STATE_EQUATION_HXX_