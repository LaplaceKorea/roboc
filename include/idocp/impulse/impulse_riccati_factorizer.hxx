#ifndef IDOCP_IMPULSE_RICCATI_FACTORIZER_HXX_
#define IDOCP_IMPULSE_RICCATI_FACTORIZER_HXX_

#include "idocp/impulse/impulse_riccati_factorizer.hpp"

namespace idocp {

inline ImpulseRiccatiFactorizer::ImpulseRiccatiFactorizer(
    const Robot& robot) 
  : has_floating_base_(robot.has_floating_base()),
    dimv_(robot.dimv()),
    backward_recursion_(robot),
    forward_recursion_(robot) {
}


inline ImpulseRiccatiFactorizer::ImpulseRiccatiFactorizer() 
  : has_floating_base_(false),
    dimv_(0),
    backward_recursion_(),
    forward_recursion_() {
}


inline ImpulseRiccatiFactorizer::~ImpulseRiccatiFactorizer() {
}


inline void ImpulseRiccatiFactorizer::backwardRiccatiRecursion(
    const RiccatiFactorization& riccati_next, ImpulseSplitKKTMatrix& kkt_matrix, 
    ImpulseSplitKKTResidual& kkt_residual, RiccatiFactorization& riccati) {
  backward_recursion_.factorizeKKTMatrix(riccati_next, kkt_matrix);
  backward_recursion_.factorizeRiccatiFactorization(riccati_next, kkt_matrix, 
                                                    kkt_residual, riccati);
}


inline void ImpulseRiccatiFactorizer::forwardStateConstraintFactorization(
    const RiccatiFactorization& riccati, const ImpulseSplitKKTMatrix& kkt_matrix, 
    const ImpulseSplitKKTResidual& kkt_residual, 
    RiccatiFactorization& riccati_next, const bool exist_state_constraint) {
  forward_recursion_.factorizeStateTransition(riccati, kkt_matrix, kkt_residual, 
                                              riccati_next);
  if (exist_state_constraint) {
    forward_recursion_.factorizeStateConstraintFactorization(riccati, 
                                                             kkt_matrix, 
                                                             riccati_next);
  }
}


template <typename MatrixType1, typename MatrixType2>
inline void ImpulseRiccatiFactorizer::backwardStateConstraintFactorization(
    const Eigen::MatrixBase<MatrixType1>& T_next,
    const ImpulseSplitKKTMatrix& kkt_matrix, 
    const Eigen::MatrixBase<MatrixType2>& T) const {
  assert(T_next.rows() == T.rows());
  assert(T_next.rows() == T.rows());
  if (has_floating_base_) {
    const_cast<Eigen::MatrixBase<MatrixType2>&> (T).topRows(dimv_).noalias() 
        = kkt_matrix.Fqq().transpose() * T_next.topRows(dimv_);
  }
  else {
    const_cast<Eigen::MatrixBase<MatrixType2>&> (T).topRows(dimv_) 
        = T_next.topRows(dimv_);
  }
  const_cast<Eigen::MatrixBase<MatrixType2>&> (T).topRows(dimv_).noalias()
      += kkt_matrix.Fvq().transpose() * T_next.bottomRows(dimv_);
  const_cast<Eigen::MatrixBase<MatrixType2>&> (T).bottomRows(dimv_).noalias() 
      =  kkt_matrix.Fvv().transpose() * T_next.bottomRows(dimv_);
}


inline void ImpulseRiccatiFactorizer::forwardRiccatiRecursion(
    const ImpulseSplitKKTMatrix& kkt_matrix, const ImpulseSplitKKTResidual& kkt_residual, 
    const ImpulseSplitDirection& d, SplitDirection& d_next) const {
  d_next.dx() = kkt_residual.Fx();
  if (has_floating_base_) {
    d_next.dq().noalias() += kkt_matrix.Fqq() * d.dq();
  }
  else {
    d_next.dq().noalias() += d.dq();
  }
  d_next.dv().noalias() += kkt_matrix.Fvq() * d.dq();
  d_next.dv().noalias() += kkt_matrix.Fvv() * d.dv();
}


inline void ImpulseRiccatiFactorizer::computeCostateDirection(
    const RiccatiFactorization& riccati, ImpulseSplitDirection& d,
    const bool exist_state_constraint) {
  d.dlmd().noalias() = riccati.Pqq * d.dq();
  d.dlmd().noalias() += riccati.Pqv * d.dv();
  d.dlmd().noalias() -= riccati.sq;
  d.dgmm().noalias() = riccati.Pqv.transpose() * d.dq();
  d.dgmm().noalias() += riccati.Pvv * d.dv();
  d.dgmm().noalias() -= riccati.sv;
  if (exist_state_constraint) {
    d.dlmdgmm().noalias() += riccati.n;
  }
}

} // namespace idocp

#endif // IDOCP_IMPULSE_RICCATI_FACTORIZER_HXX_