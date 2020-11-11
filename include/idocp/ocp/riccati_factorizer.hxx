#ifndef IDOCP_RICCATI_FACTORIZER_HXX_
#define IDOCP_RICCATI_FACTORIZER_HXX_

#include "idocp/ocp/riccati_factorizer.hpp"

#include <cassert>

namespace idocp {

inline RiccatiFactorizer::RiccatiFactorizer(const Robot& robot) 
  : has_floating_base_(robot.has_floating_base()),
    dimv_(robot.dimv()),
    dimu_(robot.dimu()),
    llt_(robot.dimu()),
    AtPqq_(Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv())),
    AtPqv_(Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv())),
    AtPvq_(Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv())),
    AtPvv_(Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv())),
    BtPq_(Eigen::MatrixXd::Zero(robot.dimu(), robot.dimv())),
    BtPv_(Eigen::MatrixXd::Zero(robot.dimu(), robot.dimv())),
    GK_(Eigen::MatrixXd::Zero(robot.dimu(), 2*robot.dimv())),
    GinvBt_(Eigen::MatrixXd::Zero(robot.dimu(), robot.dimv())),
    NApBKt_(Eigen::MatrixXd::Zero(2*robot.dimv(), 2*robot.dimv())) {
}


inline RiccatiFactorizer::RiccatiFactorizer() 
  : has_floating_base_(false),
    dimv_(0),
    dimu_(0),
    llt_(),
    AtPqq_(),
    AtPqv_(),
    AtPvq_(),
    AtPvv_(),
    BtPq_(),
    BtPv_(),
    GK_(),
    GinvBt_(), 
    NApBKt_() {
}


inline RiccatiFactorizer::~RiccatiFactorizer() {
}


inline void RiccatiFactorizer::backwardRiccatiRecursion(
    const RiccatiFactorization& riccati_next, const double dtau, 
    KKTMatrix& kkt_matrix, KKTResidual& kkt_residual, 
    RiccatiFactorization& riccati) {
  assert(dtau > 0);
  factorizeKKTMatrix(riccati_next, dtau, kkt_matrix, kkt_residual);
  computeFeedbackGainAndFeedforward(kkt_matrix, kkt_residual, riccati);
  factorizeRiccatiFactorization(riccati_next, dtau, kkt_matrix, kkt_residual, riccati);
}


inline void RiccatiFactorizer::forwardRiccatiRecursion(
    const KKTMatrix& kkt_matrix, const KKTResidual& kkt_residual,
    const SplitDirection& d, const double dtau, SplitDirection& d_next) const {
  assert(dtau > 0);
  if (has_floating_base_) {
    d_next.dq().noalias() = kkt_matrix.Fqq() * d.dq() + dtau * d.dv() 
                              + kkt_residual.Fq();
  }
  else {
    d_next.dq().noalias() = d.dq() + dtau * d.dv() + kkt_residual.Fq();
  }
  d_next.dv().noalias() = kkt_matrix.Fvq() * d.dq() + kkt_matrix.Fvv() * d.dv() 
                            + kkt_matrix.Fvu() * d.du() + kkt_residual.Fv();
}


inline void RiccatiFactorizer::factorizeStateConstraintParallel(
    const KKTMatrix& kkt_matrix, const KKTResidual& kkt_residual, 
    const double dtau, RiccatiFactorization& riccati) {
  if (has_floating_base_) {
    riccati.ApBK.topLeftCorner(dimv_, dimv_) = kkt_matrix.Fqq();
  }
  else {
    riccati.ApBK.topLeftCorner(dimv_, dimv_).setIdentity();
  }
  riccati.ApBK.topRightCorner(dimv_, dimv_) 
      = dtau * Eigen::MatrixXd::Identity(dimv_, dimv_);
  riccati.ApBK.bottomLeftCorner(dimv_, dimv_) = kkt_matrix.Fvq();
  riccati.ApBK.bottomRightCorner(dimv_, dimv_) = kkt_matrix.Fvv();
  riccati.ApBK.bottomRows(dimv_).noalias() += kkt_matrix.Fvu() * riccati.K;
  GinvBt_.noalias() = llt_.solve(kkt_matrix.Fvu().transpose());
  riccati.BGinvBt.noalias() = kkt_matrix.Fvu() * GinvBt_;
  riccati.apBk = kkt_residual.Fx();
  riccati.apBk.tail(dimv_).noalias() = kkt_matrix.Fvu() * riccati.k;
}


inline void RiccatiFactorizer::factorizeStateConstraintSerial(
    const RiccatiFactorization& riccati, 
    RiccatiFactorization& riccati_next) {
  riccati_next.Pi.noalias() = riccati.ApBK * riccati.Pi;
  riccati_next.pi.noalias() = riccati.apBk + riccati.ApBK * riccati.pi;
  riccati_next.N = riccati.BGinvBt;
  NApBKt_.noalias() = riccati.N * riccati.ApBK.transpose();
  riccati_next.N.noalias() += riccati.ApBK * NApBKt_;
}


inline void RiccatiFactorizer::factorizeStateConstraintSerialInitial(
    RiccatiFactorization& riccati) {
  riccati.Pi = riccati.ApBK;
  riccati.pi = riccati.apBk;
  riccati.N.setZero();
}


inline void RiccatiFactorizer::computeCostateDirection(
    const RiccatiFactorization& riccati, SplitDirection& d) {
  d.dlmd().noalias() = riccati.Pqq * d.dq();
  d.dlmd().noalias() += riccati.Pqv * d.dv();
  d.dlmd().noalias() -= riccati.sq;
  d.dgmm().noalias() = riccati.Pqv.transpose() * d.dq();
  d.dgmm().noalias() += riccati.Pvv * d.dv();
  d.dgmm().noalias() -= riccati.sv;
}


inline void RiccatiFactorizer::computeCostateDirection(
    const RiccatiFactorization& riccati, 
    const std::vector<StateConstraintRiccatiFactorization>& cons, 
    SplitDirection& d) {
  computeCostateDirection(riccati, d);
}


inline void RiccatiFactorizer::computeControlInputDirection(
    const RiccatiFactorization& riccati, SplitDirection& d) {
  d.du() = riccati.k;
  d.du().noalias() += riccati.K * d.dx();
}


inline void RiccatiFactorizer::factorizeKKTMatrix(
    const RiccatiFactorization& riccati_next, const double dtau, 
    KKTMatrix& kkt_matrix, KKTResidual& kkt_residual) {
  assert(dtau > 0);
  if (has_floating_base_) {
    AtPqq_.noalias() = kkt_matrix.Fqq().transpose() * riccati_next.Pqq;
    AtPqq_.noalias() += kkt_matrix.Fvq().transpose() * riccati_next.Pqv.transpose();
    AtPqv_.noalias() = kkt_matrix.Fqq().transpose() * riccati_next.Pqv;
    AtPqv_.noalias() += kkt_matrix.Fvq().transpose() * riccati_next.Pvv;
    AtPvq_ = dtau * riccati_next.Pqq;
    AtPvq_.noalias() += kkt_matrix.Fvv().transpose() * riccati_next.Pqv.transpose();
    AtPvv_ = dtau * riccati_next.Pqv;
    AtPvv_.noalias() += kkt_matrix.Fvv().transpose() * riccati_next.Pvv;
  }
  else {
    AtPqq_ = riccati_next.Pqq;
    AtPqq_.noalias() += kkt_matrix.Fvq().transpose() * riccati_next.Pqv.transpose();
    AtPqv_ = riccati_next.Pqv;
    AtPqv_.noalias() += kkt_matrix.Fvq().transpose() * riccati_next.Pvv;
    AtPvq_ = dtau * riccati_next.Pqq;
    AtPvq_.noalias() += kkt_matrix.Fvv().transpose() * riccati_next.Pqv.transpose();
    AtPvv_ = dtau * riccati_next.Pqv;
    AtPvv_.noalias() += kkt_matrix.Fvv().transpose() * riccati_next.Pvv;
  }
  BtPq_.noalias() = kkt_matrix.Fvu().transpose() * riccati_next.Pqv.transpose();
  BtPv_.noalias() = kkt_matrix.Fvu().transpose() * riccati_next.Pvv;
  // Factorize F
  if (has_floating_base_) {
    kkt_matrix.Qqq().noalias() += AtPqq_ * kkt_matrix.Fqq();
    kkt_matrix.Qqq().noalias() += AtPqv_ * kkt_matrix.Fvq();
    kkt_matrix.Qqv().noalias() += dtau * AtPqq_;
    kkt_matrix.Qqv().noalias() += AtPqv_ * kkt_matrix.Fvv();
    kkt_matrix.Qvq() = kkt_matrix.Qqv().transpose();
    kkt_matrix.Qvv().noalias() += dtau * AtPvq_;
    kkt_matrix.Qvv().noalias() += AtPvv_ * kkt_matrix.Fvv();
  }
  else {
    kkt_matrix.Qqq().noalias() += AtPqq_;
    kkt_matrix.Qqq().noalias() += AtPqv_ * kkt_matrix.Fvq();
    kkt_matrix.Qqv().noalias() += dtau * AtPqq_;
    kkt_matrix.Qqv().noalias() += AtPqv_ * kkt_matrix.Fvv();
    kkt_matrix.Qvq() = kkt_matrix.Qqv().transpose();
    kkt_matrix.Qvv().noalias() += dtau * AtPvq_;
    kkt_matrix.Qvv().noalias() += AtPvv_ * kkt_matrix.Fvv();
  }
  // Factorize H
  kkt_matrix.Qqu().noalias() += AtPqv_ * kkt_matrix.Fvu();
  kkt_matrix.Qvu().noalias() += AtPvv_ * kkt_matrix.Fvu();
  // Factorize G
  kkt_matrix.Quu().noalias() += BtPv_ * kkt_matrix.Fvu();
  // Factorize vector term
  kkt_residual.lu().noalias() += BtPq_ * kkt_residual.Fq();
  kkt_residual.lu().noalias() += BtPv_ * kkt_residual.Fv();
  kkt_residual.lu().noalias() -= kkt_matrix.Fvu().transpose() * riccati_next.sv;
}


inline void RiccatiFactorizer::computeFeedbackGainAndFeedforward(
    const KKTMatrix& kkt_matrix, const KKTResidual& kkt_residual, 
    RiccatiFactorization& riccati) {
  llt_.compute(kkt_matrix.Quu());
  assert(llt_.info() == Eigen::Success);
  riccati.K = - llt_.solve(kkt_matrix.Qxu().transpose());
  riccati.k = - llt_.solve(kkt_residual.lu());
}


inline void RiccatiFactorizer::factorizeRiccatiFactorization(
    const RiccatiFactorization& riccati_next, const double dtau, 
    const KKTMatrix& kkt_matrix, const KKTResidual& kkt_residual, 
    RiccatiFactorization& riccati) {
  assert(dtau > 0);
  riccati.Pqq = kkt_matrix.Qqq();
  riccati.Pqv = kkt_matrix.Qqv();
  riccati.Pvv = kkt_matrix.Qvv();
  GK_.noalias() = kkt_matrix.Quu() * riccati.K; 
  riccati.Pqq.noalias() -= riccati.Kq().transpose() * GK_.leftCols(dimv_);
  riccati.Pqv.noalias() -= riccati.Kq().transpose() * GK_.rightCols(dimv_);
  riccati.Pvv.noalias() -= riccati.Kv().transpose() * GK_.rightCols(dimv_);
  riccati.Pvq = riccati.Pqv.transpose();
  // preserve the symmetry
  riccati.Pqq = 0.5 * (riccati.Pqq + riccati.Pqq.transpose()).eval();
  riccati.Pvv = 0.5 * (riccati.Pvv + riccati.Pvv.transpose()).eval();
  if (has_floating_base_) {
    riccati.sq.noalias() = kkt_matrix.Fqq().transpose() * riccati_next.sq;
    riccati.sq.noalias() += kkt_matrix.Fvq().transpose() * riccati_next.sv;
    riccati.sv.noalias() = dtau * riccati_next.sq;
    riccati.sv.noalias() += kkt_matrix.Fvv().transpose() * riccati_next.sv;
  }
  else {
    riccati.sq.noalias() = riccati_next.sq;
    riccati.sq.noalias() += kkt_matrix.Fvq().transpose() * riccati_next.sv;
    riccati.sv.noalias() = dtau * riccati_next.sq;
    riccati.sv.noalias() += kkt_matrix.Fvv().transpose() * riccati_next.sv;
  }
  riccati.sq.noalias() -= AtPqq_ * kkt_residual.Fq();
  riccati.sq.noalias() -= AtPqv_ * kkt_residual.Fv();
  riccati.sv.noalias() -= AtPvq_ * kkt_residual.Fq();
  riccati.sv.noalias() -= AtPvv_ * kkt_residual.Fv();
  riccati.sq.noalias() -= kkt_residual.lq();
  riccati.sv.noalias() -= kkt_residual.lv();
  riccati.sq.noalias() -= kkt_matrix.Qqu() * riccati.k;
  riccati.sv.noalias() -= kkt_matrix.Qvu() * riccati.k;
}

} // namespace idocp

#endif // IDOCP_RICCATI_FACTORIZER_HXX_