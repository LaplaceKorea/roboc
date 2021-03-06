#include "idocp/cost/time_varying_configuration_space_cost.hpp"

#include <iostream>
#include <stdexcept>


namespace idocp {

TimeVaryingConfigurationSpaceCost::TimeVaryingConfigurationSpaceCost(
    const Robot& robot,
    const std::shared_ptr<TimeVaryingConfigurationRefBase>& ref) 
  : dimq_(robot.dimq()),
    dimv_(robot.dimv()),
    ref_(ref),
    q_weight_(Eigen::VectorXd::Zero(robot.dimv())),
    qf_weight_(Eigen::VectorXd::Zero(robot.dimv())),
    qi_weight_(Eigen::VectorXd::Zero(robot.dimv())) {
}


TimeVaryingConfigurationSpaceCost::TimeVaryingConfigurationSpaceCost()
  : dimq_(0),
    dimv_(0),
    ref_(),
    q_weight_(),
    qf_weight_(),
    qi_weight_() {
}


TimeVaryingConfigurationSpaceCost::~TimeVaryingConfigurationSpaceCost() {
}


void TimeVaryingConfigurationSpaceCost::set_ref(
    const std::shared_ptr<TimeVaryingConfigurationRefBase>& ref) {
  ref_ = ref;
}


void TimeVaryingConfigurationSpaceCost::set_q_weight(
    const Eigen::VectorXd& q_weight) {
  try {
    if (q_weight.size() != dimv_) {
      throw std::invalid_argument(
          "invalid size: q_weight.size() must be " + std::to_string(dimv_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  q_weight_ = q_weight;
}


void TimeVaryingConfigurationSpaceCost::set_qf_weight(
    const Eigen::VectorXd& qf_weight) {
  try {
    if (qf_weight.size() != dimv_) {
      throw std::invalid_argument(
          "invalid size: qf_weight.size() must be " + std::to_string(dimv_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  qf_weight_ = qf_weight;
}


void TimeVaryingConfigurationSpaceCost::set_qi_weight(
    const Eigen::VectorXd& qi_weight) {
  try {
    if (qi_weight.size() != dimv_) {
      throw std::invalid_argument(
          "invalid size: qi_weight.size() must be " + std::to_string(dimv_) + "!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  qi_weight_ = qi_weight;
}


bool TimeVaryingConfigurationSpaceCost::useKinematics() const {
  return false;
}


double TimeVaryingConfigurationSpaceCost::computeStageCost(
    Robot& robot, CostFunctionData& data, const double t, const double dt, 
    const SplitSolution& s) const {
  if (ref_->isActive(t)) {
    ref_->update_q_ref(robot, t, data.q_ref);
    double l = 0;
    robot.subtractConfiguration(s.q, data.q_ref, data.qdiff);
    l += (q_weight_.array()*data.qdiff.array()*data.qdiff.array()).sum();
    return 0.5 * dt * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingConfigurationSpaceCost::computeStageCostDerivatives(
    Robot& robot, CostFunctionData& data, const double t, const double dt, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  if (ref_->isActive(t)) {
    ref_->update_q_ref(robot, t, data.q_ref);
    if (robot.hasFloatingBase()) {
      robot.dSubtractConfiguration_dqf(s.q, data.q_ref, data.J_qdiff);
      kkt_residual.lq().noalias()
          += dt * data.J_qdiff.transpose() * q_weight_.asDiagonal() * data.qdiff;
    }
    else {
      kkt_residual.lq().array() += dt * q_weight_.array() * data.qdiff.array();
    }
  }
}


void TimeVaryingConfigurationSpaceCost::computeStageCostHessian(
    Robot& robot, CostFunctionData& data, const double t, const double dt, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix) const {
  if (ref_->isActive(t)) {
    if (robot.hasFloatingBase()) {
      kkt_matrix.Qqq().noalias()
          += dt * data.J_qdiff.transpose() * q_weight_.asDiagonal() * data.J_qdiff;
    }
    else {
      kkt_matrix.Qqq().diagonal().noalias() += dt * q_weight_;
    }
  }
}


double TimeVaryingConfigurationSpaceCost::computeTerminalCost(
    Robot& robot, CostFunctionData& data, const double t, 
    const SplitSolution& s) const {
  if (ref_->isActive(t)) {
    ref_->update_q_ref(robot, t, data.q_ref);
    double l = 0;
    robot.subtractConfiguration(s.q, data.q_ref, data.qdiff);
    l += (qf_weight_.array()*data.qdiff.array()*data.qdiff.array()).sum();
    return 0.5 * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingConfigurationSpaceCost::computeTerminalCostDerivatives(
    Robot& robot, CostFunctionData& data, const double t, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  if (ref_->isActive(t)) {
    if (robot.hasFloatingBase()) {
      robot.dSubtractConfiguration_dqf(s.q, data.q_ref, data.J_qdiff);
      kkt_residual.lq().noalias()
          += data.J_qdiff.transpose() * qf_weight_.asDiagonal() * data.qdiff;
    }
    else {
      kkt_residual.lq().array() += qf_weight_.array() * data.qdiff.array();
    }
  }
}


void TimeVaryingConfigurationSpaceCost::computeTerminalCostHessian(
    Robot& robot, CostFunctionData& data, const double t, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix) const {
  if (ref_->isActive(t)) {
    if (robot.hasFloatingBase()) {
      kkt_matrix.Qqq().noalias()
          += data.J_qdiff.transpose() * qf_weight_.asDiagonal() * data.J_qdiff;
    }
    else {
      kkt_matrix.Qqq().diagonal().noalias() += qf_weight_;
    }
  }
}


double TimeVaryingConfigurationSpaceCost::computeImpulseCost(
    Robot& robot, CostFunctionData& data, const double t, 
    const ImpulseSplitSolution& s) const {
  if (ref_->isActive(t)) {
    ref_->update_q_ref(robot, t, data.q_ref);
    double l = 0;
    robot.subtractConfiguration(s.q, data.q_ref, data.qdiff);
    l += (qi_weight_.array()*data.qdiff.array()*data.qdiff.array()).sum();
    return 0.5 * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingConfigurationSpaceCost::computeImpulseCostDerivatives(
    Robot& robot, CostFunctionData& data, const double t, 
    const ImpulseSplitSolution& s, 
    ImpulseSplitKKTResidual& kkt_residual) const {
  if (ref_->isActive(t)) {
    if (robot.hasFloatingBase()) {
      robot.dSubtractConfiguration_dqf(s.q, data.q_ref, data.J_qdiff);
      kkt_residual.lq().noalias()
          += data.J_qdiff.transpose() * qi_weight_.asDiagonal() * data.qdiff;
    }
    else {
      kkt_residual.lq().array() += qi_weight_.array() * data.qdiff.array();
    }
  }
}


void TimeVaryingConfigurationSpaceCost::computeImpulseCostHessian(
    Robot& robot, CostFunctionData& data, const double t, 
    const ImpulseSplitSolution& s, ImpulseSplitKKTMatrix& kkt_matrix) const {
  if (ref_->isActive(t)) {
    if (robot.hasFloatingBase()) {
      kkt_matrix.Qqq().noalias()
          += data.J_qdiff.transpose() * qi_weight_.asDiagonal() * data.J_qdiff;
    }
    else {
      kkt_matrix.Qqq().diagonal().noalias() += qi_weight_;
    }
  }
}

} // namespace idocp