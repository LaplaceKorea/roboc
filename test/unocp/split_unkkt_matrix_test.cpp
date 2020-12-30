#include <string>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/unocp/split_unkkt_matrix.hpp"


namespace idocp {

class SplitUnKKTMatrixTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    urdf = "../urdf/iiwa14/iiwa14.urdf";
    robot = Robot(urdf);
    dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
  }

  virtual void TearDown() {
  }

  std::string urdf;
  Robot robot;
  double dtau;
};


TEST_F(SplitUnKKTMatrixTest, invert) {
  SplitUnKKTMatrix matrix(robot);
  const int dimv = robot.dimv();
  const int dimx = 2*robot.dimv();
  const int dimQ = 3*robot.dimv();
  const int dimKKT = 5*robot.dimv();
  const Eigen::MatrixXd Q_seed_mat = Eigen::MatrixXd::Random(dimQ, dimQ);
  const Eigen::MatrixXd Q_mat = Q_seed_mat * Q_seed_mat.transpose() + Eigen::MatrixXd::Identity(dimQ, dimQ);
  matrix.Q = Q_mat;
  Eigen::MatrixXd KKT_mat_inv = Eigen::MatrixXd::Zero(dimKKT, dimKKT);
  matrix.invert(dtau, KKT_mat_inv);
  Eigen::MatrixXd KKT_mat_ref = Eigen::MatrixXd::Zero(dimKKT, dimKKT);
  KKT_mat_ref.bottomRightCorner(dimQ, dimQ) = Q_mat;
  KKT_mat_ref.block(   0, 3*dimv, dimv, dimv) = - Eigen::MatrixXd::Identity(dimv, dimv);
  KKT_mat_ref.block(   0, 4*dimv, dimv, dimv) = dtau * Eigen::MatrixXd::Identity(dimv, dimv);
  KKT_mat_ref.block(dimv, 2*dimv, dimv, dimv) = dtau * Eigen::MatrixXd::Identity(dimv, dimv);
  KKT_mat_ref.block(dimv, 4*dimv, dimv, dimv) = - Eigen::MatrixXd::Identity(dimv, dimv);
  KKT_mat_ref.block(dimx, 0, 3*dimv, 2*dimv) = KKT_mat_ref.block(0, dimx, 2*dimv, 3*dimv).transpose();
  const Eigen::MatrixXd KKT_mat_inv_ref = KKT_mat_ref.inverse();
  EXPECT_TRUE(KKT_mat_inv.isApprox(KKT_mat_inv_ref));
  EXPECT_TRUE((KKT_mat_inv*KKT_mat_ref).isIdentity());
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}