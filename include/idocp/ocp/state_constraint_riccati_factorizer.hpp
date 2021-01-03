#ifndef IDOCP_STATE_CONSTRAINT_RICCATI_FACTORIZER_HPP_
#define IDOCP_STATE_CONSTRAINT_RICCATI_FACTORIZER_HPP_

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/split_riccati_factorization.hpp"
#include "idocp/ocp/state_constraint_riccati_factorization.hpp"
#include "idocp/ocp/state_constraint_riccati_lp_factorizer.hpp"
#include "idocp/impulse/impulse_split_direction.hpp"
#include "idocp/hybrid/hybrid_container.hpp"
#include "idocp/hybrid/ocp_discretizer.hpp"

namespace idocp {

///
/// @class StateConstraintRiccatiFactorizer 
/// @brief State constriant factorizer for Riccati recursion.
///
class StateConstraintRiccatiFactorizer {
public:
  ///
  /// @brief Construct factorizer.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] N Number of discretization of the horizon. Must be more than 1. 
  /// @param[in] max_num_impulse Maximum number of impulse phases over the 
  /// horizon.
  /// @param[in] nthreads Number of threads used in this class.
  ///
  StateConstraintRiccatiFactorizer(const Robot& robot, const int N,
                                   const int max_num_impulse, 
                                   const int nthreads);

  ///
  /// @brief Default constructor. 
  ///
  StateConstraintRiccatiFactorizer();

  ///
  /// @brief Destructor. 
  ///
  ~StateConstraintRiccatiFactorizer();

  ///
  /// @brief Default copy constructor. 
  ///
  StateConstraintRiccatiFactorizer(
      const StateConstraintRiccatiFactorizer&) = default;

  ///
  /// @brief Default copy operator. 
  ///
  StateConstraintRiccatiFactorizer& operator=(
      const StateConstraintRiccatiFactorizer&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  StateConstraintRiccatiFactorizer(
      StateConstraintRiccatiFactorizer&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  StateConstraintRiccatiFactorizer& operator=(
      StateConstraintRiccatiFactorizer&&) noexcept = default;

  ///
  /// @brief Computes the directions of the Lagrange multipliers with respect
  /// to the pure-state constraints.
  /// @param[in] ocp_discretizer OCP discretizer.
  /// @param[in] riccati_factorization Riccati factorizations.
  /// @param[in, out] constraint_factorization Constraint factorizations.
  /// @param[in, out] d Split directions. Initial state direction d[0].dx() 
  /// must be computed before calling this function.
  ///
  void computeLagrangeMultiplierDirection(
      const OCPDiscretizer& ocp_discretizer,
      const RiccatiFactorization& riccati_factorization,
      StateConstraintRiccatiFactorization& constraint_factorization,
      Direction& d);

  ///
  /// @brief Aggregates the all of the Lagrange multipliers with respect to 
  /// the pure-state constriants for fowrad Riccati recursion.
  /// @param[in] constraint_factorization Pure-state constraint factorization. 
  /// @param[in] ocp_discretizer OCP discretizer.
  /// @param[in] d Split directions. 
  /// @param[in, out] riccati_factorization Riccati factorization. 
  ///
  void aggregateLagrangeMultiplierDirection(
      const StateConstraintRiccatiFactorization& constraint_factorization,
      const OCPDiscretizer& ocp_discretizer, const Direction& d,
      RiccatiFactorization& riccati_factorization) const;

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  int N_, nthreads_;
  Eigen::LDLT<Eigen::MatrixXd> ldlt_;
  std::vector<StateConstraintRiccatiLPFactorizer> lp_factorizer_;

};

} // namespace idocp

#endif // IDOCP_STATE_CONSTRAINT_RICCATI_FACTORIZATER_HPP_ 