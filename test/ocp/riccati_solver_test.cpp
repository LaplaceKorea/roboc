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
#include "idocp/impulse/impulse_split_ocp.hpp"
#include "idocp/hybrid/hybrid_container.hpp"
#include "idocp/hybrid/contact_sequence.hpp"
#include "idocp/ocp/ocp_linearizer.hpp"
#include "idocp/ocp/riccati_solver.hpp"
#include "idocp/ocp/state_constraint_riccati_factorizer.hpp"
#include "idocp/ocp/state_constraint_riccati_factorization.hpp"
#include "idocp/ocp/riccati_recursion.hpp"
#include "idocp/ocp/riccati_direction_calculator.hpp"

#include "test_helper.hpp"

namespace idocp {

class RiccatiSolverTest : public ::testing::Test {
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

  void test(const Robot& robot) const;

  std::string fixed_base_urdf, floating_base_urdf;
  int N, max_num_impulse, nproc;
  double T, t, dtau;
};



Solution RiccatiSolverTest::createSolution(const Robot& robot) const {
  return testhelper::CreateSolution(robot, N, max_num_impulse);
}


Solution RiccatiSolverTest::createSolution(const Robot& robot, const ContactSequence& contact_sequence) const {
  return testhelper::CreateSolution(robot, contact_sequence, T, N, max_num_impulse, t);
}


ContactSequence RiccatiSolverTest::createContactSequence(const Robot& robot) const {
  return testhelper::CreateContactSequence(robot, N, max_num_impulse, t, 3*dtau);
}


void RiccatiSolverTest::test(const Robot& robot) const {
  auto cost = testhelper::CreateCost(robot);
  auto constraints = testhelper::CreateConstraints(robot);
  OCPLinearizer linearizer(T, N, max_num_impulse, nproc);
  ContactSequence contact_sequence(robot, N);
  if (robot.maxPointContacts() > 0) {
    contact_sequence = createContactSequence(robot);
  }
  OCPDiscretizer ocp_discretizer(T, N, max_num_impulse);
  ocp_discretizer.discretizeOCP(contact_sequence, t);
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
  auto ocp = OCP(N, max_num_impulse, robot, cost, constraints);
  std::vector<Robot> robots(nproc, robot);
  linearizer.initConstraints(ocp, robots, contact_sequence, t, s);
  linearizer.linearizeOCP(ocp, robots, contact_sequence, t, q, v, s, kkt_matrix, kkt_residual);
  Direction d = Direction(N, max_num_impulse, robot);
  auto ocp_ref = ocp;
  auto robots_ref = robots;
  auto d_ref = d;
  auto kkt_matrix_ref = kkt_matrix;
  auto kkt_residual_ref = kkt_residual;
  RiccatiSolver riccati_solver(robot, T, N, max_num_impulse, nproc);
  riccati_solver.computeNewtonDirection(ocp, robots, contact_sequence, 
                                        t, q, v, s, d, kkt_matrix, kkt_residual);
  RiccatiRecursion riccati_recursion(robot, N, nproc);
  RiccatiFactorizer riccati_factorizer(N, max_num_impulse, robot);
  RiccatiFactorization riccati_factorization(N, max_num_impulse, robot);
  StateConstraintRiccatiFactorizer constraint_factorizer(robot, N, max_num_impulse, nproc);
  StateConstraintRiccatiFactorization constraint_factorization(robot, N, max_num_impulse);
  RiccatiDirectionCalculator direction_calculator(N, max_num_impulse, nproc);
  riccati_recursion.backwardRiccatiRecursionTerminal(kkt_matrix_ref, kkt_residual_ref, 
                                                     riccati_factorization);
  riccati_recursion.backwardRiccatiRecursion(riccati_factorizer, ocp_discretizer, 
                                             kkt_matrix_ref, kkt_residual_ref, 
                                             riccati_factorization);
  constraint_factorization.setConstraintStatus(contact_sequence);
  riccati_recursion.forwardRiccatiRecursionParallel(riccati_factorizer, ocp_discretizer,
                                                    kkt_matrix_ref, kkt_residual_ref,
                                                    constraint_factorization);
  if (ocp_discretizer.existImpulse()) {
    riccati_recursion.forwardStateConstraintFactorization(
        riccati_factorizer, ocp_discretizer, kkt_matrix_ref, kkt_residual_ref, 
        riccati_factorization);
    riccati_recursion.backwardStateConstraintFactorization(
        riccati_factorizer, ocp_discretizer, kkt_matrix_ref, 
        constraint_factorization);
  }
  direction_calculator.computeInitialStateDirection(robots_ref, q, v, s, d_ref);
  if (ocp_discretizer.existImpulse()) {
    constraint_factorizer.computeLagrangeMultiplierDirection(
        ocp_discretizer, riccati_factorization, constraint_factorization, d_ref);
    constraint_factorizer.aggregateLagrangeMultiplierDirection(
        constraint_factorization, ocp_discretizer, d_ref, riccati_factorization);
  }
  riccati_recursion.forwardRiccatiRecursion(
      riccati_factorizer, ocp_discretizer, kkt_matrix_ref, kkt_residual_ref, 
      riccati_factorization, d_ref);
  direction_calculator.computeNewtonDirectionFromRiccatiFactorization(
      ocp_ref, robots_ref, ocp_discretizer, riccati_factorizer, 
      riccati_factorization, s, d_ref);
  EXPECT_TRUE(testhelper::IsApprox(kkt_matrix, kkt_matrix_ref));
  EXPECT_TRUE(testhelper::IsApprox(kkt_residual, kkt_residual_ref));
  EXPECT_TRUE(testhelper::IsApprox(d, d_ref));
}


TEST_F(RiccatiSolverTest, fixedBase) {
  Robot robot(fixed_base_urdf);
  test(robot);
  std::vector<int> contact_frames = {18};
  robot = Robot(fixed_base_urdf, contact_frames);
  test(robot);
}


TEST_F(RiccatiSolverTest, floatingBase) {
  Robot robot(floating_base_urdf);
  test(robot);
  std::vector<int> contact_frames = {14, 24, 34, 44};
  robot = Robot(floating_base_urdf, contact_frames);
  test(robot);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
