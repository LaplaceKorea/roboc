#ifndef IDOCP_FORWARD_RICCATI_RECURSION_FACTORIZER_HPP_ 
#define IDOCP_FORWARD_RICCATI_RECURSION_FACTORIZER_HPP_

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/riccati_factorization.hpp"
#include "idocp/ocp/lqr_state_feedback_policy.hpp"


namespace idocp {

///
/// @class ForwardRiccatiRecursionFactorizer
/// @brief Factorizer of the forward Riccati recursion for SplitOCP.
///
class ForwardRiccatiRecursionFactorizer {
public:
  ///
  /// @brief Construct factorizer.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  ///
  ForwardRiccatiRecursionFactorizer(const Robot& robot);

  ///
  /// @brief Default constructor. 
  ///
  ForwardRiccatiRecursionFactorizer();

  ///
  /// @brief Destructor. 
  ///
  ~ForwardRiccatiRecursionFactorizer();

  ///
  /// @brief Default copy constructor. 
  ///
  ForwardRiccatiRecursionFactorizer(
      const ForwardRiccatiRecursionFactorizer&) = default;

  ///
  /// @brief Default copy operator. 
  ///
  ForwardRiccatiRecursionFactorizer& operator=(
      const ForwardRiccatiRecursionFactorizer&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  ForwardRiccatiRecursionFactorizer(
      ForwardRiccatiRecursionFactorizer&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  ForwardRiccatiRecursionFactorizer& operator=(
      ForwardRiccatiRecursionFactorizer&&) noexcept = default;

  ///
  /// @brief Factorize the state transition matrix and vector in terms of the 
  /// initial state.  
  /// @param[in] riccati Riccati factorization at the current time stage.
  /// @param[in] kkt_matrix The KKT matrix.
  /// @param[in] kkt_residual The KKT residual.
  /// @param[in] dtau Time step between the current time stage and the next 
  /// @param[out] riccati_next Riccati factorization at the next time stage.
  ///
  void factorizeStateTransition(const RiccatiFactorization& riccati, 
                                const KKTMatrix& kkt_matrix, 
                                const KKTResidual& kkt_residual, 
                                const double dtau,
                                RiccatiFactorization& riccati_next);

  ///
  /// @brief Factorize the pure-state constraint factorization matrix.  
  /// @param[in] riccati Riccati factorization at the current time stage.
  /// @param[in] kkt_matrix The KKT matrix.
  /// @param[in] dtau Time step between the current time stage and the next 
  /// @param[out] riccati_next Riccati factorization at the next time stage.
  ///
  void factorizeStateConstraintFactorization(
      const RiccatiFactorization& riccati, const KKTMatrix& kkt_matrix, 
      const double dtau, RiccatiFactorization& riccati_next);

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  bool has_floating_base_;
  int dimv_;
  static constexpr int kDimFloatingBase = 6;
  Eigen::MatrixXd NApBKt_;

};

} // namespace idocp

#include "idocp/ocp/forward_riccati_recursion_factorizer.hxx"

#endif // IDOCP_FORWARD_RICCATI_RECURSION_FACTORIZER_HPP_ 