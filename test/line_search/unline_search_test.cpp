#include <string>
#include <memory>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/unocp/unconstrained_container.hpp"
#include "idocp/line_search/line_search_filter.hpp"
#include "idocp/line_search/unline_search.hpp"

#include "test_helper.hpp"


namespace idocp {

class UnLineSearchTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    urdf = "../urdf/iiwa14/iiwa14.urdf";
    robot = Robot(urdf);
    dimv = robot.dimv();
    N = 20;
    nthreads = 4;
    T = 1;
    dtau = T / N;
    t = std::abs(Eigen::VectorXd::Random(1)[0]);
    step_size_reduction_rate = 0.75;
    min_step_size = 0.05;
  }

  virtual void TearDown() {
  }

  UnSolution createUnSolution(const int size) const;
  UnDirection createUnDirection(const int size) const;

  Robot robot;
  std::string urdf;
  int dimv, N, nthreads;
  double T, dtau, t, step_size_reduction_rate, min_step_size;
  std::shared_ptr<CostFunction> cost;
  std::shared_ptr<Constraints> constraints;
};


UnSolution UnLineSearchTest::createUnSolution(const int size) const {
  UnSolution s(size, SplitSolution(robot));
  for (int i=0; i<size; ++i) {
    s[i].setRandom(robot);
  }
  return s;
}


UnDirection UnLineSearchTest::createUnDirection(const int size) const {
  UnDirection d(size, SplitDirection(robot));
  for (int i=0; i<size; ++i) {
    d[i].setRandom();
  }
  return d;
}


TEST_F(UnLineSearchTest, UnOCP) {
  auto cost = testhelper::CreateCost(robot);
  auto constraints = testhelper::CreateConstraints(robot);
  const auto s = createUnSolution(N+1);
  const auto d = createUnDirection(N+1);
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  std::vector<Robot> robots(nthreads, robot);
  auto ocp = UnOCP(robot, cost, constraints, N);
  for (int i=0; i<N; ++i) {
    ocp[i].initConstraints(robot, i, s[i]);
  }
  UnLineSearch line_search(robot, T, N, nthreads);
  EXPECT_TRUE(line_search.isFilterEmpty());
  const double max_primal_step_size = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double step_size = line_search.computeStepSize(ocp, robots, t, q, v, s, d, max_primal_step_size);
  EXPECT_TRUE(step_size <= max_primal_step_size);
  EXPECT_TRUE(step_size >= min_step_size);
  EXPECT_FALSE(line_search.isFilterEmpty());
}


TEST_F(UnLineSearchTest, UnParNMPC) {
  auto cost = testhelper::CreateCost(robot);
  auto constraints = testhelper::CreateConstraints(robot);
  const auto s = createUnSolution(N);
  const auto d = createUnDirection(N);
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  std::vector<Robot> robots(nthreads, robot);
  auto parnmpc = UnParNMPC(robot, cost, constraints, N);
  for (int i=0; i<N-1; ++i) {
    parnmpc[i].initConstraints(robot, i+1, s[i]);
  }
  parnmpc.terminal.initConstraints(robot, N, s[N-1]);
  UnLineSearch line_search(robot, T, N, nthreads);
  EXPECT_TRUE(line_search.isFilterEmpty());
  const double max_primal_step_size = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double step_size = line_search.computeStepSize(parnmpc, robots, t, q, v, s, d, max_primal_step_size);
  EXPECT_TRUE(step_size <= max_primal_step_size);
  EXPECT_TRUE(step_size >= min_step_size);
  EXPECT_FALSE(line_search.isFilterEmpty());
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}