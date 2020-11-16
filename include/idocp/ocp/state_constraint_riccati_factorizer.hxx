#ifndef IDOCP_STATE_CONSTRAINT_RICCATI_FACTORIZER_HXX_
#define IDOCP_STATE_CONSTRAINT_RICCATI_FACTORIZER_HXX_

#include "idocp/ocp/state_constraint_riccati_factorizer.hpp"

#include <omp.h>
#include <cassert>

namespace idocp {

inline StateConstraintRiccatiFactorizer::StateConstraintRiccatiFactorizer(
    const Robot& robot, const int max_num_impulse, const int nproc) 
  : llts_(max_num_impulse, Eigen::LLT<Eigen::MatrixXd>()),
    lp_factorizer_(max_num_impulse, StateConstraintRiccatiLPFactorizer(robot)),
    max_num_impulse_(max_num_impulse),
    dimv_(robot.dimv()),
    nproc_(nproc) { 
}


inline StateConstraintRiccatiFactorizer::StateConstraintRiccatiFactorizer() 
  : llts_(),
    lp_factorizer_(),
    max_num_impulse_(0),
    dimv_(0),
    nproc_(0) { 
}


inline StateConstraintRiccatiFactorizer::~StateConstraintRiccatiFactorizer() { 
}


template <typename VectorType>
inline void StateConstraintRiccatiFactorizer::computeLagrangeMultiplierDirection(
    const ContactSequence& contact_sequence,
    const std::vector<RiccatiFactorization>& impulse_riccati_factorization,
    std::vector<StateConstraintRiccatiFactorization>& constraint_factorization,
    const Eigen::MatrixBase<VectorType>& dx0,
    std::vector<ImpulseSplitDirection>& d_impulse) {
  assert(impulse_riccati_factorization.size() == max_num_impulse_);
  assert(constraint_factorization.size() == max_num_impulse_);
  assert(d_impulse.size() == max_num_impulse_);
  const int num_impulse = contact_sequence.totalNumImpulseStages();
  assert(num_impulse <= max_num_impulse_);
  #pragma omp parallel for num_threads(nproc_)
  for (int i=0; i<num_impulse; ++i) {
    lp_factorizer_[i].factorizeLinearProblem(impulse_riccati_factorization[i], 
                                             constraint_factorization[i], dx0);
    llts_[i].compute(constraint_factorization[i].ENEt());
    assert(llts_[i].info() == Eigen::Success);
  }
  for (int i=num_impulse-1; i>=0; --i) {
    for (int j=i+1; j<num_impulse; ++j) {
      constraint_factorization[i].e().noalias() 
          -= constraint_factorization[i].EN() 
              * constraint_factorization[j].T_impulse(i) * d_impulse[j].dxi();
    }
    d_impulse[i].dxi() = llts_[i].solve(constraint_factorization[i].e());
  }
}


inline void StateConstraintRiccatiFactorizer::aggregateLagrangeMultiplierDirection(
    const ContactSequence& contact_sequence,
    const std::vector<StateConstraintRiccatiFactorization>& constraint_factorization,
    const std::vector<ImpulseSplitDirection>& d_impulse, const int time_stage,
    RiccatiFactorization& riccati_factorization) {
  assert(time_stage >= 0);
  const int num_impulse = contact_sequence.totalNumImpulseStages();
  riccati_factorization.n.setZero();
  for (int i=num_impulse-1; i>=0; --i) {
    if (contact_sequence.timeStageBeforeImpulse(i) >= time_stage) {
      riccati_factorization.n.noalias() 
          += constraint_factorization[i].T(time_stage) * d_impulse[i].dxi();
    }
    else {
      break;
    }
  }
}


inline void StateConstraintRiccatiFactorizer::aggregateLagrangeMultiplierDirectionImpulse(
    const ContactSequence& contact_sequence,
    const std::vector<StateConstraintRiccatiFactorization>& constraint_factorization,
    const std::vector<ImpulseSplitDirection>& d_impulse, 
    const int impulse_index,
    RiccatiFactorization& riccati_factorization) {
  assert(impulse_index >= 0);
  assert(impulse_index < contact_sequence.totalNumImpulseStages());
  const int num_impulse = contact_sequence.totalNumImpulseStages();
  riccati_factorization.n.setZero();
  for (int i=num_impulse-1; i>=impulse_index; --i) {
    riccati_factorization.n.noalias() 
        += constraint_factorization[i].T_impulse(impulse_index) * d_impulse[i].dxi();
  }
}


inline void StateConstraintRiccatiFactorizer::aggregateLagrangeMultiplierDirectionLift(
    const ContactSequence& contact_sequence,
    const std::vector<StateConstraintRiccatiFactorization>& constraint_factorization,
    const std::vector<ImpulseSplitDirection>& d_impulse, 
    const int lift_index,
    RiccatiFactorization& riccati_factorization) {
  assert(lift_index >= 0);
  assert(lift_index < contact_sequence.totalNumLiftStages());
  const int num_impulse = contact_sequence.totalNumImpulseStages();
  const int time_stage_before_lift = contact_sequence.timeStageBeforeLift(lift_index);
  riccati_factorization.n.setZero();
  for (int i=num_impulse-1; i>=0; --i) {
    if (contact_sequence.timeStageBeforeImpulse(i) > time_stage_before_lift) {
      riccati_factorization.n.noalias() 
          += constraint_factorization[i].T_lift(lift_index) * d_impulse[i].dxi();
    }
    else {
      break;
    }
  }
}


} // namespace idocp

#endif // IDOCP_STATE_CONSTRAINT_RICCATI_FACTORIZER_HXX_ 