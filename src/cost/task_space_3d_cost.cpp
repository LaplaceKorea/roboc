#include "idocp/cost/task_space_3d_cost.hpp"

#include <iostream>


namespace idocp {

TaskSpace3DCost::TaskSpace3DCost(const Robot& robot, const int frame_id)
  : CostFunctionComponentBase(),
    frame_id_(frame_id),
    q_3d_ref_(Eigen::Vector3d::Zero()),
    q_3d_weight_(Eigen::Vector3d::Zero()),
    qf_3d_weight_(Eigen::Vector3d::Zero()) {
}


TaskSpace3DCost::TaskSpace3DCost()
  : CostFunctionComponentBase(),
    frame_id_(),
    q_3d_ref_(),
    q_3d_weight_(),
    qf_3d_weight_() {
}


TaskSpace3DCost::~TaskSpace3DCost() {
}


bool TaskSpace3DCost::useKinematics() const {
  return true;
}


void TaskSpace3DCost::set_q_3d_ref(const Eigen::Vector3d& q_3d_ref) {
  q_3d_ref_ = q_3d_ref;
}


void TaskSpace3DCost::set_q_3d_weight(const Eigen::Vector3d& q_3d_weight) {
  q_3d_weight_ = q_3d_weight;
}


void TaskSpace3DCost::set_qf_3d_weight(const Eigen::Vector3d& qf_3d_weight) {
  qf_3d_weight_ = qf_3d_weight;
}


double TaskSpace3DCost::l(Robot& robot, CostFunctionData& data, 
                          const double t, const double dtau, 
                          const SplitSolution& s) const {
  double l = 0;
  data.diff_3d = robot.framePosition(frame_id_) - q_3d_ref_;
  l += (q_3d_weight_.array()*data.diff_3d.array()*data.diff_3d.array()).sum();
  return 0.5 * dtau * l;
}


double TaskSpace3DCost::phi(Robot& robot, CostFunctionData& data, 
                            const double t, const SplitSolution& s) const {
  double phi = 0;
  data.diff_3d = robot.framePosition(frame_id_) - q_3d_ref_;
  phi += (qf_3d_weight_.array()*data.diff_3d.array()*data.diff_3d.array()).sum();
  return 0.5 * phi;
}


void TaskSpace3DCost::lq(Robot& robot, CostFunctionData& data, 
                         const double t, const double dtau, 
                         const SplitSolution& s, 
                         SplitKKTResidual& kkt_residual) const {
  data.diff_3d = robot.framePosition(frame_id_) - q_3d_ref_;
  robot.getFrameJacobian(frame_id_, data.J_6d);
  data.J_3d.noalias() 
      = robot.frameRotation(frame_id_) * data.J_6d.template topRows<3>();
  kkt_residual.lq().noalias() 
      += dtau * data.J_3d.transpose() * q_3d_weight_.asDiagonal() * data.diff_3d;
}


void TaskSpace3DCost::lqq(Robot& robot, CostFunctionData& data, 
                          const double t, const double dtau, 
                          const SplitSolution& s, 
                          SplitKKTMatrix& kkt_matrix) const {
  robot.getFrameJacobian(frame_id_, data.J_6d);
  data.J_3d.noalias() 
      = robot.frameRotation(frame_id_) * data.J_6d.template topRows<3>();
  kkt_matrix.Qqq().noalias()
      += dtau * data.J_3d.transpose() * q_3d_weight_.asDiagonal() * data.J_3d;
}


void TaskSpace3DCost::phiq(Robot& robot, CostFunctionData& data, 
                           const double t, const SplitSolution& s,
                           SplitKKTResidual& kkt_residual) const {
  data.diff_3d = robot.framePosition(frame_id_) - q_3d_ref_;
  robot.getFrameJacobian(frame_id_, data.J_6d);
  data.J_3d.noalias() 
      = robot.frameRotation(frame_id_) * data.J_6d.template topRows<3>();
  kkt_residual.lq().noalias() 
      += data.J_3d.transpose() * qf_3d_weight_.asDiagonal() * data.diff_3d;
}


void TaskSpace3DCost::phiqq(Robot& robot, CostFunctionData& data, 
                            const double t, const SplitSolution& s,
                            SplitKKTMatrix& kkt_matrix) const {
    robot.getFrameJacobian(frame_id_, data.J_6d);
    data.J_3d.noalias() 
        = robot.frameRotation(frame_id_) * data.J_6d.template topRows<3>();
    kkt_matrix.Qqq().noalias()
        += data.J_3d.transpose() * qf_3d_weight_.asDiagonal() * data.J_3d;
}

} // namespace idocp