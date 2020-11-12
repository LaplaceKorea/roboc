#ifndef IDOCP_RICCATI_FACTORIZER_HPP_
#define IDOCP_RICCATI_FACTORIZER_HPP_

#include "Eigen/Core"
#include "Eigen/LU"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/riccati_factorization.hpp"


namespace idocp {

///
/// @class RiccatiFactorizer
/// @brief Riccati factorizer for SplitOCP.
///
class RiccatiFactorizer {
public:
  ///
  /// @brief Construct factorizer.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  ///
  RiccatiFactorizer(const Robot& robot);

  ///
  /// @brief Default constructor. 
  ///
  RiccatiFactorizer();

  ///
  /// @brief Destructor. 
  ///
  ~RiccatiFactorizer();
 
  ///
  /// @brief Default copy constructor. 
  ///
  RiccatiFactorizer(const RiccatiFactorizer&) = default;

  ///
  /// @brief Default copy operator. 
  ///
  RiccatiFactorizer& operator=(const RiccatiFactorizer&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  RiccatiFactorizer(RiccatiFactorizer&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  RiccatiFactorizer& operator=(RiccatiFactorizer&&) noexcept = default;

  void backwardRiccatiRecursion(const RiccatiFactorization& riccati_next, 
                                KKTMatrix& kkt_matrix, 
                                KKTResidual& kkt_residual, const double dtau, 
                                RiccatiFactorization& riccati);

  void forwardRiccatiRecursionParallel(KKTMatrix& kkt_matrix, 
                                       KKTResidual& kkt_residual, 
                                       RiccatiFactorization& riccati);

  static void forwardRiccatiRecursionSerialInitial(
      const KKTMatrix& kkt_matrix, const KKTResidual& kkt_residual, 
      RiccatiFactorization& riccati);

  void forwardRiccatiRecursionSerial(const RiccatiFactorization& riccati, 
                                     const KKTMatrix& kkt_matrix, 
                                     const KKTResidual& kkt_residual, 
                                     RiccatiFactorization& riccati_next);

  template <typename SplitDirectionType>
  void forwardRiccatiRecursion(const KKTMatrix& kkt_matrix, 
                               const KKTResidual& kkt_residual,
                               const SplitDirection& d, const double dtau,
                               SplitDirectionType& d_next) const;

  template <typename VectorType>
  void computeStateDirection(const RiccatiFactorization& riccati, 
                             SplitDirection& d, 
                             const Eigen::MatrixBase<VectorType>& dx0);

  void computeCostateDirection(const RiccatiFactorization& riccati, 
                               SplitDirection& d);

  void computeControlInputDirection(const RiccatiFactorization& riccati, 
                                    SplitDirection& d) const;

  template <typename MatrixType>
  void getStateFeedbackGain(const Eigen::MatrixBase<MatrixType>& K);

  template <typename MatrixType1, typename MatrixType2>
  void getStateFeedbackGain(const Eigen::MatrixBase<MatrixType1>& Kq,
                            const Eigen::MatrixBase<MatrixType2>& Kv);

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  bool has_floating_base_, has_state_constraint_;
  int dimv_, dimu_;
  static constexpr int kDimFloatingBase = 6;
  Eigen::LLT<Eigen::MatrixXd> llt_;
  Eigen::MatrixXd K_; // State feedback gain of the LQR subproblem
  Eigen::VectorXd k_; // Feedforward term of the LQR subproblem
  Eigen::MatrixXd AtPqq_, AtPqv_, AtPvq_, AtPvv_, BtPq_, BtPv_, GK_, 
                  GinvBt_, BGinvBt_, NApBKt_;

  void factorizeKKTMatrix(const RiccatiFactorization& riccati_next, 
                          const double dtau, KKTMatrix& kkt_matrix,  
                          KKTResidual& kkt_residual);

  void computeFeedbackGainAndFeedforward(const KKTMatrix& kkt_matrix, 
                                         const KKTResidual& kkt_residual,
                                         RiccatiFactorization& riccati);

  void factorizeRiccatiFactorization(const RiccatiFactorization& riccati_next, 
                                     const KKTMatrix& kkt_matrix, 
                                     const KKTResidual& kkt_residual,
                                     const double dtau, 
                                     RiccatiFactorization& riccati);

};

} // namespace idocp

#include "idocp/ocp/riccati_factorizer.hxx"

#endif // IDOCP_RICCATI_FACTORIZER_HPP_