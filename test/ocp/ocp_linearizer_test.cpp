#include <string>
#include <memory>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/hybrid/hybrid_container.hpp"
#include "idocp/hybrid/contact_sequence.hpp"
#include "idocp/hybrid/ocp_discretizer.hpp"
#include "idocp/ocp/riccati_solver.hpp"
#include "idocp/ocp/ocp_linearizer.hpp"

#include "test_helper.hpp"

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

  Solution createSolution(const Robot& robot) const;
  Solution createSolution(const Robot& robot, const ContactSequence& contact_sequence) const;
  ContactSequence createContactSequence(const Robot& robot) const;

  void testLinearizeOCP(const Robot& robot) const;
  void testComputeKKTResidual(const Robot& robot) const;
  void testIntegrateSolution(const Robot& robot) const;

  std::string fixed_base_urdf, floating_base_urdf;
  int N, max_num_impulse, nproc;
  double T, t, dtau;
  std::shared_ptr<CostFunction> cost;
  std::shared_ptr<Constraints> constraints;
};



Solution OCPLinearizerTest::createSolution(const Robot& robot) const {
  return testhelper::CreateSolution(robot, N, max_num_impulse);
}


Solution OCPLinearizerTest::createSolution(const Robot& robot, const ContactSequence& contact_sequence) const {
  return testhelper::CreateSolution(robot, contact_sequence, T, N, max_num_impulse, t);
}


ContactSequence OCPLinearizerTest::createContactSequence(const Robot& robot) const {
  return testhelper::CreateContactSequence(robot, N, max_num_impulse, t, 3*dtau);
}


void OCPLinearizerTest::testLinearizeOCP(const Robot& robot) const {
  auto cost = testhelper::CreateCost(robot);
  auto constraints = testhelper::CreateConstraints(robot);
  OCPLinearizer linearizer(T, N, max_num_impulse, nproc);
  ContactSequence contact_sequence(robot, N);
  if (robot.maxPointContacts() > 0) {
    contact_sequence = createContactSequence(robot);
  }
  auto kkt_matrix = KKTMatrix(N, max_num_impulse, robot);
  auto kkt_residual = KKTResidual(N, max_num_impulse, robot);
  Solution s;
  if (robot.maxPointContacts() > 0) {
    s = createSolution(robot, contact_sequence);
  }
  else {
    s = createSolution(robot);
  }
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  auto kkt_matrix_ref = kkt_matrix;
  auto kkt_residual_ref = kkt_residual;
  std::vector<Robot> robots(nproc, robot);
  auto ocp = OCP(N, max_num_impulse, robot, cost, constraints);
  auto ocp_ref = ocp;
  linearizer.initConstraints(ocp, robots, contact_sequence, t, s);
  linearizer.linearizeOCP(ocp, robots, contact_sequence, t, q, v, s, kkt_matrix, kkt_residual);
  auto robot_ref = robot;
  OCPDiscretizer ocp_discretizer(T, N, max_num_impulse);
  ocp_discretizer.discretizeOCP(contact_sequence, t);
  for (int i=0; i<N; ++i) {
    Eigen::VectorXd q_prev;
    if (i == 0 ) {
      q_prev = q;
    }
    else if (ocp_discretizer.isTimeStageBeforeImpulse(i-1)) {
      q_prev = s.aux[ocp_discretizer.impulseIndex(i-1)].q;
    }
    else if (ocp_discretizer.isTimeStageBeforeLift(i-1)) {
      q_prev = s.lift[ocp_discretizer.liftIndex(i-1)].q;
    }
    else {
      q_prev = s[i-1].q;
    }
    if (ocp_discretizer.isTimeStageBeforeImpulse(i)) {
      const int contact_phase = ocp_discretizer.contactPhase(i);
      const int impulse_index = ocp_discretizer.impulseIndex(i);
      const double ti = ocp_discretizer.t(i);
      const double t_impulse = ocp_discretizer.t_impulse(impulse_index);
      const double dt = ocp_discretizer.dtau(i);
      const double dt_aux = ocp_discretizer.dtau_aux(impulse_index);
      ASSERT_TRUE(dt >= 0);
      ASSERT_TRUE(dt <= dtau);
      ASSERT_TRUE(dt_aux >= 0);
      ASSERT_TRUE(dt_aux <= dtau);
      const bool is_state_constraint_valid = (i > 0);
      ocp_ref[i].initConstraints(robot_ref, i, dt, s[i]);
      ocp_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), ti, dt, q_prev, 
          s[i], s.impulse[impulse_index], kkt_matrix_ref[i], kkt_residual_ref[i]);
      ocp_ref.impulse[impulse_index].initConstraints(robot_ref, s.impulse[i]);
      ocp_ref.impulse[impulse_index].linearizeOCP(
          robot_ref, contact_sequence.impulseStatus(impulse_index), t_impulse, s[i].q, 
          s.impulse[impulse_index], s.aux[impulse_index], 
          kkt_matrix_ref.impulse[impulse_index], kkt_residual_ref.impulse[impulse_index], 
          is_state_constraint_valid);
      ocp_ref.aux[impulse_index].initConstraints(robot_ref, 0, dt_aux, s.aux[impulse_index]);
      ocp_ref.aux[impulse_index].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase+1), t_impulse, dt_aux, s.impulse[impulse_index].q, 
          s.aux[impulse_index], s[i+1], 
          kkt_matrix_ref.aux[impulse_index], kkt_residual_ref.aux[impulse_index]);
    }
    else if (ocp_discretizer.isTimeStageBeforeLift(i)) {
      const int contact_phase = ocp_discretizer.contactPhase(i);
      const int lift_index = ocp_discretizer.liftIndex(i);
      const double ti = ocp_discretizer.t(i);
      const double t_lift = ocp_discretizer.t_lift(lift_index);
      const double dt = ocp_discretizer.dtau(i);
      const double dt_lift = ocp_discretizer.dtau_lift(lift_index);
      ASSERT_TRUE(dt >= 0);
      ASSERT_TRUE(dt <= dtau);
      ASSERT_TRUE(dt_lift >= 0);
      ASSERT_TRUE(dt_lift <= dtau);
      ocp_ref[i].initConstraints(robot_ref, i, dt, s[i]);
      ocp_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), ti, dt, q_prev, 
          s[i], s.lift[lift_index], kkt_matrix_ref[i], kkt_residual_ref[i]);
      ocp_ref.lift[lift_index].initConstraints(robot_ref, 0, dt_lift, s.lift[lift_index]);
      ocp_ref.lift[lift_index].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase+1), t_lift, dt_lift, s[i].q, 
          s.lift[lift_index], s[i+1], kkt_matrix_ref.lift[lift_index], kkt_residual_ref.lift[lift_index]);
    }
    else {
      const int contact_phase = ocp_discretizer.contactPhase(i);
      const double dt = ocp_discretizer.dtau(i);
      ocp_ref[i].initConstraints(robot_ref, i, ocp_discretizer.dtau(i), s[i]);
      ocp_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), 
          ocp_discretizer.t(i), dt, q_prev, 
          s[i], s[i+1], kkt_matrix_ref[i], kkt_residual_ref[i]);
    }
  }
  ocp_ref.terminal.linearizeOCP(robot_ref, t+T, s[N], kkt_matrix_ref[N], kkt_residual_ref[N]);
  EXPECT_TRUE(testhelper::IsApprox(kkt_matrix, kkt_matrix_ref));
  EXPECT_TRUE(testhelper::IsApprox(kkt_residual, kkt_residual_ref));
  EXPECT_FALSE(testhelper::HasNaN(kkt_matrix));
  EXPECT_FALSE(testhelper::HasNaN(kkt_matrix_ref));
  EXPECT_FALSE(testhelper::HasNaN(kkt_residual));
  EXPECT_FALSE(testhelper::HasNaN(kkt_residual_ref));
}


void OCPLinearizerTest::testComputeKKTResidual(const Robot& robot) const {
  auto cost = testhelper::CreateCost(robot);
  auto constraints = testhelper::CreateConstraints(robot);
  OCPLinearizer linearizer(T, N, max_num_impulse, nproc);
  ContactSequence contact_sequence(robot, N);
  if (robot.maxPointContacts() > 0) {
    contact_sequence = createContactSequence(robot);
  }
  auto kkt_matrix = KKTMatrix(N, max_num_impulse, robot);
  auto kkt_residual = KKTResidual(N, max_num_impulse, robot);
  Solution s;
  if (robot.maxPointContacts() > 0) {
    s = createSolution(robot, contact_sequence);
  }
  else {
    s = createSolution(robot);
  }
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  auto kkt_matrix_ref = kkt_matrix;
  auto kkt_residual_ref = kkt_residual;
  std::vector<Robot> robots(nproc, robot);
  auto ocp = OCP(N, max_num_impulse, robot, cost, constraints);
  auto ocp_ref = ocp;
  linearizer.initConstraints(ocp, robots, contact_sequence, t, s);
  linearizer.computeKKTResidual(ocp, robots, contact_sequence, t, q, v, s, kkt_matrix, kkt_residual);
  const double kkt_error = linearizer.KKTError(ocp, kkt_residual);
  auto robot_ref = robot;
  OCPDiscretizer ocp_discretizer(T, N, max_num_impulse);
  ocp_discretizer.discretizeOCP(contact_sequence, t);
  double kkt_error_ref = 0;
  for (int i=0; i<N; ++i) {
    Eigen::VectorXd q_prev;
    if (i == 0 ) {
      q_prev = q;
    }
    else if (ocp_discretizer.isTimeStageBeforeImpulse(i-1)) {
      q_prev = s.aux[ocp_discretizer.impulseIndex(i-1)].q;
    }
    else if (ocp_discretizer.isTimeStageBeforeLift(i-1)) {
      q_prev = s.lift[ocp_discretizer.liftIndex(i-1)].q;
    }
    else {
      q_prev = s[i-1].q;
    }
    if (ocp_discretizer.isTimeStageBeforeImpulse(i)) {
      const int contact_phase = ocp_discretizer.contactPhase(i);
      const int impulse_index = ocp_discretizer.impulseIndex(i);
      const double ti = ocp_discretizer.t(i);
      const double t_impulse = ocp_discretizer.t_impulse(impulse_index);
      const double dt = ocp_discretizer.dtau(i);
      const double dt_aux = ocp_discretizer.dtau_aux(impulse_index);
      ASSERT_TRUE(dt >= 0);
      ASSERT_TRUE(dt <= dtau);
      ASSERT_TRUE(dt_aux >= 0);
      ASSERT_TRUE(dt_aux <= dtau);
      const bool is_state_constraint_valid = (i > 0);
      ocp_ref[i].initConstraints(robot_ref, i, dt, s[i]);
      ocp_ref[i].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(contact_phase), ti, dt, q_prev, 
          s[i], s.impulse[impulse_index], kkt_matrix_ref[i], kkt_residual_ref[i]);
      ocp_ref.impulse[impulse_index].initConstraints(robot_ref, s.impulse[i]);
      ocp_ref.impulse[impulse_index].computeKKTResidual(
          robot_ref, contact_sequence.impulseStatus(impulse_index), t_impulse, s[i].q, 
          s.impulse[impulse_index], s.aux[impulse_index], 
          kkt_matrix_ref.impulse[impulse_index], kkt_residual_ref.impulse[impulse_index], 
          is_state_constraint_valid);
      ocp_ref.aux[impulse_index].initConstraints(robot_ref, 0, dt_aux, s.aux[impulse_index]);
      ocp_ref.aux[impulse_index].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(contact_phase+1), t_impulse, dt_aux, s.impulse[impulse_index].q, 
          s.aux[impulse_index], s[i+1], 
          kkt_matrix_ref.aux[impulse_index], kkt_residual_ref.aux[impulse_index]);
      kkt_error_ref += ocp_ref[i].squaredNormKKTResidual(kkt_residual_ref[i], dt);
      kkt_error_ref += ocp_ref.impulse[impulse_index].squaredNormKKTResidual(kkt_residual_ref.impulse[impulse_index], 
                                                                             is_state_constraint_valid);
      kkt_error_ref += ocp_ref.aux[impulse_index].squaredNormKKTResidual(kkt_residual_ref.aux[impulse_index], dt_aux);
    }
    else if (ocp_discretizer.isTimeStageBeforeLift(i)) {
      const int contact_phase = ocp_discretizer.contactPhase(i);
      const int lift_index = ocp_discretizer.liftIndex(i);
      const double ti = ocp_discretizer.t(i);
      const double t_lift = ocp_discretizer.t_lift(lift_index);
      const double dt = ocp_discretizer.dtau(i);
      const double dt_lift = ocp_discretizer.dtau_lift(lift_index);
      ASSERT_TRUE(dt >= 0);
      ASSERT_TRUE(dt <= dtau);
      ASSERT_TRUE(dt_lift >= 0);
      ASSERT_TRUE(dt_lift <= dtau);
      ocp_ref[i].initConstraints(robot_ref, i, dt, s[i]);
      ocp_ref[i].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(contact_phase), ti, dt, q_prev, 
          s[i], s.lift[lift_index], kkt_matrix_ref[i], kkt_residual_ref[i]);
      ocp_ref.lift[lift_index].initConstraints(robot_ref, 0, dt_lift, s.lift[lift_index]);
      ocp_ref.lift[lift_index].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(contact_phase+1), t_lift, dt_lift, s[i].q, 
          s.lift[lift_index], s[i+1], kkt_matrix_ref.lift[lift_index], kkt_residual_ref.lift[lift_index]);
      kkt_error_ref += ocp_ref[i].squaredNormKKTResidual(kkt_residual_ref[i], dt);
      kkt_error_ref += ocp_ref.lift[lift_index].squaredNormKKTResidual(kkt_residual_ref.lift[lift_index], dt_lift);
    }
    else {
      const int contact_phase = ocp_discretizer.contactPhase(i);
      const double dt = ocp_discretizer.dtau(i);
      ocp_ref[i].initConstraints(robot_ref, i, ocp_discretizer.dtau(i), s[i]);
      ocp_ref[i].computeKKTResidual(
          robot_ref, contact_sequence.contactStatus(contact_phase), 
          ocp_discretizer.t(i), dt, q_prev, 
          s[i], s[i+1], kkt_matrix_ref[i], kkt_residual_ref[i]);
      kkt_error_ref += ocp_ref[i].squaredNormKKTResidual(kkt_residual_ref[i], dt);
    }
  }
  ocp_ref.terminal.computeKKTResidual(robot_ref, t+T, s[N], kkt_residual_ref[N]);
  kkt_error_ref += ocp_ref.terminal.squaredNormKKTResidual(kkt_residual_ref[N]);
  EXPECT_TRUE(testhelper::IsApprox(kkt_matrix, kkt_matrix_ref));
  EXPECT_TRUE(testhelper::IsApprox(kkt_residual, kkt_residual_ref));
  EXPECT_FALSE(testhelper::HasNaN(kkt_matrix));
  EXPECT_FALSE(testhelper::HasNaN(kkt_matrix_ref));
  EXPECT_FALSE(testhelper::HasNaN(kkt_residual));
  EXPECT_FALSE(testhelper::HasNaN(kkt_residual_ref));
  EXPECT_DOUBLE_EQ(kkt_error, std::sqrt(kkt_error_ref));
}


void OCPLinearizerTest::testIntegrateSolution(const Robot& robot) const {
  auto cost = testhelper::CreateCost(robot);
  auto constraints = testhelper::CreateConstraints(robot);
  OCPLinearizer linearizer(T, N, max_num_impulse, nproc);
  ContactSequence contact_sequence(robot, N);
  if (robot.maxPointContacts() > 0) {
    contact_sequence = createContactSequence(robot);
  }
  auto kkt_matrix = KKTMatrix(N, max_num_impulse, robot);
  auto kkt_residual = KKTResidual(N, max_num_impulse, robot);
  Solution s;
  if (robot.maxPointContacts() > 0) {
    s = createSolution(robot, contact_sequence);
  }
  else {
    s = createSolution(robot);
  }
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  std::vector<Robot> robots(nproc, robot);
  auto ocp = OCP(N, max_num_impulse, robot, cost, constraints);
  linearizer.initConstraints(ocp, robots, contact_sequence, t, s);
  linearizer.linearizeOCP(ocp, robots, contact_sequence, t, q, v, s, kkt_matrix, kkt_residual);
  OCPDiscretizer ocp_discretizer(T, N, max_num_impulse);
  ocp_discretizer.discretizeOCP(contact_sequence, t);
  Direction d(N, max_num_impulse, robot);
  RiccatiSolver riccati_solver(robots[0], T, N, max_num_impulse, nproc);
  riccati_solver.computeNewtonDirection(ocp, robots, contact_sequence, 
                                        t, q, v, s, d, kkt_matrix, kkt_residual);
  const double primal_step_size = riccati_solver.maxPrimalStepSize();
  const double dual_step_size = riccati_solver.maxDualStepSize();
  ASSERT_TRUE(primal_step_size > 0);
  ASSERT_TRUE(primal_step_size <= 1);
  ASSERT_TRUE(dual_step_size > 0);
  ASSERT_TRUE(dual_step_size <= 1);
  auto ocp_ref = ocp;
  auto s_ref = s;
  auto d_ref = d;
  auto kkt_matrix_ref = kkt_matrix;
  auto kkt_residual_ref = kkt_residual;
  linearizer.integrateSolution(ocp, robots, kkt_matrix, kkt_residual, 
                               primal_step_size, dual_step_size, d, s);
  auto robot_ref = robot;
  for (int i=0; i<N; ++i) {
    if (ocp_discretizer.isTimeStageBeforeImpulse(i)) {
      const int impulse_index = ocp_discretizer.impulseIndex(i);
      const double dt = ocp_discretizer.dtau(i);
      const double dt_aux = ocp_discretizer.dtau_aux(impulse_index);
      ASSERT_TRUE(dt >= 0);
      ASSERT_TRUE(dt <= dtau);
      ASSERT_TRUE(dt_aux >= 0);
      ASSERT_TRUE(dt_aux <= dtau);
      const bool is_state_constraint_valid = (i > 0);
      ocp_ref[i].computeCondensedDualDirection(
          robot_ref, dt, kkt_matrix_ref[i], kkt_residual_ref[i], 
          d_ref.impulse[impulse_index], d_ref[i]);
      ocp_ref[i].updatePrimal(robot_ref, primal_step_size, d_ref[i], s_ref[i]);
      ocp_ref[i].updateDual(dual_step_size);
      ocp_ref.impulse[impulse_index].computeCondensedDualDirection(
          robot_ref, kkt_matrix_ref.impulse[impulse_index], kkt_residual_ref.impulse[impulse_index], 
          d_ref.aux[impulse_index], d_ref.impulse[impulse_index]);
      ocp_ref.impulse[impulse_index].updatePrimal(
          robot_ref, primal_step_size, d_ref.impulse[impulse_index], s_ref.impulse[impulse_index],
          is_state_constraint_valid);
      ocp_ref.impulse[impulse_index].updateDual(dual_step_size);
      ocp_ref.aux[impulse_index].computeCondensedDualDirection(
          robot_ref, dt_aux, kkt_matrix_ref.aux[impulse_index], kkt_residual_ref.aux[impulse_index],
          d_ref[i+1], d_ref.aux[impulse_index]);
      ocp_ref.aux[impulse_index].updatePrimal(
          robot_ref, primal_step_size, d_ref.aux[impulse_index], s_ref.aux[impulse_index]);
      ocp_ref.aux[impulse_index].updateDual(dual_step_size);
    }
    else if (ocp_discretizer.isTimeStageBeforeLift(i)) {
      const int lift_index = ocp_discretizer.liftIndex(i);
      const double dt = ocp_discretizer.dtau(i);
      const double dt_lift = ocp_discretizer.dtau_lift(lift_index);
      ASSERT_TRUE(dt >= 0);
      ASSERT_TRUE(dt <= dtau);
      ASSERT_TRUE(dt_lift >= 0);
      ASSERT_TRUE(dt_lift <= dtau);
      ocp_ref[i].computeCondensedDualDirection(
          robot_ref, dt, kkt_matrix_ref[i], kkt_residual_ref[i], 
          d_ref.lift[lift_index], d_ref[i]);
      ocp_ref[i].updatePrimal(robot_ref, primal_step_size, d_ref[i], s_ref[i]);
      ocp_ref[i].updateDual(dual_step_size);
      ocp_ref.lift[lift_index].computeCondensedDualDirection(
          robot_ref, dt_lift, kkt_matrix_ref.lift[lift_index], kkt_residual_ref.lift[lift_index], 
          d_ref[i+1], d_ref.lift[lift_index]);
      ocp_ref.lift[lift_index].updatePrimal(robot_ref, primal_step_size, d_ref.lift[lift_index], s_ref.lift[lift_index]);
      ocp_ref.lift[lift_index].updateDual(dual_step_size);
    }
    else {
      const double dt = ocp_discretizer.dtau(i);
      ocp_ref[i].computeCondensedDualDirection(
          robot_ref, dtau, kkt_matrix_ref[i], kkt_residual_ref[i], d_ref[i+1], d_ref[i]);
      ocp_ref[i].updatePrimal(robot_ref, primal_step_size, d_ref[i], s_ref[i]);
      ocp_ref[i].updateDual(dual_step_size);
    }
  }
  ocp_ref.terminal.updatePrimal(robot_ref, primal_step_size, d_ref[N], s_ref[N]);
  ocp_ref.terminal.updateDual(dual_step_size);
  EXPECT_TRUE(testhelper::IsApprox(s, s_ref));
  EXPECT_TRUE(testhelper::IsApprox(d, d_ref));
}


TEST_F(OCPLinearizerTest, fixedBase) {
  Robot robot(fixed_base_urdf);
  testLinearizeOCP(robot);
  testComputeKKTResidual(robot);
  testIntegrateSolution(robot);
  std::vector<int> contact_frames = {18};
  robot = Robot(fixed_base_urdf, contact_frames);
  testLinearizeOCP(robot);
  testComputeKKTResidual(robot);
  testIntegrateSolution(robot);
}


TEST_F(OCPLinearizerTest, floatingBase) {
  Robot robot(floating_base_urdf);
  testLinearizeOCP(robot);
  testComputeKKTResidual(robot);
  testIntegrateSolution(robot);
  std::vector<int> contact_frames = {14, 24, 34, 44};
  robot = Robot(floating_base_urdf, contact_frames);
  testLinearizeOCP(robot);
  testComputeKKTResidual(robot);
  testIntegrateSolution(robot);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
