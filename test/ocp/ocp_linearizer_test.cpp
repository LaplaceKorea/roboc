#include <string>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/cost/cost_function.hpp"
#include "idocp/cost/joint_space_cost.hpp"
#include "idocp/cost/contact_force_cost.hpp"
#include "idocp/cost/joint_space_impulse_cost.hpp"
#include "idocp/cost/impulse_force_cost.hpp"
#include "idocp/constraints/constraints.hpp"
#include "idocp/constraints/joint_position_lower_limit.hpp"
#include "idocp/constraints/joint_position_upper_limit.hpp"
#include "idocp/constraints/joint_velocity_lower_limit.hpp"
#include "idocp/constraints/joint_velocity_upper_limit.hpp"
#include "idocp/constraints/joint_torques_lower_limit.hpp"
#include "idocp/constraints/joint_torques_upper_limit.hpp"
#include "idocp/ocp/split_ocp.hpp"
#include "idocp/impulse/split_impulse_ocp.hpp"
#include "idocp/hybrid/hybrid_container.hpp"
#include "idocp/hybrid/contact_sequence.hpp"
#include "idocp/ocp/ocp_linearizer.hpp"


namespace idocp {

class OCPLinearizerTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
    N = 20;
    max_num_impulse = 5;
    nproc = 4;
    T = 1;
    t = std::abs(Eigen::VectorXd::Random(1)[0]);
    dtau = T / N;
  }

  virtual void TearDown() {
  }

  using HybridOCP = hybrid_container<SplitOCP, SplitImpulseOCP>;
  using HybridSolution = hybrid_container<SplitSolution, ImpulseSplitSolution>;
  using HybridDirection = hybrid_container<SplitDirection, ImpulseSplitDirection>;
  using HybridKKTMatrix = hybrid_container<KKTMatrix, ImpulseKKTMatrix>;
  using HybridKKTResidual = hybrid_container<KKTResidual, ImpulseKKTResidual>;
  // using HybridRiccatiFactorization = hybrid_container<RiccatiFactorization, RiccatiFactorization>;

  static std::shared_ptr<CostFunction> createCost(const Robot& robot);
  static std::shared_ptr<Constraints> createConstraints(const Robot& robot);
  HybridSolution createSolution(const Robot& robot) const;
  HybridSolution createSolution(const Robot& robot, const ContactSequence& contact_sequence) const;
  ContactSequence createContactSequence(const Robot& robot) const;

  void testLinearizeOCP(const Robot& robot) const;
  void testComputeKKTResidual(const Robot& robot) const;

  std::string fixed_base_urdf, floating_base_urdf;
  int N, max_num_impulse, nproc;
  double T, t, dtau;
};


std::shared_ptr<CostFunction> OCPLinearizerTest::createCost(const Robot& robot) {
  auto joint_cost = std::make_shared<JointSpaceCost>(robot);
  const Eigen::VectorXd q_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  Eigen::VectorXd q_ref = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_ref);
  const Eigen::VectorXd v_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd a_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  const Eigen::VectorXd a_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd u_weight = Eigen::VectorXd::Random(robot.dimu()).array().abs();
  const Eigen::VectorXd u_ref = Eigen::VectorXd::Random(robot.dimu());
  const Eigen::VectorXd qf_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  const Eigen::VectorXd vf_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  joint_cost->set_q_weight(q_weight);
  joint_cost->set_q_ref(q_ref);
  joint_cost->set_v_weight(v_weight);
  joint_cost->set_v_ref(v_ref);
  joint_cost->set_a_weight(a_weight);
  joint_cost->set_a_ref(a_ref);
  joint_cost->set_u_weight(u_weight);
  joint_cost->set_u_ref(u_ref);
  joint_cost->set_qf_weight(qf_weight);
  joint_cost->set_vf_weight(vf_weight);
  const int task_frame = 10;
  auto contact_force_cost = std::make_shared<idocp::ContactForceCost>(robot);
  std::vector<Eigen::Vector3d> f_weight;
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    f_weight.push_back(Eigen::Vector3d::Constant(0.001));
  }
  contact_force_cost->set_f_weight(f_weight);
  auto cost = std::make_shared<CostFunction>();
  auto impulse_joint_cost = std::make_shared<JointSpaceImpulseCost>(robot);
  impulse_joint_cost->set_q_weight(q_weight);
  impulse_joint_cost->set_q_ref(q_ref);
  impulse_joint_cost->set_v_weight(v_weight);
  impulse_joint_cost->set_v_ref(v_ref);
  impulse_joint_cost->set_dv_weight(a_weight);
  impulse_joint_cost->set_dv_ref(a_ref);
  auto impulse_force_cost = std::make_shared<idocp::ImpulseForceCost>(robot);
  impulse_force_cost->set_f_weight(f_weight);
  cost->push_back(joint_cost);
  cost->push_back(contact_force_cost);
  cost->push_back(impulse_joint_cost);
  cost->push_back(impulse_force_cost);
  return cost;
}


std::shared_ptr<Constraints> OCPLinearizerTest::createConstraints(const Robot& robot) {
  auto joint_lower_limit = std::make_shared<JointPositionLowerLimit>(robot);
  auto joint_upper_limit = std::make_shared<JointPositionUpperLimit>(robot);
  auto velocity_lower_limit = std::make_shared<JointVelocityLowerLimit>(robot);
  auto velocity_upper_limit = std::make_shared<JointVelocityUpperLimit>(robot);
  auto torques_lower_limit = std::make_shared<JointTorquesLowerLimit>(robot);
  auto torques_upper_limit = std::make_shared<JointTorquesUpperLimit>(robot);
  auto constraints = std::make_shared<Constraints>();
  constraints->push_back(joint_upper_limit); 
  constraints->push_back(joint_lower_limit);
  constraints->push_back(velocity_lower_limit); 
  constraints->push_back(velocity_upper_limit);
  constraints->push_back(torques_lower_limit);
  constraints->push_back(torques_upper_limit);
  return constraints;
}


OCPLinearizerTest::HybridSolution OCPLinearizerTest::createSolution(const Robot& robot) const {
  HybridSolution s(N+1, SplitSolution(robot), max_num_impulse, ImpulseSplitSolution(robot));
  for (int i=0; i<=N; ++i) {
    s[i].setRandom(robot);
  }
  return s;
}


OCPLinearizerTest::HybridSolution OCPLinearizerTest::createSolution(const Robot& robot, const ContactSequence& contact_sequence) const {
  if (robot.max_point_contacts() == 0) {
    return createSolution(robot);
  }
  else {
    HybridSolution s(N+1, SplitSolution(robot), max_num_impulse, ImpulseSplitSolution(robot));
    for (int i=0; i<=N; ++i) {
      s[i].setRandom(robot, contact_sequence.contactStatus(i));
    }
    const int num_impulse = contact_sequence.totalNumImpulseStages();
    for (int i=0; i<num_impulse; ++i) {
      s.impulse[i].setRandom(robot, contact_sequence.impulseStatus(i));
    }
    for (int i=0; i<num_impulse; ++i) {
      const int time_stage = contact_sequence.timeStageBeforeImpulse(i);
      s.aux[i].setRandom(robot, contact_sequence.contactStatus(time_stage+1));
    }
    const int num_lift = contact_sequence.totalNumLiftStages();
    for (int i=0; i<num_lift; ++i) {
      const int time_stage = contact_sequence.timeStageBeforeLift(i);
      s.lift[i].setRandom(robot, contact_sequence.contactStatus(time_stage+1));
    }
    return s;
  }
}


ContactSequence OCPLinearizerTest::createContactSequence(const Robot& robot) const {
  std::vector<DiscreteEvent> discrete_events;
  ContactStatus pre_contact_status = robot.createContactStatus();
  pre_contact_status.setRandom();
  ContactSequence contact_sequence(robot, T, N);
  contact_sequence.setContactStatusUniformly(pre_contact_status);
  ContactStatus post_contact_status = pre_contact_status;
  std::random_device rnd;
  for (int i=0; i<max_num_impulse; ++i) {
    DiscreteEvent tmp(robot);
    tmp.setDiscreteEvent(pre_contact_status, post_contact_status);
    while (!tmp.existDiscreteEvent()) {
      post_contact_status.setRandom();
      tmp.setDiscreteEvent(pre_contact_status, post_contact_status);
    }
    tmp.eventTime = i * 0.15 + 0.01 * std::abs(Eigen::VectorXd::Random(1)[0]);
    discrete_events.push_back(tmp);
    pre_contact_status = post_contact_status;
  }
  for (int i=0; i<max_num_impulse; ++i) {
    contact_sequence.setDiscreteEvent(discrete_events[i]);
  }
  return contact_sequence;
}


void OCPLinearizerTest::testLinearizeOCP(const Robot& robot) const {
  auto cost = createCost(robot);
  auto constraints = createConstraints(robot);
  OCPLinearizer linearizer(T, N, max_num_impulse, nproc);
  ContactSequence contact_sequence(robot, T, N);
  if (robot.max_point_contacts() > 0) {
    contact_sequence = createContactSequence(robot);
  }
  auto kkt_matrix = HybridKKTMatrix(N+1, KKTMatrix(robot), max_num_impulse, ImpulseKKTMatrix(robot));
  auto kkt_residual = HybridKKTResidual(N+1, KKTResidual(robot), max_num_impulse, ImpulseKKTResidual(robot));
  HybridSolution s;
  if (robot.max_point_contacts() > 0) {
    s = createSolution(robot, contact_sequence);
  }
  else {
    s = createSolution(robot);
  }
  Eigen::VectorXd q(robot.dimq());
  robot.generateFeasibleConfiguration(q);
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  auto kkt_matrix_ref = kkt_matrix;
  auto kkt_residual_ref = kkt_residual;
  std::vector<Robot> robots(nproc, robot);
  auto split_ocps = HybridOCP(N, SplitOCP(robot, cost, constraints), 
                              max_num_impulse, SplitImpulseOCP(robot, 
                                                               cost->getImpulseCostFunction(), 
                                                               constraints->getImpulseConstraints()));
  auto terminal_ocp = TerminalOCP(robot, cost, constraints);
  linearizer.linearizeOCP(split_ocps, terminal_ocp, robots, contact_sequence, t, q, v, s, kkt_matrix, kkt_residual);
  auto terminal_ocp_ref = TerminalOCP(robot, cost, constraints);
  auto split_ocps_ref = HybridOCP(N, SplitOCP(robot, cost, constraints), 
                                  max_num_impulse, SplitImpulseOCP(robot, 
                                                                  cost->getImpulseCostFunction(), 
                                                                  constraints->getImpulseConstraints()));
  auto robot_ref = robot;
  if (contact_sequence.existImpulseStage(0)) {
    const double dtau_impulse = contact_sequence.impulseTime(0);
    const double dtau_aux = dtau - dtau_impulse;
    ASSERT_TRUE(dtau_impulse > 0);
    ASSERT_TRUE(dtau_impulse < dtau);
    split_ocps_ref[0].linearizeOCP(
        robot_ref, contact_sequence.contactStatus(0), t, dtau_impulse, 
        q, s[0], s.impulse[0], kkt_matrix_ref[0], kkt_residual_ref[0]);
    split_ocps_ref.impulse[0].linearizeOCP(
        robot_ref, contact_sequence.impulseStatus(0), t+dtau_impulse, 
        s[0].q, s.impulse[0], s.aux[0], kkt_matrix_ref.impulse[0], kkt_residual_ref.impulse[0]);
    split_ocps_ref.aux[0].linearizeOCP(
        robot_ref, contact_sequence.contactStatus(1), t+dtau_impulse, dtau_aux, 
        s.impulse[0].q, s.aux[0], s[1], kkt_matrix_ref.aux[0], kkt_residual_ref.aux[0]);
  }
  else if (contact_sequence.existLiftStage(0)) {
    const double dtau_lift = contact_sequence.liftTime(0);
    const double dtau_aux = dtau - dtau_lift;
    ASSERT_TRUE(dtau_lift > 0);
    ASSERT_TRUE(dtau_lift < dtau);
    split_ocps_ref[0].linearizeOCP(
        robot_ref, contact_sequence.contactStatus(0), t, dtau_lift, 
        q, s[0], s.lift[0], kkt_matrix_ref[0], kkt_residual_ref[0]);
    split_ocps_ref.lift[0].linearizeOCP(
        robot_ref, contact_sequence.contactStatus(1), t+dtau_lift, dtau_aux, 
        s[0].q, s.lift[0], s[1], kkt_matrix_ref.lift[0], kkt_residual_ref.lift[0]);
  }
  else {
    split_ocps_ref[0].linearizeOCP(
        robot_ref, contact_sequence.contactStatus(0), t, dtau, 
        q, s[0], s[1], kkt_matrix_ref[0], kkt_residual_ref[0]);
  }
  for (int i=1; i<N; ++i) {
    Eigen::VectorXd q_prev;
    if (contact_sequence.existImpulseStage(i-1)) {
      q_prev = s.aux[contact_sequence.impulseIndex(i-1)].q;
    }
    else if (contact_sequence.existLiftStage(i-1)) {
      q_prev = s.lift[contact_sequence.liftIndex(i-1)].q;
    }
    else {
      q_prev = s[i-1].q;
    }
    if (contact_sequence.existImpulseStage(i)) {
      const int impulse_index = contact_sequence.impulseIndex(i);
      const double impulse_time = contact_sequence.impulseTime(impulse_index);
      const double dtau_impulse = impulse_time - i * dtau;
      const double dtau_aux = dtau - dtau_impulse;
      ASSERT_TRUE(dtau_impulse > 0);
      ASSERT_TRUE(dtau_aux > 0);
      split_ocps_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(i), 
          t+i*dtau, dtau_impulse, q_prev, s[i], s.impulse[impulse_index], 
          kkt_matrix_ref[i], kkt_residual_ref[i]);
      split_ocps_ref.impulse[impulse_index].linearizeOCP(
          robot_ref, contact_sequence.impulseStatus(impulse_index), t+impulse_time, 
          s[i].q, s.impulse[impulse_index], s.aux[impulse_index], 
          kkt_matrix_ref.impulse[impulse_index], kkt_residual_ref.impulse[impulse_index]);
      split_ocps_ref.aux[impulse_index].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(i+1), t+impulse_time, dtau_aux, 
          s.impulse[impulse_index].q, s.aux[impulse_index], s[i+1], 
          kkt_matrix_ref.aux[impulse_index], kkt_residual_ref.aux[impulse_index]);
    }
    else if (contact_sequence.existLiftStage(i)) {
      const int lift_index = contact_sequence.liftIndex(i);
      const double lift_time = contact_sequence.liftTime(lift_index);
      const double dtau_lift = lift_time - i * dtau;
      const double dtau_aux = dtau - dtau_lift;
      ASSERT_TRUE(dtau_lift > 0);
      ASSERT_TRUE(dtau_aux > 0);
      split_ocps_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(i), t+i*dtau, dtau_lift, 
          q_prev, s[i], s.lift[lift_index], kkt_matrix_ref[i], kkt_residual_ref[i]);
      split_ocps_ref.lift[lift_index].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(i+1), t+lift_time, dtau_aux, 
          s[i].q, s.lift[lift_index], s[i+1], kkt_matrix_ref.lift[lift_index], kkt_residual_ref.lift[lift_index]);
    }
    else {
      split_ocps_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(i), t+i*dtau, dtau, 
          q_prev, s[i], s[i+1], kkt_matrix_ref[i], kkt_residual_ref[i]);
    }
  }
  terminal_ocp_ref.linearizeOCP(robot_ref, t+T, s[N], kkt_matrix_ref[N], kkt_residual_ref[N]);
  for (int i=0; i<=N; ++i) {
    kkt_matrix[i].isApprox(kkt_matrix_ref[i]);
    kkt_residual[i].isApprox(kkt_residual_ref[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_matrix.aux[i].isApprox(kkt_matrix_ref.aux[i]);
    kkt_residual.aux[i].isApprox(kkt_residual_ref.aux[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_matrix.impulse[i].isApprox(kkt_matrix_ref.impulse[i]);
    kkt_residual.impulse[i].isApprox(kkt_residual_ref.impulse[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_matrix.lift[i].isApprox(kkt_matrix_ref.lift[i]);
    kkt_residual.lift[i].isApprox(kkt_residual_ref.lift[i]);
  }
}


void OCPLinearizerTest::testComputeKKTResidual(const Robot& robot) const {
  auto cost = createCost(robot);
  auto constraints = createConstraints(robot);
  OCPLinearizer linearizer(T, N, max_num_impulse, nproc);
  ContactSequence contact_sequence(robot, T, N);
  if (robot.max_point_contacts() > 0) {
    contact_sequence = createContactSequence(robot);
  }
  auto kkt_matrix = HybridKKTMatrix(N+1, KKTMatrix(robot), max_num_impulse, ImpulseKKTMatrix(robot));
  auto kkt_residual = HybridKKTResidual(N+1, KKTResidual(robot), max_num_impulse, ImpulseKKTResidual(robot));
  HybridSolution s;
  if (robot.max_point_contacts() > 0) {
    s = createSolution(robot, contact_sequence);
  }
  else {
    s = createSolution(robot);
  }
  Eigen::VectorXd q(robot.dimq());
  robot.generateFeasibleConfiguration(q);
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  auto kkt_matrix_ref = kkt_matrix;
  auto kkt_residual_ref = kkt_residual;
  std::vector<Robot> robots(nproc, robot);
  auto split_ocps = HybridOCP(N, SplitOCP(robot, cost, constraints), 
                              max_num_impulse, SplitImpulseOCP(robot, 
                                                               cost->getImpulseCostFunction(), 
                                                               constraints->getImpulseConstraints()));
  auto terminal_ocp = TerminalOCP(robot, cost, constraints);
  linearizer.computeKKTResidual(split_ocps, terminal_ocp, robots, contact_sequence, t, q, v, s, kkt_matrix, kkt_residual);
  auto split_ocps_ref = HybridOCP(N, SplitOCP(robot, cost, constraints), 
                                  max_num_impulse, SplitImpulseOCP(robot, 
                                                                  cost->getImpulseCostFunction(), 
                                                                  constraints->getImpulseConstraints()));
  auto terminal_ocp_ref = TerminalOCP(robot, cost, constraints);
  auto robot_ref = robot;
  if (contact_sequence.existImpulseStage(0)) {
    const double dtau_impulse = contact_sequence.impulseTime(0);
    const double dtau_aux = dtau - dtau_impulse;
    ASSERT_TRUE(dtau_impulse > 0);
    ASSERT_TRUE(dtau_impulse < dtau);
    split_ocps_ref[0].computeKKTResidual(
        robot_ref, contact_sequence.contactStatus(0), t, dtau_impulse, 
        q, s[0], s.impulse[0], kkt_matrix_ref[0], kkt_residual_ref[0]);
    split_ocps_ref.impulse[0].computeKKTResidual(
        robot_ref, contact_sequence.impulseStatus(0), t+dtau_impulse, 
        s[0].q, s.impulse[0], s.aux[0], kkt_matrix_ref.impulse[0], kkt_residual_ref.impulse[0]);
    split_ocps_ref.aux[0].computeKKTResidual(
        robot_ref, contact_sequence.contactStatus(1), t+dtau_impulse, dtau_aux, 
        s.impulse[0].q, s.aux[0], s[1], kkt_matrix_ref.aux[0], kkt_residual_ref.aux[0]);
  }
  else if (contact_sequence.existLiftStage(0)) {
    const double dtau_lift = contact_sequence.liftTime(0);
    const double dtau_aux = dtau - dtau_lift;
    ASSERT_TRUE(dtau_lift > 0);
    ASSERT_TRUE(dtau_lift < dtau);
    split_ocps_ref[0].computeKKTResidual(
        robot_ref, contact_sequence.contactStatus(0), t, dtau_lift, 
        q, s[0], s.lift[0], kkt_matrix_ref[0], kkt_residual_ref[0]);
    split_ocps_ref.lift[0].computeKKTResidual(
        robot_ref, contact_sequence.contactStatus(1), t+dtau_lift, dtau_aux, 
        s[0].q, s.lift[0], s[1], kkt_matrix_ref.lift[0], kkt_residual_ref.lift[0]);
  }
  else {
    split_ocps_ref[0].computeKKTResidual(
        robot_ref, contact_sequence.contactStatus(0), t, dtau, 
        q, s[0], s[1], kkt_matrix_ref[0], kkt_residual_ref[0]);
  }
  for (int i=1; i<N; ++i) {
    Eigen::VectorXd q_prev;
    if (contact_sequence.existImpulseStage(i-1)) {
      q_prev = s.aux[contact_sequence.impulseIndex(i-1)].q;
    }
    else if (contact_sequence.existLiftStage(i-1)) {
      q_prev = s.lift[contact_sequence.liftIndex(i-1)].q;
    }
    else {
      q_prev = s[i-1].q;
    }
    if (contact_sequence.existImpulseStage(i)) {
      const int impulse_index = contact_sequence.impulseIndex(i);
      const double impulse_time = contact_sequence.impulseTime(impulse_index);
      const double dtau_impulse = impulse_time - i * dtau;
      const double dtau_aux = dtau - dtau_impulse;
      ASSERT_TRUE(dtau_impulse > 0);
      ASSERT_TRUE(dtau_aux > 0);
      split_ocps_ref[i].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(i), 
          t+i*dtau, dtau_impulse, q_prev, s[i], s.impulse[impulse_index], 
          kkt_matrix_ref[i], kkt_residual_ref[i]);
      split_ocps_ref.impulse[impulse_index].computeKKTResidual(
          robot_ref, contact_sequence.impulseStatus(impulse_index), 
          t+impulse_time, s[i].q, s.impulse[impulse_index], s.aux[impulse_index], 
          kkt_matrix_ref.impulse[impulse_index], kkt_residual_ref.impulse[impulse_index]);
      split_ocps_ref.aux[impulse_index].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(i+1), 
          t+impulse_time, dtau_aux, s.impulse[impulse_index].q, s.aux[impulse_index], s[i+1], 
          kkt_matrix_ref.aux[impulse_index], kkt_residual_ref.aux[impulse_index]);
    }
    else if (contact_sequence.existLiftStage(i)) {
      const int lift_index = contact_sequence.liftIndex(i);
      const double lift_time = contact_sequence.liftTime(lift_index);
      const double dtau_lift = lift_time - i * dtau;
      const double dtau_aux = dtau - dtau_lift;
      ASSERT_TRUE(dtau_lift > 0);
      ASSERT_TRUE(dtau_aux > 0);
      split_ocps_ref[i].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(i), 
          t+i*dtau, dtau_lift, q_prev, s[i], s.lift[lift_index], 
          kkt_matrix_ref[i], kkt_residual_ref[i]);
      split_ocps_ref.lift[lift_index].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(i+1), 
          t+lift_time, dtau_aux, s[i].q, s.lift[lift_index], s[i+1], 
          kkt_matrix_ref.lift[lift_index], kkt_residual_ref.lift[lift_index]);
    }
    else {
      split_ocps_ref[i].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(i), 
          t+i*dtau, dtau, q_prev, s[i], s[i+1],
          kkt_matrix_ref[i], kkt_residual_ref[i]);
    }
  }
  terminal_ocp_ref.computeKKTResidual(robot_ref, t+T, s[N], kkt_residual_ref[N]);
  for (int i=0; i<=N; ++i) {
    kkt_matrix[i].isApprox(kkt_matrix_ref[i]);
    kkt_residual[i].isApprox(kkt_residual_ref[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_matrix.aux[i].isApprox(kkt_matrix_ref.aux[i]);
    kkt_residual.aux[i].isApprox(kkt_residual_ref.aux[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_matrix.impulse[i].isApprox(kkt_matrix_ref.impulse[i]);
    kkt_residual.impulse[i].isApprox(kkt_residual_ref.impulse[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_matrix.lift[i].isApprox(kkt_matrix_ref.lift[i]);
    kkt_residual.lift[i].isApprox(kkt_residual_ref.lift[i]);
  }
}


TEST_F(OCPLinearizerTest, fixedBase) {
  Robot robot(fixed_base_urdf);
  testLinearizeOCP(robot);
  testComputeKKTResidual(robot);
  std::vector<int> contact_frames = {18};
  robot = Robot(fixed_base_urdf, contact_frames);
  testLinearizeOCP(robot);
  testComputeKKTResidual(robot);
}


TEST_F(OCPLinearizerTest, floatingBase) {
  Robot robot(floating_base_urdf);
  testLinearizeOCP(robot);
  testComputeKKTResidual(robot);
  std::vector<int> contact_frames = {14, 24, 34, 44};
  robot = Robot(floating_base_urdf, contact_frames);
  testLinearizeOCP(robot);
  testComputeKKTResidual(robot);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
