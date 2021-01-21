#include <string>
#include <memory>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/hybrid/hybrid_container.hpp"
#include "idocp/hybrid/contact_sequence.hpp"
#include "idocp/ocp/parnmpc_linearizer.hpp"
#include "idocp/ocp/backward_correction.hpp"

#include "test_helper.hpp"

namespace idocp {

class BackwardCorrectionTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
    N = 20;
    max_num_impulse = 5;
    nthreads = 4;
    T = 1;
    t = std::abs(Eigen::VectorXd::Random(1)[0]);
    dtau = T / N;
  }

  virtual void TearDown() {
  }

  Solution createSolution(const Robot& robot) const;
  Solution createSolution(const Robot& robot, const ContactSequence& contact_sequence) const;
  ContactSequence createContactSequence(const Robot& robot) const;

  void testCoarseUpdate(const Robot& robot) const;

  std::string fixed_base_urdf, floating_base_urdf;
  int N, max_num_impulse, nthreads;
  double T, t, dtau;
  std::shared_ptr<CostFunction> cost;
  std::shared_ptr<Constraints> constraints;
};



Solution BackwardCorrectionTest::createSolution(const Robot& robot) const {
  return testhelper::CreateSolution(robot, N, max_num_impulse);
}


Solution BackwardCorrectionTest::createSolution(const Robot& robot, const ContactSequence& contact_sequence) const {
  return testhelper::CreateSolution(robot, contact_sequence, T, N, max_num_impulse, t, true);
}


ContactSequence BackwardCorrectionTest::createContactSequence(const Robot& robot) const {
  return testhelper::CreateContactSequence(robot, N, max_num_impulse, t, 3*dtau);
}


void BackwardCorrectionTest::testCoarseUpdate(const Robot& robot) const {
  auto cost = testhelper::CreateCost(robot);
  auto constraints = testhelper::CreateConstraints(robot);
  const auto contact_sequence = createContactSequence(robot);
  auto kkt_matrix = KKTMatrix(robot, N, max_num_impulse);
  auto kkt_residual = KKTResidual(robot, N, max_num_impulse);
  const auto s = createSolution(robot, contact_sequence);
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  auto kkt_matrix_ref = kkt_matrix;
  auto kkt_residual_ref = kkt_residual;
  std::vector<Robot> robots(nthreads, robot);
  auto parnmpc = ParNMPC(robot, cost, constraints, T, N, max_num_impulse);
  parnmpc.discretize(contact_sequence, t);
  ParNMPCLinearizer linearizer(N, max_num_impulse, nthreads);
  linearizer.initConstraints(parnmpc, robots, contact_sequence, s);
  auto parnmpc_ref = parnmpc;
  BackwardCorrection back_corr(robot, N, max_num_impulse, nthreads);
  back_corr.coarseUpdate(parnmpc, robots, contact_sequence, q, v, s, kkt_matrix, kkt_residual);
  Robot robot_ref = robot;
  for (int i=0; i<N; ++i) {
    Eigen::VectorXd q_prev, v_prev;
    if (parnmpc_ref.discrete().isTimeStageAfterImpulse(i)) {
      q_prev = s.impulse[parnmpc_ref.discrete().impulseIndexBeforeTimeStage(i)].q;
      v_prev = s.impulse[parnmpc_ref.discrete().impulseIndexBeforeTimeStage(i)].v;
    }
    else if (parnmpc_ref.discrete().isTimeStageAfterLift(i)) {
      q_prev = s.lift[parnmpc_ref.discrete().liftIndexBeforeTimeStage(i)].q;
      v_prev = s.lift[parnmpc_ref.discrete().liftIndexBeforeTimeStage(i)].v;
    }
    else if (i == 0) {
      q_prev = q;
      v_prev = v;
    }
    else {
      q_prev = s[i-1].q;
      v_prev = s[i-1].v;
    }
    if (i == N-1) {
      const int contact_phase = parnmpc_ref.discrete().contactPhase(i);
      const double dt = parnmpc_ref.discrete().dtau(i);
      parnmpc_ref.terminal.linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), 
          parnmpc_ref.discrete().t(i), dt, q_prev, v_prev,
          s[i], kkt_matrix_ref[i], kkt_residual_ref[i]);
    }
    else if (parnmpc_ref.discrete().isTimeStageBeforeImpulse(i)) {
      const int contact_phase = parnmpc_ref.discrete().contactPhase(i);
      const int impulse_index = parnmpc_ref.discrete().impulseIndexAfterTimeStage(i);
      const double ti = parnmpc_ref.discrete().t(i);
      const double t_impulse = parnmpc_ref.discrete().t_impulse(impulse_index);
      const double dt = parnmpc_ref.discrete().dtau(i);
      const double dt_aux = parnmpc_ref.discrete().dtau_aux(impulse_index);
      ASSERT_TRUE(dt >= 0);
      ASSERT_TRUE(dt <= dtau);
      ASSERT_TRUE(dt_aux >= 0);
      ASSERT_TRUE(dt_aux <= dtau);
      parnmpc_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), ti, dt, 
          q_prev, v_prev, s[i], s.aux[impulse_index], kkt_matrix_ref[i], kkt_residual_ref[i]);
      parnmpc_ref.aux[impulse_index].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), 
          contact_sequence.impulseStatus(impulse_index), t_impulse, dt_aux, 
          s[i].q, s[i].v, s.aux[impulse_index], s.impulse[impulse_index], 
          kkt_matrix_ref.aux[impulse_index], kkt_residual_ref.aux[impulse_index]);
      parnmpc_ref.impulse[impulse_index].linearizeOCP(
          robot_ref, contact_sequence.impulseStatus(impulse_index), t_impulse, 
          s.aux[impulse_index].q, s.aux[impulse_index].v, 
          s.impulse[impulse_index], s[i+1], 
          kkt_matrix_ref.impulse[impulse_index], kkt_residual_ref.impulse[impulse_index]);
    }
    else if (parnmpc_ref.discrete().isTimeStageBeforeLift(i)) {
      const int contact_phase = parnmpc_ref.discrete().contactPhase(i);
      const int lift_index = parnmpc_ref.discrete().liftIndexAfterTimeStage(i);
      const double ti = parnmpc_ref.discrete().t(i);
      const double t_lift = parnmpc_ref.discrete().t_lift(lift_index);
      const double dt = parnmpc_ref.discrete().dtau(i);
      const double dt_lift = parnmpc_ref.discrete().dtau_lift(lift_index);
      ASSERT_TRUE(dt >= 0);
      ASSERT_TRUE(dt <= dtau);
      ASSERT_TRUE(dt_lift >= 0);
      ASSERT_TRUE(dt_lift <= dtau);
      parnmpc_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), ti, dt, 
          q_prev, v_prev, s[i], s.lift[lift_index], kkt_matrix_ref[i], kkt_residual_ref[i]);
      parnmpc_ref.lift[lift_index].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), t_lift, dt_lift, 
          s[i].q, s[i].v, s.lift[lift_index], s[i+1], 
          kkt_matrix_ref.lift[lift_index], kkt_residual_ref.lift[lift_index]);
    }
    else {
      const int contact_phase = parnmpc_ref.discrete().contactPhase(i);
      const double dt = parnmpc_ref.discrete().dtau(i);
      parnmpc_ref[i].linearizeOCP(
          robot_ref, contact_sequence.contactStatus(contact_phase), 
          parnmpc_ref.discrete().t(i), dt, q_prev, v_prev,
          s[i], s[i+1], kkt_matrix_ref[i], kkt_residual_ref[i]);
    }
  }
  // Discrete events before s[0]. (between s[0] and q, v).
  if (parnmpc_ref.discrete().isTimeStageAfterImpulse(0)) {
    const int contact_phase = parnmpc_ref.discrete().contactPhase(-1);
    const int impulse_index = parnmpc_ref.discrete().impulseIndexBeforeTimeStage(0);
    ASSERT_EQ(contact_phase, 0);
    ASSERT_EQ(impulse_index, 0);
    const double t_impulse = parnmpc_ref.discrete().t_impulse(impulse_index);
    const double dt_aux = parnmpc_ref.discrete().dtau_aux(impulse_index);
    parnmpc_ref.aux[impulse_index].linearizeOCP(
        robot_ref, contact_sequence.contactStatus(contact_phase), 
        t_impulse, dt_aux, q, v, s.aux[impulse_index], s.impulse[impulse_index], 
        kkt_matrix_ref.aux[impulse_index], kkt_residual_ref.aux[impulse_index]);
    parnmpc_ref.impulse[impulse_index].linearizeOCP(
        robot_ref, contact_sequence.impulseStatus(impulse_index), t_impulse, 
        s.aux[impulse_index].q, s.aux[impulse_index].v, 
        s.impulse[impulse_index], s[0], 
        kkt_matrix_ref.impulse[impulse_index], kkt_residual_ref.impulse[impulse_index]);
  }
  else if (parnmpc_ref.discrete().isTimeStageAfterLift(0)) {
    const int contact_phase = parnmpc_ref.discrete().contactPhase(-1);
    const int lift_index = parnmpc_ref.discrete().liftIndexBeforeTimeStage(0);
    ASSERT_EQ(contact_phase, 0);
    ASSERT_EQ(lift_index, 0);
    const double t_lift = parnmpc_ref.discrete().t_lift(lift_index);
    const double dt_lift = parnmpc_ref.discrete().dtau_lift(lift_index);
    parnmpc_ref.lift[lift_index].linearizeOCP(
        robot_ref, contact_sequence.contactStatus(contact_phase), t_lift, dt_lift, 
        q, v, s.lift[lift_index], s[0], 
        kkt_matrix_ref.lift[lift_index], kkt_residual_ref.lift[lift_index]);
  }
  for (int i=0; i<N; ++i) {
    std::cout << "i = " << i << std::endl;
    EXPECT_TRUE(kkt_matrix[i].isApprox(kkt_matrix_ref[i]));
  }

  EXPECT_TRUE(testhelper::IsApprox(kkt_matrix, kkt_matrix_ref));
  EXPECT_TRUE(testhelper::IsApprox(kkt_residual, kkt_residual_ref));
  EXPECT_FALSE(testhelper::HasNaN(kkt_matrix));
  EXPECT_FALSE(testhelper::HasNaN(kkt_matrix_ref));
  EXPECT_FALSE(testhelper::HasNaN(kkt_residual));
  EXPECT_FALSE(testhelper::HasNaN(kkt_residual_ref));
}


TEST_F(BackwardCorrectionTest, fixedBase) {
  Robot robot(fixed_base_urdf);
  testCoarseUpdate(robot);
  std::vector<int> contact_frames = {18};
  robot = Robot(fixed_base_urdf, contact_frames);
  testCoarseUpdate(robot);
}


TEST_F(BackwardCorrectionTest, floatingBase) {
  Robot robot(floating_base_urdf);
  testCoarseUpdate(robot);
  std::vector<int> contact_frames = {14, 24, 34, 44};
  robot = Robot(floating_base_urdf, contact_frames);
  testCoarseUpdate(robot);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
