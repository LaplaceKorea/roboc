#include <string>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/cost/cost_function.hpp"
#include "idocp/cost/joint_space_cost.hpp"
#include "idocp/cost/contact_force_cost.hpp"
#include "idocp/constraints/constraints.hpp"
#include "idocp/constraints/joint_position_lower_limit.hpp"
#include "idocp/constraints/joint_position_upper_limit.hpp"
#include "idocp/constraints/joint_velocity_lower_limit.hpp"
#include "idocp/constraints/joint_velocity_upper_limit.hpp"
#include "idocp/constraints/joint_torques_lower_limit.hpp"
#include "idocp/constraints/joint_torques_upper_limit.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_parnmpc.hpp"
#include "idocp/ocp/parnmpc.hpp"


namespace idocp {

// tests the equivalence between the paralle and serial implementation.
class ParNMPCTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf_ = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf_ = "../urdf/anymal/anymal.urdf";
    t_ = std::abs(Eigen::VectorXd::Random(1)[0]);
    T_ = std::abs(Eigen::VectorXd::Random(1)[0]);
    N_ = 10;
    nthreads_ = 1;
    dtau_ = T_ / N_;
  }

  virtual void TearDown() {
  }

  double dtau_, t_, T_;
  int N_, nthreads_;
  std::string fixed_base_urdf_, floating_base_urdf_;
};


TEST_F(ParNMPCTest, updateSolutionFixedBaseWithoutContact) {
  Robot robot(fixed_base_urdf_);
  std::random_device rnd;
  auto cost = std::make_shared<CostFunction>();
  auto joint_cost = std::make_shared<JointSpaceCost>(robot);
  // auto contact_cost = std::make_shared<ContactCost>(robot);
  const Eigen::VectorXd q_weight = Eigen::VectorXd::Constant(robot.dimv(), 10);
  const Eigen::VectorXd qf_weight = Eigen::VectorXd::Constant(robot.dimv(), 10);
  const Eigen::VectorXd q_ref = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v_weight = Eigen::VectorXd::Constant(robot.dimv(), 1);
  const Eigen::VectorXd vf_weight = Eigen::VectorXd::Constant(robot.dimv(), 1);
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd a_weight = Eigen::VectorXd::Constant(robot.dimv(), 0.1);
  const Eigen::VectorXd a_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd u_weight = Eigen::VectorXd::Constant(robot.dimv(), 0.01);
  const Eigen::VectorXd u_ref = Eigen::VectorXd::Zero(robot.dimv());
  std::vector<Eigen::Vector3d> f_weight, f_ref;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    f_weight.push_back(Eigen::Vector3d::Constant(0.01));
    f_ref.push_back(Eigen::Vector3d::Zero());
  }
  joint_cost->set_q_weight(q_weight);
  joint_cost->set_qf_weight(qf_weight);
  joint_cost->set_q_ref(q_ref);
  joint_cost->set_v_weight(v_weight);
  joint_cost->set_vf_weight(vf_weight);
  joint_cost->set_v_ref(v_ref);
  joint_cost->set_a_weight(a_weight);
  joint_cost->set_a_ref(a_ref);
  joint_cost->set_u_weight(u_weight);
  joint_cost->set_u_ref(u_ref);
  // contact_cost->set_f_weight(f_weight);
  // contact_cost->set_f_ref(f_ref);
  cost->push_back(joint_cost);
  // cost->push_back(contact_cost);
  auto constraints = std::make_shared<Constraints>();
  auto joint_lower_limit = std::make_shared<JointPositionLowerLimit>(robot);
  auto joint_upper_limit = std::make_shared<JointPositionUpperLimit>(robot);
  auto velocity_lower_limit = std::make_shared<JointVelocityLowerLimit>(robot);
  auto velocity_upper_limit = std::make_shared<JointVelocityUpperLimit>(robot);
  auto torques_lower_limit = std::make_shared<JointTorquesLowerLimit>(robot);
  auto torques_upper_limit = std::make_shared<JointTorquesUpperLimit>(robot);
  constraints->push_back(joint_upper_limit); 
  constraints->push_back(joint_lower_limit);
  constraints->push_back(velocity_lower_limit); 
  constraints->push_back(velocity_upper_limit);
  constraints->push_back(torques_lower_limit); 
  constraints->push_back(torques_upper_limit);
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  ParNMPC parnmpc(robot, cost, constraints, T_, N_, 1);
  ParNMPC parnmpc_ref(robot, cost, constraints, T_, N_, 2);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.computeKKTResidual(t_, q, v);
  parnmpc_ref.computeKKTResidual(t_, q, v);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.updateSolution(t_, q, v, false);
  parnmpc_ref.updateSolution(t_, q, v, false);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.updateSolution(t_, q, v, true);
  parnmpc_ref.updateSolution(t_, q, v, true);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
}


TEST_F(ParNMPCTest, updateSolutionFixedBaseWithContact) {
  std::vector<int> contact_frames = {18};
  Robot robot(fixed_base_urdf_, contact_frames);
  std::random_device rnd;
  std::vector<bool> contact_status = {rnd()%2==0};
  auto cost = std::make_shared<CostFunction>();
  auto joint_cost = std::make_shared<JointSpaceCost>(robot);
  // auto contact_cost = std::make_shared<ContactCost>(robot);
  const Eigen::VectorXd q_weight = Eigen::VectorXd::Constant(robot.dimv(), 10);
  const Eigen::VectorXd qf_weight = Eigen::VectorXd::Constant(robot.dimv(), 10);
  const Eigen::VectorXd q_ref = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v_weight = Eigen::VectorXd::Constant(robot.dimv(), 1);
  const Eigen::VectorXd vf_weight = Eigen::VectorXd::Constant(robot.dimv(), 1);
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd a_weight = Eigen::VectorXd::Constant(robot.dimv(), 0.1);
  const Eigen::VectorXd a_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd u_weight = Eigen::VectorXd::Constant(robot.dimv(), 0.01);
  const Eigen::VectorXd u_ref = Eigen::VectorXd::Zero(robot.dimv());
  std::vector<Eigen::Vector3d> f_weight, f_ref;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    f_weight.push_back(Eigen::Vector3d::Constant(0.01));
    f_ref.push_back(Eigen::Vector3d::Zero());
  }
  joint_cost->set_q_weight(q_weight);
  joint_cost->set_qf_weight(qf_weight);
  joint_cost->set_q_ref(q_ref);
  joint_cost->set_v_weight(v_weight);
  joint_cost->set_vf_weight(vf_weight);
  joint_cost->set_v_ref(v_ref);
  joint_cost->set_a_weight(a_weight);
  joint_cost->set_a_ref(a_ref);
  joint_cost->set_u_weight(u_weight);
  joint_cost->set_u_ref(u_ref);
  // contact_cost->set_f_weight(f_weight);
  // contact_cost->set_f_ref(f_ref);
  cost->push_back(joint_cost);
  // cost->push_back(contact_cost);
  auto constraints = std::make_shared<Constraints>();
  auto joint_lower_limit = std::make_shared<JointPositionLowerLimit>(robot);
  auto joint_upper_limit = std::make_shared<JointPositionUpperLimit>(robot);
  auto velocity_lower_limit = std::make_shared<JointVelocityLowerLimit>(robot);
  auto velocity_upper_limit = std::make_shared<JointVelocityUpperLimit>(robot);
  auto torques_lower_limit = std::make_shared<JointTorquesLowerLimit>(robot);
  auto torques_upper_limit = std::make_shared<JointTorquesUpperLimit>(robot);
  constraints->push_back(joint_upper_limit); 
  constraints->push_back(joint_lower_limit);
  constraints->push_back(velocity_lower_limit); 
  constraints->push_back(velocity_upper_limit);
  constraints->push_back(torques_lower_limit); 
  constraints->push_back(torques_upper_limit);
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  ParNMPC parnmpc(robot, cost, constraints, T_, N_, 1);
  if (contact_status[0]) {
    parnmpc.activateContact(0, 0, N_);
  }
  ParNMPC parnmpc_ref(robot, cost, constraints, T_, N_, 2);
  if (contact_status[0]) {
    parnmpc_ref.activateContact(0, 0, N_);
  }
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.computeKKTResidual(t_, q, v);
  parnmpc_ref.computeKKTResidual(t_, q, v);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.updateSolution(t_, q, v, false);
  parnmpc_ref.updateSolution(t_, q, v, false);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.updateSolution(t_, q, v, true);
  parnmpc_ref.updateSolution(t_, q, v, true);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
}


TEST_F(ParNMPCTest, floating_base) {
  std::vector<int> contact_frames = {14, 24, 34, 44};
  Robot robot(floating_base_urdf_, contact_frames);
  std::random_device rnd;
  std::vector<bool> contact_status;
  for (int i=0; i<contact_frames.size(); ++i) {
    contact_status.push_back(rnd()%2==0);
  }
  std::shared_ptr<CostFunction> cost = std::make_shared<CostFunction>();
  std::shared_ptr<JointSpaceCost> joint_cost = std::make_shared<JointSpaceCost>(robot);
  // std::shared_ptr<ContactCost> contact_cost = std::make_shared<ContactCost>(robot);
  const Eigen::VectorXd q_weight = Eigen::VectorXd::Constant(robot.dimv(), 10);
  const Eigen::VectorXd qf_weight = Eigen::VectorXd::Constant(robot.dimv(), 10);
  const Eigen::VectorXd q_ref = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v_weight = Eigen::VectorXd::Constant(robot.dimv(), 1);
  const Eigen::VectorXd vf_weight = Eigen::VectorXd::Constant(robot.dimv(), 1);
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd a_weight = Eigen::VectorXd::Constant(robot.dimv(), 0.1);
  const Eigen::VectorXd a_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd u_weight = Eigen::VectorXd::Constant(robot.dimv(), 0.01);
  const Eigen::VectorXd u_ref = Eigen::VectorXd::Constant(robot.dimv(), 0.1);
  std::vector<Eigen::Vector3d> f_weight, f_ref;
  for (int i=0; i<robot.maxPointContacts(); ++i) {
    f_weight.push_back(Eigen::Vector3d::Constant(0.01));
    f_ref.push_back(Eigen::Vector3d::Zero());
  }
  joint_cost->set_q_weight(q_weight);
  joint_cost->set_qf_weight(qf_weight);
  joint_cost->set_q_ref(q_ref);
  joint_cost->set_v_weight(v_weight);
  joint_cost->set_vf_weight(vf_weight);
  joint_cost->set_v_ref(v_ref);
  joint_cost->set_a_weight(a_weight);
  joint_cost->set_a_ref(a_ref);
  joint_cost->set_u_weight(u_weight);
  joint_cost->set_u_ref(u_ref);
  // contact_cost->set_f_weight(f_weight);
  // contact_cost->set_f_ref(f_ref);
  cost->push_back(joint_cost);
  // cost->push_back(contact_cost);
  auto constraints = std::make_shared<Constraints>();
  auto joint_lower_limit = std::make_shared<JointPositionLowerLimit>(robot);
  auto joint_upper_limit = std::make_shared<JointPositionUpperLimit>(robot);
  auto velocity_lower_limit = std::make_shared<JointVelocityLowerLimit>(robot);
  auto velocity_upper_limit = std::make_shared<JointVelocityUpperLimit>(robot);
  auto torques_lower_limit = std::make_shared<JointTorquesLowerLimit>(robot);
  auto torques_upper_limit = std::make_shared<JointTorquesUpperLimit>(robot);
  constraints->push_back(joint_upper_limit); 
  constraints->push_back(joint_lower_limit);
  constraints->push_back(velocity_lower_limit); 
  constraints->push_back(velocity_upper_limit);
  constraints->push_back(torques_lower_limit); 
  constraints->push_back(torques_upper_limit);
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  ParNMPC parnmpc(robot, cost, constraints, T_, N_, 1);
  for (int i=0; i<contact_status.size(); ++i) {
    if (contact_status[i]) {
      parnmpc.activateContact(i, 0, N_);
    }
  }
  ParNMPC parnmpc_ref(robot, cost, constraints, T_, N_, 2);
  for (int i=0; i<contact_status.size(); ++i) {
    if (contact_status[i]) {
      parnmpc_ref.activateContact(i, 0, N_);
    }
  }
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.computeKKTResidual(t_, q, v);
  parnmpc_ref.computeKKTResidual(t_, q, v);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.updateSolution(t_, q, v, false);
  parnmpc_ref.updateSolution(t_, q, v, false);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
  parnmpc.updateSolution(t_, q, v, true);
  parnmpc_ref.updateSolution(t_, q, v, true);
  EXPECT_DOUBLE_EQ(parnmpc.KKTError(), parnmpc_ref.KKTError());
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}