#include <string>
#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/impulse/impulse_kkt_matrix.hpp"
#include "idocp/impulse/impulse_kkt_residual.hpp"
#include "idocp/impulse/impulse_split_direction.hpp"
#include "idocp/impulse/impulse_split_direction.hpp"
#include "idocp/ocp/riccati_factorization.hpp"
#include "idocp/impulse/impulse_forward_riccati_recursion_factorizer.hpp"


namespace idocp {

class ImpulseForwardRiccatiRecursionFactorizerTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
    fixed_base_robot = Robot(fixed_base_urdf);
    floating_base_robot = Robot(floating_base_urdf);
  }

  virtual void TearDown() {
  }

  static ImpulseKKTMatrix createKKTMatrix(const Robot& robot);
  static ImpulseKKTResidual createKKTResidual(const Robot& robot);
  static RiccatiFactorization createRiccatiFactorization(const Robot& robot);

  static void test(const Robot& robot);

  std::string fixed_base_urdf, floating_base_urdf;
  Robot fixed_base_robot, floating_base_robot;
};


ImpulseKKTMatrix ImpulseForwardRiccatiRecursionFactorizerTest::createKKTMatrix(const Robot& robot) {
  const int dimv = robot.dimv();
  Eigen::MatrixXd seed = Eigen::MatrixXd::Random(dimv, dimv);
  ImpulseKKTMatrix kkt_matrix(robot);
  kkt_matrix.Qqq() = seed * seed.transpose();
  kkt_matrix.Qqv().setRandom();
  kkt_matrix.Qvq() = kkt_matrix.Qqv().transpose();
  seed = Eigen::MatrixXd::Random(dimv, dimv);
  kkt_matrix.Qvv() = seed * seed.transpose();
  if (robot.has_floating_base()) {
    kkt_matrix.Fqq().setIdentity();
    kkt_matrix.Fqq().topLeftCorner(robot.dim_passive(), robot.dim_passive()).setRandom();
  }
  kkt_matrix.Fqv().setZero();
  kkt_matrix.Fvq().setRandom();
  kkt_matrix.Fvv().setRandom();
  return kkt_matrix;
}


ImpulseKKTResidual ImpulseForwardRiccatiRecursionFactorizerTest::createKKTResidual(const Robot& robot) {
  ImpulseKKTResidual kkt_residual(robot);
  kkt_residual.lx().setRandom();
  kkt_residual.Fx().setRandom();
  return kkt_residual;
}


RiccatiFactorization ImpulseForwardRiccatiRecursionFactorizerTest::createRiccatiFactorization(const Robot& robot) {
  RiccatiFactorization riccati(robot);
  const int dimv = robot.dimv();
  Eigen::MatrixXd seed = Eigen::MatrixXd::Random(dimv, dimv);
  riccati.Pqq = seed * seed.transpose();
  riccati.Pqv = Eigen::MatrixXd::Random(dimv, dimv);
  riccati.Pvq = riccati.Pqv.transpose();
  seed = Eigen::MatrixXd::Random(dimv, dimv);
  riccati.Pvv = seed * seed.transpose();
  riccati.sq.setRandom();
  riccati.sv.setRandom();
  const int dimx = 2 * robot.dimv();
  seed = Eigen::MatrixXd::Random(dimx, dimx);
  riccati.N = seed * seed.transpose();
  riccati.Pi.setRandom();
  riccati.pi.setRandom();
  return riccati; 
}


void ImpulseForwardRiccatiRecursionFactorizerTest::test(const Robot& robot) {
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  const ImpulseKKTMatrix kkt_matrix = createKKTMatrix(robot);
  if (!robot.has_floating_base()) {
    ASSERT_TRUE(kkt_matrix.Fqq().isZero());
  }
  ASSERT_TRUE(kkt_matrix.Fqv().isZero());
  const ImpulseKKTResidual kkt_residual = createKKTResidual(robot);
  ImpulseForwardRiccatiRecursionFactorizer factorizer(robot);
  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(2*dimv, 2*dimv);
  if (robot.has_floating_base()) {
    A.topLeftCorner(dimv, dimv) = kkt_matrix.Fqq();
  }
  else {
    A.topLeftCorner(dimv, dimv).setIdentity();
  }
  A.topRightCorner(dimv, dimv).setZero();
  A.bottomLeftCorner(dimv, dimv) = kkt_matrix.Fvq();
  A.bottomRightCorner(dimv, dimv) = kkt_matrix.Fvv();
  const RiccatiFactorization riccati = createRiccatiFactorization(robot);
  RiccatiFactorization riccati_next(robot);
  factorizer.factorizeStateTransition(riccati, kkt_matrix, kkt_residual, riccati_next);
  RiccatiFactorization riccati_next_ref(robot);
  riccati_next_ref.Pi = A * riccati.Pi;
  riccati_next_ref.pi = A * riccati.pi + kkt_residual.Fx();
  EXPECT_TRUE(riccati_next.Pi.isApprox(riccati_next_ref.Pi));
  EXPECT_TRUE(riccati_next.pi.isApprox(riccati_next_ref.pi));
  factorizer.factorizeStateConstraintFactorization(riccati, kkt_matrix, riccati_next);
  riccati_next_ref.N = A * riccati.N * A.transpose();
  std::cout << riccati_next_ref.Pi << std::endl;
  std::cout << riccati_next_ref.pi << std::endl;
  std::cout << riccati_next_ref.N << std::endl;
  std::cout << A << std::endl;
  std::cout << kkt_residual.Fx() << std::endl;
  EXPECT_TRUE(riccati_next.N.isApprox(riccati_next_ref.N));
}


TEST_F(ImpulseForwardRiccatiRecursionFactorizerTest, fixedBase) {
  test(fixed_base_robot);
}


TEST_F(ImpulseForwardRiccatiRecursionFactorizerTest, floating_base) {
  test(floating_base_robot);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}