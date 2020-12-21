#include <string>
#include <memory>
#include <limits>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/split_kkt_matrix.hpp"
#include "idocp/ocp/split_kkt_residual.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/split_riccati_factorization.hpp"
#include "idocp/ocp/lqr_state_feedback_policy.hpp"
#include "idocp/ocp/backward_riccati_recursion_factorizer.hpp"
#include "idocp/ocp/split_riccati_factorizer.hpp"

namespace idocp {

class SplitRiccatiFactorizerTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
    dtau = std::abs(Eigen::VectorXd::Random(1)[0]) + std::sqrt(std::numeric_limits<double>::epsilon());
  }

  virtual void TearDown() {
  }

  SplitKKTMatrix createKKTMatrix(const Robot& robot) const;
  SplitKKTResidual createKKTResidual(const Robot& robot) const;
  SplitRiccatiFactorization createRiccatiFactorization(const Robot& robot) const;


  void testBackwardRecursion(const Robot& robot) const;
  void testForwardStateConstraintFactorization(const Robot& robot) const;
  void testBackwardStateConstraintFactorization(const Robot& robot) const;
  void testForwardRecursion(const Robot& robot) const;
  void testNearZeroDtau(const Robot& robot) const;

  double dtau;
  std::string fixed_base_urdf, floating_base_urdf;
};


SplitKKTMatrix SplitRiccatiFactorizerTest::createKKTMatrix(const Robot& robot) const {
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  Eigen::MatrixXd seed = Eigen::MatrixXd::Random(dimv, dimv);
  SplitKKTMatrix kkt_matrix(robot);
  kkt_matrix.Qqq() = seed * seed.transpose();
  kkt_matrix.Qqv().setRandom();
  kkt_matrix.Qvq() = kkt_matrix.Qqv().transpose();
  seed = Eigen::MatrixXd::Random(dimv, dimv);
  kkt_matrix.Qvv() = seed * seed.transpose();
  kkt_matrix.Qqu().setRandom();
  kkt_matrix.Qvu().setRandom();
  seed = Eigen::MatrixXd::Random(dimu, dimu);
  kkt_matrix.Quu() = seed * seed.transpose();
  kkt_matrix.Fqq() = Eigen::MatrixXd::Identity(dimv, dimv);
  if (robot.hasFloatingBase()) {
    kkt_matrix.Fqq().topLeftCorner(robot.dim_passive(), robot.dim_passive()).setRandom();
  }
  kkt_matrix.Fqv() = dtau * Eigen::MatrixXd::Identity(dimv, dimv);
  kkt_matrix.Fvq().setRandom();
  kkt_matrix.Fvv().setRandom();
  kkt_matrix.Fvu().setRandom();
  return kkt_matrix;
}


SplitKKTResidual SplitRiccatiFactorizerTest::createKKTResidual(const Robot& robot) const {
  SplitKKTResidual kkt_residual(robot);
  kkt_residual.lx().setRandom();
  kkt_residual.lu().setRandom();
  kkt_residual.Fx().setRandom();
  return kkt_residual;
}


SplitRiccatiFactorization SplitRiccatiFactorizerTest::createRiccatiFactorization(const Robot& robot) const {
  SplitRiccatiFactorization riccati(robot);
  const int dimv = robot.dimv();
  Eigen::MatrixXd seed = Eigen::MatrixXd::Random(dimv, dimv);
  riccati.Pqq = seed * seed.transpose();
  riccati.Pqv = Eigen::MatrixXd::Random(dimv, dimv);
  riccati.Pvq = riccati.Pqv.transpose();
  seed = Eigen::MatrixXd::Random(dimv, dimv);
  riccati.Pvv = seed * seed.transpose();
  riccati.sq.setRandom();
  riccati.sv.setRandom();
  riccati.Pi.setRandom();
  riccati.pi.setRandom();
  riccati.N.setRandom();
  riccati.N = (riccati.N + riccati.N.transpose()).eval();
  riccati.n.setRandom();
  return riccati; 
}


void SplitRiccatiFactorizerTest::testBackwardRecursion(const Robot& robot) const {
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  const SplitRiccatiFactorization riccati_next = createRiccatiFactorization(robot);
  SplitKKTMatrix kkt_matrix = createKKTMatrix(robot);
  SplitKKTResidual kkt_residual = createKKTResidual(robot);
  SplitKKTMatrix kkt_matrix_ref = kkt_matrix;
  SplitKKTResidual kkt_residual_ref = kkt_residual;
  SplitRiccatiFactorizer factorizer(robot);
  LQRStateFeedbackPolicy lqr_policy_ref(robot);
  BackwardRiccatiRecursionFactorizer backward_recursion_ref(robot);
  SplitRiccatiFactorization riccati = createRiccatiFactorization(robot);
  SplitRiccatiFactorization riccati_ref = riccati;
  factorizer.backwardRiccatiRecursion(riccati_next, dtau, kkt_matrix, kkt_residual, riccati);
  backward_recursion_ref.factorizeKKTMatrix(riccati_next, dtau, kkt_matrix_ref, kkt_residual_ref);
  Eigen::MatrixXd Ginv = kkt_matrix_ref.Quu().inverse();
  lqr_policy_ref.K = - Ginv  * kkt_matrix_ref.Qxu().transpose();
  lqr_policy_ref.k = - Ginv  * kkt_residual.lu();
  backward_recursion_ref.factorizeRiccatiFactorization(riccati_next, kkt_matrix_ref, kkt_residual_ref, lqr_policy_ref, dtau, riccati_ref);
  EXPECT_TRUE(riccati.Pqq.isApprox(riccati_ref.Pqq));
  EXPECT_TRUE(riccati.Pqv.isApprox(riccati_ref.Pqv));
  EXPECT_TRUE(riccati.Pvq.isApprox(riccati_ref.Pvq));
  EXPECT_TRUE(riccati.Pvv.isApprox(riccati_ref.Pvv));
  EXPECT_TRUE(riccati.sq.isApprox(riccati_ref.sq));
  EXPECT_TRUE(riccati.sv.isApprox(riccati_ref.sv));
  EXPECT_TRUE(riccati.Pqq.isApprox(riccati.Pqq.transpose()));
  EXPECT_TRUE(riccati.Pvv.isApprox(riccati.Pvv.transpose()));
  EXPECT_TRUE(riccati.Pvq.isApprox(riccati.Pqv.transpose()));
  EXPECT_TRUE(kkt_matrix.Qxx().isApprox(kkt_matrix.Qxx().transpose()));
  EXPECT_TRUE(kkt_matrix.Quu().isApprox(kkt_matrix.Quu().transpose()));
  Eigen::MatrixXd K(robot.dimu(), 2*robot.dimv());
  factorizer.getStateFeedbackGain(K);
  EXPECT_TRUE(lqr_policy_ref.K.isApprox(K));
  Eigen::MatrixXd Kq(dimu, dimv);
  Eigen::MatrixXd Kv(dimu, dimv);
  factorizer.getStateFeedbackGain(Kq, Kv);
  EXPECT_TRUE(lqr_policy_ref.K.leftCols(dimv).isApprox(Kq));
  EXPECT_TRUE(lqr_policy_ref.K.rightCols(dimv).isApprox(Kv));
}


void SplitRiccatiFactorizerTest::testForwardStateConstraintFactorization(const Robot& robot) const {
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  SplitRiccatiFactorization riccati_next = createRiccatiFactorization(robot);
  SplitKKTMatrix kkt_matrix = createKKTMatrix(robot);
  SplitKKTResidual kkt_residual = createKKTResidual(robot);
  SplitKKTMatrix kkt_matrix_ref = kkt_matrix;
  SplitKKTResidual kkt_residual_ref = kkt_residual;
  SplitRiccatiFactorizer factorizer(robot);
  LQRStateFeedbackPolicy lqr_policy_ref(robot);
  BackwardRiccatiRecursionFactorizer backward_recursion_ref(robot);
  SplitRiccatiFactorization riccati = createRiccatiFactorization(robot);
  SplitRiccatiFactorization riccati_ref = riccati;
  factorizer.backwardRiccatiRecursion(riccati_next, dtau, kkt_matrix, kkt_residual, riccati);
  backward_recursion_ref.factorizeKKTMatrix(riccati_next, dtau, kkt_matrix_ref, kkt_residual_ref);
  const Eigen::MatrixXd Ginv = kkt_matrix_ref.Quu().inverse();
  lqr_policy_ref.K = - Ginv  * kkt_matrix_ref.Qxu().transpose();
  lqr_policy_ref.k = - Ginv  * kkt_residual.lu();
  SplitRiccatiFactorization riccati_next_ref = riccati_next;
  backward_recursion_ref.factorizeRiccatiFactorization(riccati_next, kkt_matrix_ref, kkt_residual_ref, lqr_policy_ref, dtau, riccati_ref);
  factorizer.forwardRiccatiRecursionParallel(kkt_matrix, kkt_residual, true);
  kkt_matrix_ref.Fxx() += kkt_matrix_ref.Fxu() * lqr_policy_ref.K;
  kkt_residual_ref.Fx() += kkt_matrix_ref.Fxu() * lqr_policy_ref.k;
  const Eigen::MatrixXd BGinvBt = kkt_matrix_ref.Fxu() * Ginv * kkt_matrix_ref.Fxu().transpose();
  EXPECT_TRUE(kkt_matrix_ref.isApprox(kkt_matrix));
  EXPECT_TRUE(kkt_residual_ref.isApprox(kkt_residual));
  factorizer.forwardStateConstraintFactorization(riccati, kkt_matrix, kkt_residual, dtau, riccati_next, false);
  riccati_next_ref.Pi = kkt_matrix_ref.Fxx() * riccati.Pi;
  riccati_next_ref.pi = kkt_residual_ref.Fx() + kkt_matrix_ref.Fxx() * riccati.pi;
  EXPECT_TRUE(riccati_next.Pi.isApprox(riccati_next_ref.Pi));
  EXPECT_TRUE(riccati_next.pi.isApprox(riccati_next_ref.pi));
  EXPECT_TRUE(riccati_next.N.isApprox(riccati_next_ref.N));
  factorizer.forwardStateConstraintFactorization(riccati, kkt_matrix, kkt_residual, dtau, riccati_next, true);
  riccati_next_ref.Pi = kkt_matrix_ref.Fxx() * riccati.Pi;
  riccati_next_ref.pi = kkt_residual_ref.Fx() + kkt_matrix_ref.Fxx() * riccati.pi;
  riccati_next_ref.N = BGinvBt + kkt_matrix_ref.Fxx() * riccati.N * kkt_matrix_ref.Fxx().transpose();
  EXPECT_TRUE(riccati_next.Pi.isApprox(riccati_next_ref.Pi));
  EXPECT_TRUE(riccati_next.pi.isApprox(riccati_next_ref.pi));
  EXPECT_TRUE(riccati_next.N.isApprox(riccati_next_ref.N));
}


void SplitRiccatiFactorizerTest::testBackwardStateConstraintFactorization(const Robot& robot) const {
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  SplitKKTMatrix kkt_matrix = createKKTMatrix(robot);
  SplitKKTResidual kkt_residual = createKKTResidual(robot);
  SplitRiccatiFactorization riccati_next = createRiccatiFactorization(robot);
  SplitRiccatiFactorization riccati = createRiccatiFactorization(robot);
  SplitRiccatiFactorizer factorizer(robot);
  factorizer.backwardRiccatiRecursion(riccati_next, dtau, kkt_matrix, kkt_residual, riccati);
  const int dimf = 6;
  Eigen::MatrixXd T_next(2*dimv, dimf), T(2*dimv, dimf), T_ref(2*dimv, dimf);
  T_next.setRandom();
  T.setRandom();
  T_ref = T;
  factorizer.backwardStateConstraintFactorization(T_next, kkt_matrix, dtau, T);
  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(2*dimv, 2*dimv);
  if (robot.hasFloatingBase()) {
    A.topLeftCorner(dimv, dimv) = kkt_matrix.Fqq();
  }
  else {
    A.topLeftCorner(dimv, dimv).setIdentity();
  }
  A.topRightCorner(dimv, dimv) = dtau * Eigen::MatrixXd::Identity(dimv, dimv);
  A.bottomLeftCorner(dimv, dimv) = kkt_matrix.Fvq();
  A.bottomRightCorner(dimv, dimv) = kkt_matrix.Fvv();
  T_ref = A.transpose() * T_next;
  EXPECT_TRUE(T.isApprox(T_ref));
}


void SplitRiccatiFactorizerTest::testForwardRecursion(const Robot& robot) const {
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  SplitRiccatiFactorization riccati_next = createRiccatiFactorization(robot);
  SplitRiccatiFactorization riccati_next_ref = riccati_next;
  SplitKKTMatrix kkt_matrix = createKKTMatrix(robot);
  SplitKKTResidual kkt_residual = createKKTResidual(robot);
  SplitKKTMatrix kkt_matrix_ref = kkt_matrix;
  SplitKKTResidual kkt_residual_ref = kkt_residual;
  SplitRiccatiFactorizer factorizer(robot);
  LQRStateFeedbackPolicy lqr_policy_ref(robot);
  BackwardRiccatiRecursionFactorizer backward_recursion_ref(robot);
  SplitRiccatiFactorization riccati = createRiccatiFactorization(robot);
  SplitRiccatiFactorization riccati_ref = riccati;
  factorizer.backwardRiccatiRecursion(riccati_next, dtau, kkt_matrix, kkt_residual, riccati);
  backward_recursion_ref.factorizeKKTMatrix(riccati_next, dtau, kkt_matrix_ref, kkt_residual_ref);
  const Eigen::MatrixXd Ginv = kkt_matrix_ref.Quu().inverse();
  lqr_policy_ref.K = - Ginv  * kkt_matrix_ref.Qxu().transpose();
  lqr_policy_ref.k = - Ginv  * kkt_residual.lu();
  backward_recursion_ref.factorizeRiccatiFactorization(riccati_next, kkt_matrix_ref, kkt_residual_ref, lqr_policy_ref, dtau, riccati_ref);
  factorizer.forwardRiccatiRecursionParallel(kkt_matrix, kkt_residual, true);
  kkt_matrix_ref.Fxx() += kkt_matrix_ref.Fxu() * lqr_policy_ref.K;
  kkt_residual_ref.Fx() += kkt_matrix_ref.Fxu() * lqr_policy_ref.k;
  EXPECT_TRUE(kkt_matrix.isApprox(kkt_matrix_ref));
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  const Eigen::MatrixXd BGinvBt = kkt_matrix_ref.Fxu() * Ginv * kkt_matrix_ref.Fxu().transpose();

  SplitDirection d(robot);
  d.setRandom();
  SplitDirection d_next(robot);
  d_next.setRandom();
  SplitDirection d_next_ref = d_next;
  factorizer.forwardRiccatiRecursion(kkt_matrix, kkt_residual, riccati_next, d, dtau, d_next, false);
  if (!robot.hasFloatingBase()) {
    kkt_matrix_ref.Fqq().setIdentity();
  }
  d_next_ref.dx() = kkt_matrix_ref.Fxx() * d.dx() + kkt_residual_ref.Fx();
  EXPECT_TRUE(d_next.isApprox(d_next_ref));
  factorizer.forwardRiccatiRecursion(kkt_matrix, kkt_residual, riccati_next, d, dtau, d_next, true);
  d_next_ref.dx() = kkt_matrix_ref.Fxx() * d.dx() + kkt_residual_ref.Fx() 
                      - kkt_matrix_ref.Fxu() * Ginv * kkt_matrix_ref.Fxu().transpose() * riccati_next.n;
  EXPECT_TRUE(d_next.isApprox(d_next_ref));
  SplitDirection d_ref = d;
  factorizer.computeCostateDirection(riccati, d, false);
  d_ref.dlmd() = riccati.Pqq * d.dq() + riccati.Pqv * d.dv() - riccati.sq;
  d_ref.dgmm() = riccati.Pvq * d.dq() + riccati.Pvv * d.dv() - riccati.sv;
  EXPECT_TRUE(d.isApprox(d_ref));
  factorizer.computeCostateDirection(riccati, d, true);
  d_ref.dlmd() = riccati.Pqq * d.dq() + riccati.Pqv * d.dv() - riccati.sq + riccati.n.head(dimv);
  d_ref.dgmm() = riccati.Pvq * d.dq() + riccati.Pvv * d.dv() - riccati.sv + riccati.n.tail(dimv);
  EXPECT_TRUE(d.isApprox(d_ref));
  factorizer.computeControlInputDirection(riccati, d, false);
  d_ref.du() = lqr_policy_ref.K * d.dx() + lqr_policy_ref.k;
  EXPECT_TRUE(d.isApprox(d_ref));
  factorizer.computeControlInputDirection(riccati, d, true);
  d_ref.du() = lqr_policy_ref.K * d.dx() + lqr_policy_ref.k - Ginv * kkt_matrix_ref.Fxu().transpose() * riccati.n;
  EXPECT_TRUE(d.isApprox(d_ref));
}


void SplitRiccatiFactorizerTest::testNearZeroDtau(const Robot& robot) const {
  const double dtau_near_zero 
      = std::abs(Eigen::VectorXd::Random(1).coeff(0)) * std::sqrt(std::numeric_limits<double>::epsilon());
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  SplitRiccatiFactorization riccati_next = createRiccatiFactorization(robot);
  SplitKKTMatrix kkt_matrix = createKKTMatrix(robot);
  SplitKKTResidual kkt_residual = createKKTResidual(robot);
  kkt_matrix.Fqv() = dtau_near_zero * Eigen::MatrixXd::Identity(dimv, dimv);
  SplitKKTMatrix kkt_matrix_ref = kkt_matrix;
  SplitKKTResidual kkt_residual_ref = kkt_residual;
  SplitRiccatiFactorizer factorizer(robot);
  LQRStateFeedbackPolicy lqr_policy_ref(robot);
  BackwardRiccatiRecursionFactorizer backward_recursion_ref(robot);
  SplitRiccatiFactorization riccati = createRiccatiFactorization(robot);
  SplitRiccatiFactorization riccati_ref = riccati;
  factorizer.backwardRiccatiRecursion(riccati_next, dtau_near_zero, kkt_matrix, kkt_residual, riccati);
  backward_recursion_ref.factorizeKKTMatrix(riccati_next, dtau_near_zero, kkt_matrix_ref, kkt_residual_ref);
  lqr_policy_ref.K.setZero();
  lqr_policy_ref.k.setZero();
  backward_recursion_ref.factorizeRiccatiFactorization(riccati_next, kkt_matrix_ref, kkt_residual_ref, lqr_policy_ref, dtau_near_zero, riccati_ref);
  EXPECT_TRUE(riccati.Pqq.isApprox(riccati_ref.Pqq));
  EXPECT_TRUE(riccati.Pqv.isApprox(riccati_ref.Pqv));
  EXPECT_TRUE(riccati.Pvq.isApprox(riccati_ref.Pvq));
  EXPECT_TRUE(riccati.Pvv.isApprox(riccati_ref.Pvv));
  EXPECT_TRUE(riccati.sq.isApprox(riccati_ref.sq));
  EXPECT_TRUE(riccati.sv.isApprox(riccati_ref.sv));
  EXPECT_TRUE(riccati.Pqq.isApprox(riccati.Pqq.transpose()));
  EXPECT_TRUE(riccati.Pvv.isApprox(riccati.Pvv.transpose()));
  EXPECT_TRUE(riccati.Pvq.isApprox(riccati.Pqv.transpose()));
  EXPECT_TRUE(kkt_matrix.Qxx().isApprox(kkt_matrix.Qxx().transpose()));
  EXPECT_TRUE(kkt_matrix.Quu().isApprox(kkt_matrix.Quu().transpose()));
  Eigen::MatrixXd K(dimu, 2*dimv);
  factorizer.getStateFeedbackGain(K);
  EXPECT_TRUE(K.isZero());
  Eigen::MatrixXd Kq(dimu, dimv);
  Eigen::MatrixXd Kv(dimu, dimv);
  factorizer.getStateFeedbackGain(Kq, Kv);
  EXPECT_TRUE(Kq.isZero());
  EXPECT_TRUE(Kv.isZero());

  factorizer.forwardRiccatiRecursionParallel(kkt_matrix, kkt_residual, true);
  EXPECT_TRUE(kkt_matrix_ref.isApprox(kkt_matrix));
  EXPECT_TRUE(kkt_residual_ref.isApprox(kkt_residual));
  auto riccati_next_ref = riccati_next;
  factorizer.forwardStateConstraintFactorization(riccati, kkt_matrix, kkt_residual, dtau_near_zero, riccati_next, false);
  riccati_next_ref.Pi = kkt_matrix_ref.Fxx() * riccati.Pi;
  riccati_next_ref.pi = kkt_residual_ref.Fx() + kkt_matrix_ref.Fxx() * riccati.pi;
  EXPECT_TRUE(riccati_next.Pi.isApprox(riccati_next_ref.Pi));
  EXPECT_TRUE(riccati_next.pi.isApprox(riccati_next_ref.pi));
  EXPECT_TRUE(riccati_next.N.isApprox(riccati_next_ref.N));
  factorizer.forwardStateConstraintFactorization(riccati, kkt_matrix, kkt_residual, dtau_near_zero, riccati_next, true);
  riccati_next_ref.Pi = kkt_matrix_ref.Fxx() * riccati.Pi;
  riccati_next_ref.pi = kkt_residual_ref.Fx() + kkt_matrix_ref.Fxx() * riccati.pi;
  riccati_next_ref.N = kkt_matrix_ref.Fxx() * riccati.N * kkt_matrix_ref.Fxx().transpose();
  EXPECT_TRUE(riccati_next.Pi.isApprox(riccati_next_ref.Pi));
  EXPECT_TRUE(riccati_next.pi.isApprox(riccati_next_ref.pi));
  EXPECT_TRUE(riccati_next.N.isApprox(riccati_next_ref.N));

  const int dimf = 6;
  Eigen::MatrixXd T_next(2*dimv, dimf), T(2*dimv, dimf), T_ref(2*dimv, dimf);
  T_next.setRandom();
  T.setRandom();
  T_ref = T;
  factorizer.backwardStateConstraintFactorization(T_next, kkt_matrix, dtau_near_zero, T);
  T_ref = kkt_matrix.Fxx().transpose() * T_next;
  EXPECT_TRUE(T.isApprox(T_ref));

  SplitDirection d(robot);
  d.setRandom();
  SplitDirection d_next(robot);
  d_next.setRandom();
  SplitDirection d_next_ref = d_next;
  factorizer.forwardRiccatiRecursion(kkt_matrix, kkt_residual, riccati_next, d, dtau_near_zero, d_next, false);
  if (!robot.hasFloatingBase()) {
    kkt_matrix_ref.Fqq().setIdentity();
  }
  d_next_ref.dx() = kkt_matrix_ref.Fxx() * d.dx() + kkt_residual_ref.Fx();
  EXPECT_TRUE(d_next.isApprox(d_next_ref));
  factorizer.forwardRiccatiRecursion(kkt_matrix, kkt_residual, riccati_next, d, dtau_near_zero, d_next, true);
  d_next_ref.dx() = kkt_matrix_ref.Fxx() * d.dx() + kkt_residual_ref.Fx();
  EXPECT_TRUE(d_next.isApprox(d_next_ref));
  SplitDirection d_ref = d;
  factorizer.computeCostateDirection(riccati, d, false);
  d_ref.dlmd() = riccati.Pqq * d.dq() + riccati.Pqv * d.dv() - riccati.sq;
  d_ref.dgmm() = riccati.Pvq * d.dq() + riccati.Pvv * d.dv() - riccati.sv;
  EXPECT_TRUE(d.isApprox(d_ref));
  factorizer.computeCostateDirection(riccati, d, true);
  d_ref.dlmd() = riccati.Pqq * d.dq() + riccati.Pqv * d.dv() - riccati.sq + riccati.n.head(dimv);
  d_ref.dgmm() = riccati.Pvq * d.dq() + riccati.Pvv * d.dv() - riccati.sv + riccati.n.tail(dimv);
  EXPECT_TRUE(d.isApprox(d_ref));
  factorizer.computeControlInputDirection(riccati, d, false);
  d_ref.du().setZero();
  EXPECT_TRUE(d.isApprox(d_ref));
  factorizer.computeControlInputDirection(riccati, d, true);
  d_ref.du().setZero();
  EXPECT_TRUE(d.isApprox(d_ref));
}


TEST_F(SplitRiccatiFactorizerTest, fixedBase) {
  Robot robot(fixed_base_urdf, {18});
  testBackwardRecursion(robot);
  testForwardStateConstraintFactorization(robot);
  testBackwardStateConstraintFactorization(robot);
  testForwardRecursion(robot);
  testNearZeroDtau(robot);
}


TEST_F(SplitRiccatiFactorizerTest, floating_base) {
  Robot robot(floating_base_urdf, {14, 24, 34, 44});
  testBackwardRecursion(robot);
  testForwardStateConstraintFactorization(robot);
  testBackwardStateConstraintFactorization(robot);
  testForwardRecursion(robot);
  testNearZeroDtau(robot);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}