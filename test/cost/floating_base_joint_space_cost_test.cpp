#include <string>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/cost/joint_space_cost.hpp"
#include "idocp/cost/cost_function_data.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/kkt_matrix.hpp"


namespace idocp {

class FloatingBaseJointSpaceCostTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    urdf_ = "../urdf/anymal/anymal.urdf";
    robot_ = Robot(urdf_);
    dtau_ = std::abs(Eigen::VectorXd::Random(1)[0]);
    t_ = std::abs(Eigen::VectorXd::Random(1)[0]);
    data_ = CostFunctionData(robot_);
    s = SplitSolution(robot_);
    kkt_res = KKTResidual(robot_);
    kkt_mat = KKTMatrix(robot_);
  }

  virtual void TearDown() {
  }

  double dtau_, t_;
  std::string urdf_;
  Robot robot_;
  ContactStatus contact_status_;
  CostFunctionData data_;
  SplitSolution s;
  KKTResidual kkt_res;
  KKTMatrix kkt_mat;
};


TEST_F(FloatingBaseJointSpaceCostTest, setWeights) {
  const int dimq = robot_.dimq();
  const int dimv = robot_.dimv();
  const int dimu = robot_.dimu();
  const Eigen::VectorXd q_weight = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd v_weight = Eigen::VectorXd::Random(dimv); 
  const Eigen::VectorXd a_weight = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd u_weight = Eigen::VectorXd::Random(dimu);
  const Eigen::VectorXd qf_weight = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd vf_weight = Eigen::VectorXd::Random(dimv);
  Eigen::VectorXd q_ref = Eigen::VectorXd::Zero(dimq);
  robot_.generateFeasibleConfiguration(q_ref);
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(dimv); 
  const Eigen::VectorXd a_ref = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd u_ref = Eigen::VectorXd::Random(dimu);
  JointSpaceCost cost(robot_);
  EXPECT_FALSE(cost.useKinematics());
  cost.set_q_weight(q_weight);
  cost.set_v_weight(v_weight);
  cost.set_a_weight(a_weight);
  cost.set_u_weight(u_weight);
  cost.set_qf_weight(qf_weight);
  cost.set_vf_weight(vf_weight);
  cost.set_q_ref(q_ref);
  cost.set_v_ref(v_ref);
  cost.set_a_ref(a_ref);
  cost.set_u_ref(u_ref);
  s = SplitSolution::Random(robot_);
  Eigen::VectorXd q_diff = Eigen::VectorXd::Zero(dimv); 
  robot_.subtractConfiguration(s.q, q_ref, q_diff);
  const double l_ref = 0.5 * dtau_ 
                           * ((q_weight.array()*q_diff.array()*q_diff.array()).sum()
                            + (v_weight.array()* (s.v-v_ref).array()*(s.v-v_ref).array()).sum()
                            + (a_weight.array()* (s.a-a_ref).array()*(s.a-a_ref).array()).sum()
                            + (u_weight.array()* (s.u-u_ref).array()*(s.u-u_ref).array()).sum());
  EXPECT_DOUBLE_EQ(cost.l(robot_, data_, t_, dtau_, s), l_ref);
  const double phi_ref = 0.5 * ((qf_weight.array()* q_diff.array()*q_diff.array()).sum()
                                + (vf_weight.array()* (s.v-v_ref).array()*(s.v-v_ref).array()).sum());
  EXPECT_DOUBLE_EQ(cost.phi(robot_, data_, t_, s), phi_ref);
  Eigen::VectorXd lq_ref = Eigen::VectorXd::Zero(dimv);
  Eigen::VectorXd lv_ref = Eigen::VectorXd::Zero(dimv);
  Eigen::VectorXd la_ref = Eigen::VectorXd::Zero(dimv);
  Eigen::VectorXd lu_ref = Eigen::VectorXd::Zero(dimu);
  cost.lq(robot_, data_, t_, dtau_, s, kkt_res);
  Eigen::MatrixXd Jq_diff = Eigen::MatrixXd::Zero(dimv, dimv);
  robot_.dSubtractdConfigurationPlus(s.q, q_ref, Jq_diff);
  lq_ref = dtau_ * Jq_diff.transpose() * q_weight.asDiagonal() * q_diff;
  EXPECT_TRUE(kkt_res.lq().isApprox(lq_ref));
  cost.lv(robot_, data_, t_, dtau_, s, kkt_res);
  lv_ref = dtau_ * v_weight.asDiagonal() * (s.v-v_ref);
  EXPECT_TRUE(kkt_res.lv().isApprox(lv_ref));
  cost.la(robot_, data_, t_, dtau_, s, kkt_res);
  la_ref = dtau_ * a_weight.asDiagonal() * (s.a-a_ref);
  EXPECT_TRUE(kkt_res.la.isApprox(la_ref));
  cost.lu(robot_, data_, t_, dtau_, s, kkt_res);
  lu_ref = dtau_ * u_weight.asDiagonal() * (s.u-u_ref);
  EXPECT_TRUE(kkt_res.lu().isApprox(lu_ref));
  cost.phiq(robot_, data_, t_, s, kkt_res);
  lq_ref += Jq_diff.transpose() * qf_weight.asDiagonal() * q_diff;
  EXPECT_TRUE(kkt_res.lq().isApprox(lq_ref));
  cost.phiv(robot_, data_, t_, s, kkt_res);
  lv_ref += vf_weight.asDiagonal() * (s.v-v_ref);
  EXPECT_TRUE(kkt_res.lv().isApprox(lv_ref));
  Eigen::MatrixXd lqq_ref = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd lvv_ref = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd laa_ref = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd luu_ref = Eigen::MatrixXd::Zero(dimu, dimu);
  lqq_ref = dtau_ * Jq_diff.transpose() * q_weight.asDiagonal() * Jq_diff;
  lvv_ref = dtau_ * v_weight.asDiagonal();
  laa_ref = dtau_ * a_weight.asDiagonal();
  luu_ref = dtau_ * u_weight.asDiagonal();
  cost.lqq(robot_, data_, t_, dtau_, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.Qqq().isApprox(lqq_ref));
  cost.lvv(robot_, data_, t_, dtau_, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.Qvv().isApprox(lvv_ref));
  cost.laa(robot_, data_, t_, dtau_, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.Qaa().isApprox(laa_ref));
  cost.luu(robot_, data_, t_, dtau_, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.Quu().isApprox(luu_ref));
  lqq_ref += Jq_diff.transpose() * qf_weight.asDiagonal() * Jq_diff;
  lvv_ref += vf_weight.asDiagonal();
  cost.phiqq(robot_, data_, t_, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.Qqq().isApprox(lqq_ref));
  cost.phivv(robot_, data_, t_, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.Qvv().isApprox(lvv_ref));
}


} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}