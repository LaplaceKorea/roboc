#ifndef IDOCP_RICCATI_MATRIX_INVERTER_HXX_
#define IDOCP_RICCATI_MATRIX_INVERTER_HXX_

#include "Eigen/LU"

#include <assert.h>

namespace idocp {

inline RiccatiMatrixInverter::RiccatiMatrixInverter(const Robot& robot) 
  : dimv_(robot.dimv()),
    dim_passive_(robot.dim_passive()),
    dimf_(0),
    dimc_(robot.dim_passive()),
    dimaf_(robot.dimv()),
    G_inv_(Eigen::MatrixXd::Zero(robot.dimv()+2*robot.max_dimf()+robot.dim_passive(), 
                                 robot.dimv()+2*robot.max_dimf()+robot.dim_passive())),
    Sc_(Eigen::MatrixXd::Zero(robot.max_dimf()+robot.dim_passive(), 
                              robot.max_dimf()+robot.dim_passive())),
    G_inv_Caf_trans_(Eigen::MatrixXd::Zero(robot.dimv()+robot.max_dimf(), 
                                           robot.max_dimf()+robot.dim_passive())) {
}


inline RiccatiMatrixInverter::RiccatiMatrixInverter() 
  : dimv_(0),
    dim_passive_(0),
    dimf_(0),
    dimc_(0),
    dimaf_(0),
    G_inv_(),
    Sc_(),
    G_inv_Caf_trans_() {
}


inline RiccatiMatrixInverter::~RiccatiMatrixInverter() {
}


inline void RiccatiMatrixInverter::setContactStatus(
    const ContactStatus& contact_status) {
  dimf_ = contact_status.dimf();
  dimaf_ = dimv_ + contact_status.dimf();
  dimc_ = dim_passive_ + contact_status.dimf();
}


template <typename MatrixType1, typename MatrixType2, typename MatrixType3>
inline void RiccatiMatrixInverter::invert(
    const Eigen::MatrixBase<MatrixType1>& G, 
    const Eigen::MatrixBase<MatrixType2>& Caf, 
    const Eigen::MatrixBase<MatrixType3>& G_inv) {
  assert(G.rows() == dimaf_);
  assert(G.cols() == dimaf_);
  assert(Caf.rows() == dimc_);
  assert(Caf.cols() == dimaf_);
  assert(G_inv.rows() == dimaf_+dimc_);
  assert(G_inv.cols() == dimaf_+dimc_);
  if (dimc_ > 0) {
    const_cast<Eigen::MatrixBase<MatrixType3>&> (G_inv)
        .topLeftCorner(dimaf_, dimaf_).noalias()
        = G.llt().solve(Eigen::MatrixXd::Identity(dimaf_, dimaf_));
    G_inv_Caf_trans_.topLeftCorner(dimaf_, dimc_).noalias() 
        = G_inv.topLeftCorner(dimaf_, dimaf_) * Caf.transpose();
    Sc_.topLeftCorner(dimc_, dimc_).noalias()
        = Caf * G_inv_Caf_trans_.topLeftCorner(dimaf_, dimc_);
    const_cast<Eigen::MatrixBase<MatrixType3>&> (G_inv)
        .block(dimaf_, dimaf_, dimc_, dimc_).noalias() 
        = - Sc_.topLeftCorner(dimc_, dimc_)
               .llt().solve(Eigen::MatrixXd::Identity(dimc_, dimc_));
    const_cast<Eigen::MatrixBase<MatrixType3>&> (G_inv)
        .block(0, dimaf_, dimaf_, dimc_).noalias() 
        = - G_inv_Caf_trans_.topLeftCorner(dimaf_, dimc_)
            * G_inv.block(dimaf_, dimaf_, dimc_, dimc_);
    const_cast<Eigen::MatrixBase<MatrixType3>&> (G_inv)
        .block(dimaf_, 0, dimc_, dimaf_).noalias() 
        = G_inv.block(0, dimaf_, dimaf_, dimc_).transpose();
    const_cast<Eigen::MatrixBase<MatrixType3>&> (G_inv)
        .topLeftCorner(dimaf_, dimaf_).noalias()
        -= G_inv.block(0, dimaf_, dimaf_, dimc_) 
            * Sc_.topLeftCorner(dimc_, dimc_)
            * G_inv.block(0, dimaf_, dimaf_, dimc_).transpose();
  }
  else {
    const_cast<Eigen::MatrixBase<MatrixType3>&> (G_inv).noalias()
        = G.llt().solve(Eigen::MatrixXd::Identity(dimaf_, dimaf_));
  }
}


template <typename MatrixType1, typename MatrixType2>
inline void RiccatiMatrixInverter::invert(
    const Eigen::MatrixBase<MatrixType1>& G, 
    const Eigen::MatrixBase<MatrixType2>& Caf) {
  assert(G.rows() == dimaf_);
  assert(G.cols() == dimaf_);
  assert(Caf.rows() == dimc_);
  assert(Caf.cols() == dimaf_);
  if (dimc_ > 0) {
    G_inv_.topLeftCorner(dimaf_, dimaf_).noalias()
        = G.llt().solve(Eigen::MatrixXd::Identity(dimaf_, dimaf_));
    G_inv_Caf_trans_.topLeftCorner(dimaf_, dimc_).noalias() 
        = G_inv_.topLeftCorner(dimaf_, dimaf_) * Caf.transpose();
    Sc_.topLeftCorner(dimc_, dimc_).noalias() 
        = Caf * G_inv_Caf_trans_.topLeftCorner(dimaf_, dimc_);
    G_inv_.block(dimaf_, dimaf_, dimc_, dimc_).noalias() 
        = - Sc_.topLeftCorner(dimc_, dimc_)
               .llt().solve(Eigen::MatrixXd::Identity(dimc_, dimc_));
    G_inv_.block(0, dimaf_, dimaf_, dimc_).noalias() 
        = - G_inv_Caf_trans_.topLeftCorner(dimaf_, dimc_)
              * G_inv_.block(dimaf_, dimaf_, dimc_, dimc_);
    G_inv_.block(dimaf_, 0, dimc_, dimaf_).noalias() 
        = G_inv_.block(0, dimaf_, dimaf_, dimc_).transpose();
    G_inv_.topLeftCorner(dimaf_, dimaf_).noalias()
        -= G_inv_.block(0, dimaf_, dimaf_, dimc_) 
            * Sc_.topLeftCorner(dimc_, dimc_)
            * G_inv_.block(0, dimaf_, dimaf_, dimc_).transpose();
  }
  else {
    G_inv_.topLeftCorner(dimaf_, dimaf_).noalias()
        = G.llt().solve(Eigen::MatrixXd::Identity(dimaf_, dimaf_));
  }
}


template <typename MatrixType>
void RiccatiMatrixInverter::getInverseMatrix(
    const Eigen::MatrixBase<MatrixType>& G_inv) {
  assert(G_inv.rows() == dimaf_+dimc_);
  assert(G_inv.cols() == dimaf_+dimc_);
  const_cast<Eigen::MatrixBase<MatrixType>&> (G_inv).noalias()
      = G_inv_.topLeftCorner(dimaf_+dimc_, dimaf_+dimc_);
}


template <typename MatrixType>
void RiccatiMatrixInverter::firstOrderCorrection(
    const double dtau, const Eigen::MatrixBase<MatrixType>& dPvv, 
    const Eigen::MatrixBase<MatrixType>& G_inv) {
  assert(dtau > 0);
  assert(dPvv.rows() == dimv_);
  assert(dPvv.cols() == dimv_);
  assert(G_inv.rows() == dimaf_+dimc_);
  assert(G_inv.cols() == dimaf_+dimc_);
  if (dimc_ > 0) {
    (const_cast<Eigen::MatrixBase<MatrixType>&>(G_inv))
        .topLeftCorner(dimv_, dimv_).noalias()
        += dtau * dtau * G_inv_.block(0, 0, dimv_, dimv_) * dPvv 
                       * G_inv_.block(0, 0, dimv_, dimv_);
    (const_cast<Eigen::MatrixBase<MatrixType>&>(G_inv))
        .topRightCorner(dimv_, dimf_+dimc_).noalias()
        += dtau * dtau * G_inv_.block(0, 0, dimv_, dimv_) * dPvv 
                       * G_inv_.block(0, dimv_, dimv_, dimf_+dimc_);
    (const_cast<Eigen::MatrixBase<MatrixType>&>(G_inv))
        .bottomLeftCorner(dimf_+dimc_, dimv_).noalias()
        += dtau * dtau * G_inv_.block(dimv_, 0, dimf_+dimc_, dimv_) * dPvv 
                       * G_inv_.block(0, 0, dimv_, dimv_);
    (const_cast<Eigen::MatrixBase<MatrixType>&>(G_inv))
        .bottomRightCorner(dimf_+dimc_, dimf_+dimc_).noalias()
        += dtau * dtau * G_inv_.block(dimv_, 0, dimf_+dimc_, dimv_) * dPvv 
                       * G_inv_.block(0, dimv_, dimv_, dimf_+dimc_);
  }
  else {
    (const_cast<Eigen::MatrixBase<MatrixType>&>(G_inv)).noalias()
        += dtau * dtau * G_inv_.topLeftCorner(dimv_, dimv_) * dPvv 
                       * G_inv_.topLeftCorner(dimv_, dimv_);
  }
}

} // namespace idocp


#endif // IDOCP_RICCATI_MATRIX_INVERTER_HXX_