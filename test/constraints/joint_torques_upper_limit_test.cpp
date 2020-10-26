#include <string>
#include <random>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/constraints/joint_torques_upper_limit.hpp"
#include "idocp/constraints/pdipm.hpp"

namespace idocp {

class JointTorquesUpperLimitTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    fixed_base_urdf_ = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf_ = "../urdf/anymal/anymal.urdf";
    fixed_base_robot_ = Robot(fixed_base_urdf_);
    floating_base_robot_ = Robot(floating_base_urdf_);
    barrier_ = 1.0e-04;
    dtau_ = std::abs(Eigen::VectorXd::Random(1)[0]);
  }

  virtual void TearDown() {
  }

  double barrier_, dtau_;
  std::string fixed_base_urdf_, floating_base_urdf_;
  Robot fixed_base_robot_, floating_base_robot_;
};


TEST_F(JointTorquesUpperLimitTest, useKinematicsFixedBase) {
  JointTorquesUpperLimit limit(fixed_base_robot_); 
  EXPECT_FALSE(limit.useKinematics());
  EXPECT_EQ(limit.kinematicsLevel(), KinematicsLevel::AccelerationLevel);
}


TEST_F(JointTorquesUpperLimitTest, useKinematicsFloatingBase) {
  JointTorquesUpperLimit limit(floating_base_robot_); 
  EXPECT_FALSE(limit.useKinematics());
  EXPECT_EQ(limit.kinematicsLevel(), KinematicsLevel::AccelerationLevel);
}


TEST_F(JointTorquesUpperLimitTest, isFeasibleFixedBase) {
  JointTorquesUpperLimit limit(fixed_base_robot_); 
  ConstraintComponentData data(limit.dimc());
  SplitSolution s(fixed_base_robot_);
  EXPECT_TRUE(limit.isFeasible(fixed_base_robot_, data, s));
  s.u = 2*fixed_base_robot_.jointEffortLimit();
  EXPECT_FALSE(limit.isFeasible(fixed_base_robot_, data, s));
}


TEST_F(JointTorquesUpperLimitTest, isFeasibleFloatingBase) {
  JointTorquesUpperLimit limit(floating_base_robot_);
  ConstraintComponentData data(limit.dimc());
  SplitSolution s(floating_base_robot_);
  EXPECT_TRUE(limit.isFeasible(floating_base_robot_, data, s));
  const int dimc = floating_base_robot_.jointEffortLimit().size();
  s.u = 2*floating_base_robot_.jointEffortLimit();
  ASSERT_EQ(s.u.size(), floating_base_robot_.dimu());
  EXPECT_FALSE(limit.isFeasible(floating_base_robot_, data, s));
}


TEST_F(JointTorquesUpperLimitTest, setSlackAndDualFixedBase) {
  JointTorquesUpperLimit limit(fixed_base_robot_);
  ConstraintComponentData data(limit.dimc()), data_ref(limit.dimc());
  const int dimq = fixed_base_robot_.dimq();
  const int dimv = fixed_base_robot_.dimv();
  const int dimu = fixed_base_robot_.dimu();
  SplitSolution s(fixed_base_robot_);
  Eigen::VectorXd umax = fixed_base_robot_.jointEffortLimit();
  ASSERT_EQ(dimq, fixed_base_robot_.jointEffortLimit().size());
  s = SplitSolution::Random(fixed_base_robot_);
  limit.setSlackAndDual(fixed_base_robot_, data, dtau_, s);
  KKTMatrix kkt_matrix(fixed_base_robot_);
  KKTResidual kkt_residual(fixed_base_robot_);
  limit.augmentDualResidual(fixed_base_robot_, data, dtau_, s, kkt_residual);
  data_ref.slack = dtau_ * (umax-s.u);
  Eigen::VectorXd lu_ref = Eigen::VectorXd::Zero(dimu);
  pdipm::SetSlackAndDualPositive(barrier_, data_ref);
  lu_ref = dtau_ * data_ref.dual;
  EXPECT_TRUE(kkt_residual.lu().isApprox(lu_ref));
  EXPECT_TRUE(kkt_residual.la.isZero());
  EXPECT_TRUE(kkt_residual.lf().isZero());
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_TRUE(kkt_residual.lv().isZero());
  const double cost_slack_barrier = limit.costSlackBarrier(data);
  const double cost_slack_barrier_ref = pdipm::CostBarrier(barrier_, data_ref.slack);
  EXPECT_DOUBLE_EQ(cost_slack_barrier, cost_slack_barrier_ref);
  limit.computePrimalAndDualResidual(fixed_base_robot_, data, dtau_, s);
  const double l1residual = limit.l1NormPrimalResidual(data);
  const double l1residual_ref = (dtau_*(s.u-umax)+data_ref.slack).lpNorm<1>();
  EXPECT_DOUBLE_EQ(l1residual, l1residual_ref);
  const double l2residual = limit.squaredNormPrimalAndDualResidual(data);
  pdipm::ComputeDuality(barrier_, data_ref);
  const double l2residual_ref 
      = (dtau_*(s.u-umax)+data_ref.slack).squaredNorm() + data_ref.duality.squaredNorm();
  EXPECT_DOUBLE_EQ(l2residual, l2residual_ref);
}


TEST_F(JointTorquesUpperLimitTest, setSlackAndDualFloatingBase) {
  JointTorquesUpperLimit limit(floating_base_robot_);
  ConstraintComponentData data(limit.dimc()), data_ref(limit.dimc());
  const int dimq = floating_base_robot_.dimq();
  const int dimv = floating_base_robot_.dimv();
  const int dimu = floating_base_robot_.dimu();
  SplitSolution s(floating_base_robot_);
  Eigen::VectorXd umax = floating_base_robot_.jointEffortLimit();
  const int dimc = floating_base_robot_.jointEffortLimit().size();
  ASSERT_EQ(dimc, dimu);
  s = SplitSolution::Random(floating_base_robot_);
  limit.setSlackAndDual(floating_base_robot_, data, dtau_, s);
  KKTMatrix kkt_matrix(floating_base_robot_);
  KKTResidual kkt_residual(floating_base_robot_);
  limit.augmentDualResidual(floating_base_robot_, data, dtau_, s, kkt_residual);
  data_ref.slack = dtau_ * (umax-s.u);
  Eigen::VectorXd lu_ref = Eigen::VectorXd::Zero(dimu);
  pdipm::SetSlackAndDualPositive(barrier_, data_ref);
  lu_ref = dtau_ * data_ref.dual;
  EXPECT_TRUE(kkt_residual.lu().isApprox(lu_ref));
  EXPECT_TRUE(kkt_residual.la.isZero());
  EXPECT_TRUE(kkt_residual.lf().isZero());
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_TRUE(kkt_residual.lv().isZero());
  const double cost_slack_barrier = limit.costSlackBarrier(data);
  const double cost_slack_barrier_ref = pdipm::CostBarrier(barrier_, data_ref.slack);
  EXPECT_DOUBLE_EQ(cost_slack_barrier, cost_slack_barrier_ref);
  limit.computePrimalAndDualResidual(floating_base_robot_, data, dtau_, s);
  const double l1residual = limit.l1NormPrimalResidual(data);
  const double l1residual_ref = (dtau_*(s.u-umax)+data_ref.slack).lpNorm<1>();
  EXPECT_DOUBLE_EQ(l1residual, l1residual_ref);
  const double l2residual = limit.squaredNormPrimalAndDualResidual(data);
  Eigen::VectorXd duality_ref = Eigen::VectorXd::Zero(dimc);
  pdipm::ComputeDuality(barrier_, data_ref);
  const double l2residual_ref 
      = (dtau_*(s.u-umax)+data_ref.slack).squaredNorm() + data_ref.duality.squaredNorm();
  EXPECT_DOUBLE_EQ(l2residual, l2residual_ref);
}


TEST_F(JointTorquesUpperLimitTest, condenseSlackAndDualFixedBase) {
  JointTorquesUpperLimit limit(fixed_base_robot_);
  ConstraintComponentData data(limit.dimc()), data_ref(limit.dimc());
  const int dimq = fixed_base_robot_.dimq();
  const int dimv = fixed_base_robot_.dimv();
  const int dimu = fixed_base_robot_.dimu();
  SplitSolution s(fixed_base_robot_);
  Eigen::VectorXd umax = fixed_base_robot_.jointEffortLimit();
  ASSERT_EQ(dimq, fixed_base_robot_.jointEffortLimit().size());
  s = SplitSolution::Random(fixed_base_robot_);
  KKTMatrix kkt_matrix(fixed_base_robot_);
  KKTResidual kkt_residual(fixed_base_robot_);
  limit.setSlackAndDual(fixed_base_robot_, data, dtau_, s);
  data_ref.slack = dtau_ * (umax-s.u);
  pdipm::SetSlackAndDualPositive(barrier_, data_ref);
  limit.condenseSlackAndDual(fixed_base_robot_, data, dtau_, s, kkt_matrix, kkt_residual);
  data_ref.residual = dtau_ * (s.u-umax) + data_ref.slack;
  data_ref.duality = Eigen::VectorXd::Zero(dimu);
  pdipm::ComputeDuality(barrier_, data_ref);
  Eigen::VectorXd lu_ref = Eigen::VectorXd::Zero(dimu);
  lu_ref.array() 
      += dtau_ * (data_ref.dual.array()*data_ref.residual.array()-data_ref.duality.array()) 
               / data_ref.slack.array();
  Eigen::MatrixXd Quu_ref = Eigen::MatrixXd::Zero(dimu, dimu);
  for (int i=0; i<dimu; ++i) {
    Quu_ref(i, i) += dtau_ * dtau_ * data_ref.dual.coeff(i) / data_ref.slack.coeff(i);
  }
  EXPECT_TRUE(kkt_residual.lu().isApprox(lu_ref));
  EXPECT_TRUE(kkt_residual.la.isZero());
  EXPECT_TRUE(kkt_residual.lf().isZero());
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_TRUE(kkt_residual.lv().isZero());
  EXPECT_TRUE(kkt_matrix.Quu().isApprox(Quu_ref));
  EXPECT_TRUE(kkt_matrix.Qqq().isZero());
  EXPECT_TRUE(kkt_matrix.Qaa().isZero());
  EXPECT_TRUE(kkt_matrix.Qff().isZero());
  EXPECT_TRUE(kkt_matrix.Qvv().isZero());
  SplitDirection d = SplitDirection::Random(fixed_base_robot_);
  limit.computeSlackAndDualDirection(fixed_base_robot_, data, dtau_, s, d);
  data_ref.dslack = - dtau_ * d.du() - data_ref.residual;
  pdipm::ComputeDualDirection(data_ref);
  const double margin_rate = 0.995;
  const double slack_step_size = limit.maxSlackStepSize(data);
  const double dual_step_size = limit.maxDualStepSize(data);
  const double slack_step_size_ref 
      = pdipm::FractionToBoundarySlack(margin_rate, data_ref);
  const double dual_step_size_ref 
      = pdipm::FractionToBoundaryDual(margin_rate, data_ref);
  EXPECT_DOUBLE_EQ(slack_step_size, slack_step_size_ref);
  EXPECT_DOUBLE_EQ(dual_step_size, dual_step_size_ref);
  const double step_size = std::min(slack_step_size, dual_step_size); 
  const double berrier = limit.costSlackBarrier(data, step_size);
  const double berrier_ref 
      = pdipm::CostBarrier(barrier_, data_ref.slack+step_size*data_ref.dslack);
  EXPECT_DOUBLE_EQ(berrier, berrier_ref);
  limit.updateSlack(data, step_size);
  limit.updateDual(data, step_size);
  const double cost_slack_barrier = limit.costSlackBarrier(data);
  data_ref.slack += step_size * data_ref.dslack;
  data_ref.dual += step_size * data_ref.ddual;
  const double cost_slack_barrier_ref = pdipm::CostBarrier(barrier_, data_ref.slack);
  EXPECT_DOUBLE_EQ(cost_slack_barrier, cost_slack_barrier_ref);
  kkt_residual.lu().setZero();
  lu_ref.setZero();
  limit.augmentDualResidual(floating_base_robot_, data, dtau_, s, kkt_residual);
  lu_ref = dtau_ * data_ref.dual;
  EXPECT_TRUE(kkt_residual.lu().isApprox(lu_ref));
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_TRUE(kkt_residual.la.isZero());
  EXPECT_TRUE(kkt_residual.lf().isZero());
  EXPECT_TRUE(kkt_residual.lv().isZero());
}


TEST_F(JointTorquesUpperLimitTest, condenseSlackAndDualFloatingBase) {
  JointTorquesUpperLimit limit(floating_base_robot_);
  ConstraintComponentData data(limit.dimc()), data_ref(limit.dimc());
  const int dimq = floating_base_robot_.dimq();
  const int dimv = floating_base_robot_.dimv();
  const int dimu = floating_base_robot_.dimu();
  SplitSolution s(floating_base_robot_);
  Eigen::VectorXd umax = floating_base_robot_.jointEffortLimit();
  const int dimc = floating_base_robot_.jointEffortLimit().size();
  s = SplitSolution::Random(floating_base_robot_);
  KKTMatrix kkt_matrix(floating_base_robot_);
  KKTResidual kkt_residual(floating_base_robot_);
  limit.setSlackAndDual(floating_base_robot_, data, dtau_, s);
  data_ref.slack = dtau_ * (umax-s.u);
  pdipm::SetSlackAndDualPositive(barrier_, data_ref);
  limit.condenseSlackAndDual(floating_base_robot_, data, dtau_, s, kkt_matrix, kkt_residual);
  data_ref.residual = dtau_ * (s.u-umax) + data_ref.slack;
  pdipm::ComputeDuality(barrier_, data_ref);
  Eigen::VectorXd lu_ref = Eigen::VectorXd::Zero(dimu);
  lu_ref.array() 
      += dtau_ * (data_ref.dual.array()*data_ref.residual.array()-data_ref.duality.array()) 
               / data_ref.slack.array();
  Eigen::MatrixXd Quu_ref = Eigen::MatrixXd::Zero(dimu, dimu);
  for (int i=0; i<dimu; ++i) {
    Quu_ref(i, i) += dtau_ * dtau_ * data_ref.dual.coeff(i) / data_ref.slack.coeff(i);
  }
  EXPECT_TRUE(kkt_residual.lu().isApprox(lu_ref));
  EXPECT_TRUE(kkt_residual.la.isZero());
  EXPECT_TRUE(kkt_residual.lf().isZero());
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_TRUE(kkt_residual.lu().isZero());
  EXPECT_TRUE(kkt_matrix.Quu().isApprox(Quu_ref));
  EXPECT_TRUE(kkt_matrix.Qqq().isZero());
  EXPECT_TRUE(kkt_matrix.Qaa().isZero());
  EXPECT_TRUE(kkt_matrix.Qff().isZero());
  EXPECT_TRUE(kkt_matrix.Qvv().isZero());
  SplitDirection d = SplitDirection::Random(floating_base_robot_);
  limit.computeSlackAndDualDirection(floating_base_robot_, data, dtau_, s, d);
  data_ref.dslack = - dtau_ * d.du() - data_ref.residual;
  pdipm::ComputeDualDirection(data_ref);
  const double margin_rate = 0.995;
  const double slack_step_size = limit.maxSlackStepSize(data);
  const double dual_step_size = limit.maxDualStepSize(data);
  const double slack_step_size_ref 
      = pdipm::FractionToBoundarySlack(margin_rate, data_ref);
  const double dual_step_size_ref 
      = pdipm::FractionToBoundaryDual(margin_rate, data_ref);
  EXPECT_DOUBLE_EQ(slack_step_size, slack_step_size_ref);
  EXPECT_DOUBLE_EQ(dual_step_size, dual_step_size_ref);
  const double step_size = std::min(slack_step_size, dual_step_size); 
  const double berrier = limit.costSlackBarrier(data, step_size);
  const double berrier_ref 
      = pdipm::CostBarrier(barrier_, data_ref.slack+step_size*data_ref.dslack);
  EXPECT_DOUBLE_EQ(berrier, berrier_ref);
  limit.updateSlack(data, step_size);
  limit.updateDual(data, step_size);
  const double cost_slack_barrier = limit.costSlackBarrier(data);
  data_ref.slack += step_size * data_ref.dslack;
  data_ref.dual += step_size * data_ref.ddual;
  const double cost_slack_barrier_ref = pdipm::CostBarrier(barrier_, data_ref.slack);
  EXPECT_DOUBLE_EQ(cost_slack_barrier, cost_slack_barrier_ref);
  kkt_residual.lu().setZero();
  lu_ref.setZero();
  limit.augmentDualResidual(floating_base_robot_, data, dtau_, s, kkt_residual);
  lu_ref = dtau_ * data_ref.dual;
  EXPECT_TRUE(kkt_residual.lu().isApprox(lu_ref));
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_TRUE(kkt_residual.la.isZero());
  EXPECT_TRUE(kkt_residual.lf().isZero());
  EXPECT_TRUE(kkt_residual.lv().isZero());
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}