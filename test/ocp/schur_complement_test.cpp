#include <random>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/ocp/schur_complement.hpp"


namespace idocp {

class SchurComplementTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
  }

  virtual void TearDown() {
  }
};


TEST_F(SchurComplementTest, invertWithZeroBottomRightCorner) {
  const int max_dimA = 100;
  const int max_dimD = 50;
  SchurComplement schur_complement(max_dimA, max_dimD);
  const int dimA = 70;
  const int dimD = 30;
  const Eigen::MatrixXd M_seed = Eigen::MatrixXd::Random(dimA+dimD, dimA+dimD);
  Eigen::MatrixXd M = M_seed * M_seed.transpose() + Eigen::MatrixXd::Identity(dimA+dimD, dimA+dimD);
  M.bottomRightCorner(dimD, dimD).setZero();
  const Eigen::MatrixXd Minv_ref = M.inverse();
  const Eigen::MatrixXd A = M.topLeftCorner(dimA, dimA);
  const Eigen::MatrixXd C = M.bottomLeftCorner(dimD, dimA);
  Eigen::MatrixXd Minv = Eigen::MatrixXd::Zero(dimA+dimD, dimA+dimD);
  schur_complement.invertWithZeroBottomRightCorner(dimA, dimD, A, C, Minv);
  EXPECT_TRUE(Minv.isApprox(Minv_ref));
  EXPECT_TRUE((Minv*M).isIdentity());
  EXPECT_TRUE((M*Minv).isIdentity());
}


TEST_F(SchurComplementTest, invertWithZeroTopLeftCorner) {
  const int max_dimA = 50;
  const int max_dimD = 100;
  SchurComplement schur_complement(max_dimA, max_dimD);
  const int dimA = 30;
  const int dimD = 70;
  const Eigen::MatrixXd M_seed = Eigen::MatrixXd::Random(dimA+dimD, dimA+dimD);
  Eigen::MatrixXd M = M_seed * M_seed.transpose() + Eigen::MatrixXd::Identity(dimA+dimD, dimA+dimD);
  M.topLeftCorner(dimA, dimA).setZero();
  const Eigen::MatrixXd Minv_ref = M.inverse();
  const Eigen::MatrixXd B = M.topRightCorner(dimA, dimD);
  const Eigen::MatrixXd D = M.bottomRightCorner(dimD, dimD);
  Eigen::MatrixXd Minv = Eigen::MatrixXd::Zero(dimA+dimD, dimA+dimD);
  schur_complement.invertWithZeroTopLeftCorner(dimA, dimD, B, D, Minv);
  EXPECT_TRUE(Minv.isApprox(Minv_ref));
  EXPECT_TRUE((Minv*M).isIdentity());
  EXPECT_TRUE((M*Minv).isIdentity());
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}