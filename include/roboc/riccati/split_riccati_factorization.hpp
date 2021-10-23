#ifndef ROBOC_SPLIT_RICCATI_FACTORIZATION_HPP_
#define ROBOC_SPLIT_RICCATI_FACTORIZATION_HPP_

#include "Eigen/Core"

#include "roboc/robot/robot.hpp"


namespace roboc {

///
/// @class SplitRiccatiFactorization
/// @brief Riccati factorization matrix and vector for a time stage.
///
class SplitRiccatiFactorization {
public:
  ///
  /// @brief Constructs Riccati factorization matrix and vector.
  /// @param[in] robot Robot model. 
  ///
  SplitRiccatiFactorization(const Robot& robot)
    : P(Eigen::MatrixXd::Zero(2*robot.dimv(), 2*robot.dimv())),
      s(Eigen::VectorXd::Zero(2*robot.dimv())),
      Gmm(Eigen::VectorXd::Zero(2*robot.dimv())),
      xi(0.0),
      eta(0.0),
      Gmm_cvx(Eigen::VectorXd::Zero(2*robot.dimv())),
      hx_cvx(Eigen::VectorXd::Zero(2*robot.dimv())),
      hu_cvx(Eigen::VectorXd::Zero(robot.dimu())),
      xi_cvx(0.0),
      dimv_(robot.dimv()),
      dimx_(2*robot.dimv()) {
  }

  ///
  /// @brief Default constructor. 
  ///
  SplitRiccatiFactorization()
    : P(),
      s(),
      Gmm(),
      xi(0.0),
      eta(0.0),
      Gmm_cvx(),
      hx_cvx(),
      hu_cvx(),
      xi_cvx(0.0),
      dimv_(0.0),
      dimx_(0.0) {
  }

  ///
  /// @brief Destructor. 
  ///
  ~SplitRiccatiFactorization() {
  }

  ///
  /// @brief Default copy constructor. 
  ///
  SplitRiccatiFactorization(const SplitRiccatiFactorization&) = default;

  ///
  /// @brief Default copy operator. 
  ///
  SplitRiccatiFactorization& operator=(
      const SplitRiccatiFactorization&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  SplitRiccatiFactorization(
      SplitRiccatiFactorization&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  SplitRiccatiFactorization& operator=(
      SplitRiccatiFactorization&&) noexcept = default;

  ///
  /// @brief Riccati factorization matrix. Size is 
  /// 2 * Robot::dimv() x 2 * Robot::dimv().
  ///
  Eigen::MatrixXd P;

  ///
  /// @brief Riccati factorization vector. Size is 2 * Robot::dimv().
  ///
  Eigen::VectorXd s;

  Eigen::Block<Eigen::MatrixXd> Pqq() {
    return P.topLeftCorner(dimv_, dimv_); 
  }

  const Eigen::Block<const Eigen::MatrixXd> Pqq() const {
    return P.topLeftCorner(dimv_, dimv_); 
  }

  Eigen::Block<Eigen::MatrixXd> Pqv() {
    return P.topRightCorner(dimv_, dimv_); 
  }

  const Eigen::Block<const Eigen::MatrixXd> Pqv() const {
    return P.topRightCorner(dimv_, dimv_); 
  }

  Eigen::Block<Eigen::MatrixXd> Pvq() {
    return P.bottomLeftCorner(dimv_, dimv_); 
  }

  const Eigen::Block<const Eigen::MatrixXd> Pvq() const {
    return P.bottomLeftCorner(dimv_, dimv_); 
  }

  Eigen::Block<Eigen::MatrixXd> Pvv() {
    return P.bottomRightCorner(dimv_, dimv_); 
  }

  const Eigen::Block<const Eigen::MatrixXd> Pvv() const {
    return P.bottomRightCorner(dimv_, dimv_); 
  }

  Eigen::VectorBlock<Eigen::VectorXd> sq() {
    return s.head(dimv_);
  }

  const Eigen::VectorBlock<const Eigen::VectorXd> sq() const {
    return s.head(dimv_);
  }

  Eigen::VectorBlock<Eigen::VectorXd> sv() {
    return s.tail(dimv_);
  }

  const Eigen::VectorBlock<const Eigen::VectorXd> sv() const {
    return s.tail(dimv_);
  }

  ///
  /// @brief Riccati factorization vector w.r.t. the switching time. Size is 
  /// 2 * Robot::dimv().
  ///
  Eigen::VectorXd Gmm;

  ///
  /// @brief Riccati factorization w.r.t. the switching time. 
  ///
  double xi;

  ///
  /// @brief Riccati factorization w.r.t. the switching time. 
  ///
  double eta;

  ///
  /// @brief Riccati factorization vector w.r.t. the switching time. Size is 
  /// 2 * Robot::dimv().
  ///
  Eigen::VectorXd Gmm_cvx;

  ///
  /// @brief Riccati factorization vector w.r.t. the switching time. Size is 
  /// 2 * Robot::dimv().
  ///
  Eigen::VectorXd hx_cvx;

  ///
  /// @brief Riccati factorization vector w.r.t. the switching time. Size is 
  /// Robot::dimu().
  ///
  Eigen::VectorXd hu_cvx;

  ///
  /// @brief Riccati factorization w.r.t. the switching time. 
  ///
  double xi_cvx;

  ///
  /// @brief Checks the equivalence of two SplitRiccatiFactorization.
  /// @param[in] other object.
  /// @return true if this and other is same. false otherwise.
  ///
  bool isApprox(const SplitRiccatiFactorization& other) const {
    if (!P.isApprox(other.P)) return false;
    if (!s.isApprox(other.s)) return false;
    if (!Gmm.isApprox(other.Gmm)) return false;
    if (!Gmm_cvx.isApprox(other.Gmm_cvx)) return false;
    if (!hx_cvx.isApprox(other.hx_cvx)) return false;
    if (!hu_cvx.isApprox(other.hu_cvx)) return false;
    Eigen::Vector3d vec, other_vec;
    vec << xi, eta, xi_cvx;
    other_vec << other.xi, other.eta, other.xi_cvx;
    if (!vec.isApprox(other_vec)) return false;
    return true;
  }

  ///
  /// @brief Checks this object has at least one NaN.
  /// @return true if this has at least one NaN. false otherwise.
  ///
  bool hasNaN() const {
    if (P.hasNaN()) return true;
    if (s.hasNaN()) return true;
    if (Gmm.hasNaN()) return true;
    if (Gmm_cvx.hasNaN()) return true;
    if (hx_cvx.hasNaN()) return true;
    if (hu_cvx.hasNaN()) return true;
    Eigen::Vector3d vec;
    vec << xi, eta, xi_cvx;
    if (vec.hasNaN()) return true;
    return false;
  }

private:
  int dimv_, dimx_;

};

} // namespace roboc 

#endif // ROBOC_SPLIT_RICCATI_FACTORIZATION_HPP_ 