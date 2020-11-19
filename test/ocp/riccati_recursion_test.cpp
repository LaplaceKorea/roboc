#include <string>
#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/impulse/impulse_kkt_matrix.hpp"
#include "idocp/impulse/impulse_kkt_residual.hpp"
#include "idocp/ocp/riccati_factorization.hpp"
#include "idocp/ocp/riccati_factorizer.hpp"
#include "idocp/ocp/riccati_recursion.hpp"
#include "idocp/hybrid/hybrid_container.hpp"

namespace idocp {

class RiccatiRecursionTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
    fixed_base_robot = Robot(fixed_base_urdf, {18});
    floating_base_robot = Robot(floating_base_urdf, {14, 24, 34, 44});
    N = 20;
    max_num_impulse = 5;
    nproc = 4;
    T = 1;
    t = std::abs(Eigen::VectorXd::Random(1)[0]);
    dtau = T / N;
  }

  virtual void TearDown() {
  }

  using HybridKKTMatrix = hybrid_container<KKTMatrix, ImpulseKKTMatrix>;
  using HybridKKTResidual = hybrid_container<KKTResidual, ImpulseKKTResidual>;
  using HybridRiccatiFactorization = hybrid_container<RiccatiFactorization, RiccatiFactorization>;
  using HybridRiccatiFactorizer = hybrid_container<RiccatiFactorizer, ImpulseRiccatiFactorizer>;

  HybridKKTMatrix createHybridKKTMatrix(const Robot& robot) const;
  HybridKKTResidual createHybridKKTResidual(const Robot& robot) const;

  DiscreteEvent createDiscreteEvent(const Robot& robot, const ContactStatus& pre_contact_status) const;
  ContactSequence createContactSequence(const Robot& robot) const;

  static void testIsRiccatiZero(const RiccatiFactorization& riccati);
  static void testIsRiccatiSame(const RiccatiFactorization& lhs, const RiccatiFactorization& rhs);
  void testIsConstraintFactorizationSame(const StateConstraintRiccatiFactorization& lhs, 
                                         const StateConstraintRiccatiFactorization& rhs) const;

  void testRiccatiRecursion(const Robot& robot) const;
  void testStateConstraintFactorization(const Robot& robot) const;

  int N, max_num_impulse, nproc;
  double T, t, dtau;
  std::string fixed_base_urdf, floating_base_urdf;
  Robot fixed_base_robot, floating_base_robot;
};


RiccatiRecursionTest::HybridKKTMatrix RiccatiRecursionTest::createHybridKKTMatrix(const Robot& robot) const {
  HybridKKTMatrix kkt_matrix = HybridKKTMatrix(N+1, KKTMatrix(robot), max_num_impulse, ImpulseKKTMatrix(robot));
  const int dimx = 2*robot.dimv();
  const int dimu = robot.dimu();
  for (int i=0; i<=N; ++i) {
    Eigen::MatrixXd tmp = Eigen::MatrixXd::Random(dimx, dimx);
    kkt_matrix[i].Qxx() = tmp * tmp.transpose() + Eigen::MatrixXd::Identity(dimx, dimx);
    tmp = Eigen::MatrixXd::Random(dimu, dimu);
    kkt_matrix[i].Quu() = tmp * tmp.transpose() + Eigen::MatrixXd::Identity(dimu, dimu);
    kkt_matrix[i].Qxu().setRandom();
    if (robot.has_floating_base()) {
      kkt_matrix[i].Fqq().topLeftCorner(6, 6).setRandom();
    }
    kkt_matrix[i].Fvq().setRandom();
    kkt_matrix[i].Fvv().setRandom();
    kkt_matrix[i].Fvu().setRandom();
  }
  for (int i=0; i<max_num_impulse; ++i) {
    Eigen::MatrixXd tmp = Eigen::MatrixXd::Random(dimx, dimx);
    kkt_matrix.impulse[i].Qxx() = tmp * tmp.transpose() + Eigen::MatrixXd::Identity(dimx, dimx);
    if (robot.has_floating_base()) {
      kkt_matrix.impulse[i].Fqq().topLeftCorner(6, 6).setRandom();
    }
    kkt_matrix.impulse[i].Fvq().setRandom();
    kkt_matrix.impulse[i].Fvv().setRandom();
  }
  for (int i=0; i<max_num_impulse; ++i) {
    Eigen::MatrixXd tmp = Eigen::MatrixXd::Random(dimx, dimx);
    kkt_matrix.aux[i].Qxx() = tmp * tmp.transpose() + Eigen::MatrixXd::Identity(dimx, dimx);
    tmp = Eigen::MatrixXd::Random(dimu, dimu);
    kkt_matrix.aux[i].Quu() = tmp * tmp.transpose() + Eigen::MatrixXd::Identity(dimu, dimu);
    kkt_matrix.aux[i].Qxu().setRandom();
    if (robot.has_floating_base()) {
      kkt_matrix.aux[i].Fqq().topLeftCorner(6, 6).setRandom();
    }
    kkt_matrix.aux[i].Fvq().setRandom();
    kkt_matrix.aux[i].Fvv().setRandom();
    kkt_matrix.aux[i].Fvu().setRandom();
  }
  for (int i=0; i<max_num_impulse; ++i) {
    Eigen::MatrixXd tmp = Eigen::MatrixXd::Random(dimx, dimx);
    kkt_matrix.lift[i].Qxx() = tmp * tmp.transpose() + Eigen::MatrixXd::Identity(dimx, dimx);
    tmp = Eigen::MatrixXd::Random(dimu, dimu);
    kkt_matrix.lift[i].Quu() = tmp * tmp.transpose() + Eigen::MatrixXd::Identity(dimu, dimu);
    kkt_matrix.lift[i].Qxu().setRandom();
    if (robot.has_floating_base()) {
      kkt_matrix.lift[i].Fqq().topLeftCorner(6, 6).setRandom();
    }
    kkt_matrix.lift[i].Fvq().setRandom();
    kkt_matrix.lift[i].Fvv().setRandom();
    kkt_matrix.lift[i].Fvu().setRandom();
  }
  return kkt_matrix;
}


RiccatiRecursionTest::HybridKKTResidual RiccatiRecursionTest::createHybridKKTResidual(const Robot& robot) const {
  HybridKKTResidual kkt_residual = HybridKKTResidual(N+1, KKTResidual(robot), max_num_impulse, ImpulseKKTResidual(robot));
  for (int i=0; i<=N; ++i) {
    kkt_residual[i].lx().setRandom();
    kkt_residual[i].lu().setRandom();
    kkt_residual[i].Fx().setRandom();
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_residual.impulse[i].lx().setRandom();
    kkt_residual.impulse[i].Fx().setRandom();
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_residual.aux[i].lx().setRandom();
    kkt_residual.aux[i].lu().setRandom();
    kkt_residual.aux[i].Fx().setRandom();
  }
  for (int i=0; i<max_num_impulse; ++i) {
    kkt_residual.lift[i].lx().setRandom();
    kkt_residual.lift[i].lu().setRandom();
    kkt_residual.lift[i].Fx().setRandom();
  }
  return kkt_residual;
}


DiscreteEvent RiccatiRecursionTest::createDiscreteEvent(const Robot& robot, const ContactStatus& pre_contact_status) const {
  DiscreteEvent discrete_event(robot);
  ContactStatus post_contact_status = pre_contact_status;
  std::random_device rnd;
  while (!discrete_event.existDiscreteEvent()) {
    post_contact_status.setRandom();
    discrete_event.setDiscreteEvent(pre_contact_status, post_contact_status);
  }
  return discrete_event;
}


ContactSequence RiccatiRecursionTest::createContactSequence(const Robot& robot) const {
  std::vector<DiscreteEvent> discrete_events;
  ContactStatus pre_contact_status = robot.createContactStatus();
  pre_contact_status.setRandom();
  ContactSequence contact_sequence(robot, T, N);
  contact_sequence.setContactStatusUniformly(pre_contact_status);
  ContactStatus post_contact_status = pre_contact_status;
  std::random_device rnd;
  for (int i=0; i<max_num_impulse; ++i) {
    DiscreteEvent tmp(robot);
    tmp.setDiscreteEvent(pre_contact_status, post_contact_status);
    while (!tmp.existDiscreteEvent()) {
      post_contact_status.setRandom();
      tmp.setDiscreteEvent(pre_contact_status, post_contact_status);
    }
    tmp.eventTime = i * 0.15 + 0.01 * std::abs(Eigen::VectorXd::Random(1)[0]);
    discrete_events.push_back(tmp);
    pre_contact_status = post_contact_status;
  }
  for (int i=0; i<max_num_impulse; ++i) {
    contact_sequence.setDiscreteEvent(discrete_events[i]);
  }
  return contact_sequence;
}


void RiccatiRecursionTest::testIsRiccatiZero(const RiccatiFactorization& riccati) {
  EXPECT_TRUE(riccati.Pqq.isZero());
  EXPECT_TRUE(riccati.Pqv.isZero());
  EXPECT_TRUE(riccati.Pvq.isZero());
  EXPECT_TRUE(riccati.Pvv.isZero());
  EXPECT_TRUE(riccati.sq.isZero());
  EXPECT_TRUE(riccati.sv.isZero());
  EXPECT_TRUE(riccati.Pi.isIdentity()); // Default value of Pi is identity.
  EXPECT_TRUE(riccati.pi.isZero());
  EXPECT_TRUE(riccati.N.isZero());
  EXPECT_TRUE(riccati.n.isZero());
}


void RiccatiRecursionTest::testIsRiccatiSame(const RiccatiFactorization& lhs, const RiccatiFactorization& rhs) {
  EXPECT_TRUE(lhs.Pqq.isApprox(rhs.Pqq));
  EXPECT_TRUE(lhs.Pqv.isApprox(rhs.Pqv));
  EXPECT_TRUE(lhs.Pvq.isApprox(rhs.Pvq));
  EXPECT_TRUE(lhs.Pvv.isApprox(rhs.Pvv));
  EXPECT_TRUE(lhs.sq.isApprox(rhs.sq));
  EXPECT_TRUE(lhs.sv.isApprox(rhs.sv));
  EXPECT_TRUE(lhs.Pi.isApprox(rhs.Pi));
  EXPECT_TRUE(lhs.pi.isApprox(rhs.pi));
  EXPECT_TRUE(lhs.N.isApprox(rhs.N));
  EXPECT_TRUE(lhs.n.isApprox(rhs.n));
}


void RiccatiRecursionTest::testIsConstraintFactorizationSame(
    const StateConstraintRiccatiFactorization& lhs, 
    const StateConstraintRiccatiFactorization& rhs) const {
  for (int i=0; i<N; ++i) {
    EXPECT_TRUE(lhs.T(i).isApprox(rhs.T(i)));
  }
  for (int i=0; i<max_num_impulse; ++i) {
    EXPECT_TRUE(lhs.T_impulse(i).isApprox(rhs.T_impulse(i)));
  }
  for (int i=0; i<max_num_impulse; ++i) {
    EXPECT_TRUE(lhs.T_aux(i).isApprox(rhs.T_aux(i)));
  }
  for (int i=0; i<max_num_impulse; ++i) {
    EXPECT_TRUE(lhs.T_lift(i).isApprox(rhs.T_lift(i)));
  }
}


void RiccatiRecursionTest::testRiccatiRecursion(const Robot& robot) const {
  const auto contact_sequence = createContactSequence(robot);
  auto kkt_matrix = createHybridKKTMatrix(robot);
  auto kkt_residual= createHybridKKTResidual(robot);
  HybridRiccatiFactorization factorization(N+1, RiccatiFactorization(robot), max_num_impulse, RiccatiFactorization(robot));
  RiccatiRecursion riccati_recursion(robot, T, N, max_num_impulse, nproc);
  riccati_recursion.backwardRiccatiRecursionTerminal(kkt_matrix, kkt_residual, factorization);
  for (int i=0; i<N; ++i) {
    testIsRiccatiZero(factorization[i]);
  }
  EXPECT_TRUE(factorization[N].Pqq.isApprox(kkt_matrix[N].Qqq()));
  EXPECT_TRUE(factorization[N].Pqv.isZero());
  EXPECT_TRUE(factorization[N].Pvq.isZero());
  EXPECT_TRUE(factorization[N].Pvv.isApprox(kkt_matrix[N].Qvv()));
  EXPECT_TRUE(factorization[N].sq.isApprox(-1*kkt_residual[N].lq()));
  EXPECT_TRUE(factorization[N].sv.isApprox(-1*kkt_residual[N].lv()));
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiZero(factorization.impulse[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiZero(factorization.aux[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiZero(factorization.lift[i]);
  }
  auto factorization_ref = factorization;
  auto kkt_matrix_ref = kkt_matrix; 
  auto kkt_residual_ref = kkt_residual; 
  riccati_recursion.backwardRiccatiRecursion(contact_sequence, kkt_matrix, kkt_residual, factorization);
  HybridRiccatiFactorizer factorizer(N+1, RiccatiFactorizer(robot), max_num_impulse, ImpulseRiccatiFactorizer(robot));
  for (int i=N-1; i>=0; --i) {
    if (contact_sequence.existImpulseStage(i)) {
      const int impulse_index = contact_sequence.impulseIndex(i);
      const double dtau_impulse = contact_sequence.impulseTime(impulse_index) - i * dtau;
      const double dtau_aux = dtau - dtau_impulse;
      ASSERT_TRUE(dtau_impulse > 0);
      ASSERT_TRUE(dtau_aux > 0);
      factorizer.aux[impulse_index].backwardRiccatiRecursion(
          factorization_ref[i+1], 
          dtau_aux, 
          kkt_matrix_ref.aux[impulse_index], 
          kkt_residual_ref.aux[impulse_index], 
          factorization_ref.aux[impulse_index]);
      factorizer.impulse[impulse_index].backwardRiccatiRecursion(
          factorization_ref.aux[impulse_index], 
          kkt_matrix_ref.impulse[impulse_index], 
          kkt_residual_ref.impulse[impulse_index], 
          factorization_ref.impulse[impulse_index]);
      factorizer[i].backwardRiccatiRecursion(
          factorization_ref.impulse[impulse_index], 
          dtau_impulse, 
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          factorization_ref[i]);
    }
    else if (contact_sequence.existLiftStage(i)) {
      const int lift_index = contact_sequence.liftIndex(i);
      const double dtau_lift = contact_sequence.liftTime(lift_index) - i * dtau;
      const double dtau_aux = dtau - dtau_lift;
      ASSERT_TRUE(dtau_lift > 0);
      ASSERT_TRUE(dtau_aux > 0);
      factorizer.lift[lift_index].backwardRiccatiRecursion(
          factorization_ref[i+1], 
          dtau_aux, 
          kkt_matrix_ref.lift[lift_index], 
          kkt_residual_ref.lift[lift_index], 
          factorization_ref.lift[lift_index]);
      factorizer[i].backwardRiccatiRecursion(
          factorization_ref.lift[lift_index], 
          dtau_lift, 
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          factorization_ref[i]);
    }
    else {
      factorizer[i].backwardRiccatiRecursion(
          factorization_ref[i+1], 
          dtau, 
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          factorization_ref[i]);
    }
  }
  for (int i=0; i<=N; ++i) {
    testIsRiccatiSame(factorization[i], factorization_ref[i]);
    kkt_matrix[i].isApprox(kkt_matrix_ref[i]);
    kkt_residual[i].isApprox(kkt_residual_ref[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiSame(factorization.aux[i], factorization_ref.aux[i]);
    kkt_matrix.aux[i].isApprox(kkt_matrix_ref.aux[i]);
    kkt_residual.aux[i].isApprox(kkt_residual_ref.aux[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiSame(factorization.impulse[i], factorization_ref.impulse[i]);
    kkt_matrix.impulse[i].isApprox(kkt_matrix_ref.impulse[i]);
    kkt_residual.impulse[i].isApprox(kkt_residual_ref.impulse[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiSame(factorization.lift[i], factorization_ref.lift[i]);
    kkt_matrix.lift[i].isApprox(kkt_matrix_ref.lift[i]);
    kkt_residual.lift[i].isApprox(kkt_residual_ref.lift[i]);
  }
  riccati_recursion.forwardRiccatiRecursionParallel(contact_sequence, kkt_matrix, kkt_residual);
  riccati_recursion.forwardRiccatiRecursionSerial(contact_sequence, kkt_matrix, kkt_residual, factorization);
  const bool exist_state_constraint = contact_sequence.existImpulseStage();
  for (int i=0; i<N; ++i) {
    if (contact_sequence.existImpulseStage(i)) {
      const int impulse_index = contact_sequence.impulseIndex(i);
      const double dtau_impulse = contact_sequence.impulseTime(impulse_index) - i * dtau;
      const double dtau_aux = dtau - dtau_impulse;
      ASSERT_TRUE(dtau_impulse > 0);
      ASSERT_TRUE(dtau_aux > 0);
      factorizer[i].forwardRiccatiRecursionParallel(
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          exist_state_constraint);
      factorizer[i].forwardRiccatiRecursionSerial(
          factorization_ref[i],
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          dtau_impulse,
          factorization_ref.impulse[impulse_index], 
          exist_state_constraint);

      factorizer.impulse[impulse_index].forwardRiccatiRecursionSerial(
          factorization_ref.impulse[impulse_index], 
          kkt_matrix_ref.impulse[impulse_index], 
          kkt_residual_ref.impulse[impulse_index], 
          factorization_ref.aux[impulse_index]);

      factorizer.aux[impulse_index].forwardRiccatiRecursionParallel(
          kkt_matrix_ref.aux[impulse_index], 
          kkt_residual_ref.aux[impulse_index], 
          exist_state_constraint);
      factorizer.aux[impulse_index].forwardRiccatiRecursionSerial(
          factorization_ref.aux[impulse_index],
          kkt_matrix_ref.aux[impulse_index], 
          kkt_residual_ref.aux[impulse_index], 
          dtau_aux,
          factorization_ref[i+1], 
          exist_state_constraint);
    }
    else if (contact_sequence.existLiftStage(i)) {
      const int lift_index = contact_sequence.liftIndex(i);
      const double dtau_lift = contact_sequence.liftTime(lift_index) - i * dtau;
      const double dtau_aux = dtau - dtau_lift;
      ASSERT_TRUE(dtau_lift > 0);
      ASSERT_TRUE(dtau_aux > 0);

      factorizer[i].forwardRiccatiRecursionParallel(
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          exist_state_constraint);
      factorizer[i].forwardRiccatiRecursionSerial(
          factorization_ref[i],
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          dtau_lift,
          factorization_ref.lift[lift_index], 
          exist_state_constraint);

      factorizer.lift[lift_index].forwardRiccatiRecursionParallel(
          kkt_matrix_ref.lift[lift_index], 
          kkt_residual_ref.lift[lift_index], 
          exist_state_constraint);
      factorizer.lift[lift_index].forwardRiccatiRecursionSerial(
          factorization_ref.lift[lift_index],
          kkt_matrix_ref.lift[lift_index], 
          kkt_residual_ref.lift[lift_index], 
          dtau_aux,
          factorization_ref[i+1], 
          exist_state_constraint);
    }
    else {
      factorizer[i].forwardRiccatiRecursionParallel(
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          exist_state_constraint);
      factorizer[i].forwardRiccatiRecursionSerial(
          factorization_ref[i],
          kkt_matrix_ref[i], 
          kkt_residual_ref[i], 
          dtau,
          factorization_ref[i+1], 
          exist_state_constraint);
    }
  }
  for (int i=0; i<=N; ++i) {
    testIsRiccatiSame(factorization[i], factorization_ref[i]);
    kkt_matrix[i].isApprox(kkt_matrix_ref[i]);
    kkt_residual[i].isApprox(kkt_residual_ref[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiSame(factorization.aux[i], factorization_ref.aux[i]);
    kkt_matrix.aux[i].isApprox(kkt_matrix_ref.aux[i]);
    kkt_residual.aux[i].isApprox(kkt_residual_ref.aux[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiSame(factorization.impulse[i], factorization_ref.impulse[i]);
    kkt_matrix.impulse[i].isApprox(kkt_matrix_ref.impulse[i]);
    kkt_residual.impulse[i].isApprox(kkt_residual_ref.impulse[i]);
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsRiccatiSame(factorization.lift[i], factorization_ref.lift[i]);
    kkt_matrix.lift[i].isApprox(kkt_matrix_ref.lift[i]);
    kkt_residual.lift[i].isApprox(kkt_residual_ref.lift[i]);
  }
}


void RiccatiRecursionTest::testStateConstraintFactorization(const Robot& robot) const {
  const auto contact_sequence = createContactSequence(robot);
  auto kkt_matrix = createHybridKKTMatrix(robot);
  auto kkt_residual= createHybridKKTResidual(robot);
  HybridRiccatiFactorization factorization(N+1, RiccatiFactorization(robot), max_num_impulse, RiccatiFactorization(robot));
  RiccatiRecursion riccati_recursion(robot, T, N, max_num_impulse, nproc);
  riccati_recursion.backwardRiccatiRecursionTerminal(kkt_matrix, kkt_residual, factorization);
  riccati_recursion.backwardRiccatiRecursion(contact_sequence, kkt_matrix, kkt_residual, factorization);
  riccati_recursion.forwardRiccatiRecursionParallel(contact_sequence, kkt_matrix, kkt_residual);
  riccati_recursion.forwardRiccatiRecursionSerial(contact_sequence, kkt_matrix, kkt_residual, factorization);
  const auto kkt_matrix_ref = kkt_matrix; 
  std::vector<StateConstraintRiccatiFactorization> constraint_factorization(max_num_impulse, StateConstraintRiccatiFactorization(robot, N, max_num_impulse));
  for (int i=0; i<contact_sequence.totalNumImpulseStages(); ++i) {
    constraint_factorization[i].setImpulseStatus(contact_sequence.impulseStatus(i));
    constraint_factorization[i].T_impulse(i).setRandom();
  }
  auto constraint_factorization_ref = constraint_factorization;
  riccati_recursion.backwardStateConstraintFactorization(contact_sequence, kkt_matrix, constraint_factorization);
  HybridRiccatiFactorizer factorizer(N+1, RiccatiFactorizer(robot), max_num_impulse, ImpulseRiccatiFactorizer(robot));
  const int num_constraint = contact_sequence.totalNumImpulseStages();
  for (int constraint_index=0; constraint_index<num_constraint; ++constraint_index) {
    StateConstraintRiccatiFactorization& factorization_ref = constraint_factorization_ref[constraint_index];
    const int constraint_stage = contact_sequence.timeStageBeforeImpulse(constraint_index);
    const double dtau_constraint = contact_sequence.impulseTime(constraint_index) - constraint_stage * dtau;
    ASSERT_TRUE(dtau_constraint > 0);
    ASSERT_TRUE(dtau_constraint < dtau);
    factorizer[constraint_stage].backwardStateConstraintFactorization(
        factorization_ref.T_impulse(constraint_index), 
        kkt_matrix[constraint_stage], dtau_constraint, 
        factorization_ref.T(constraint_stage));
    for (int i=constraint_stage-1; i>=0; --i) {
      if (contact_sequence.existImpulseStage(i)) {
        const int impulse_index = contact_sequence.impulseIndex(i);
        const double dtau_impulse = contact_sequence.impulseTime(impulse_index) - i * dtau;
        const double dtau_aux = dtau - dtau_impulse;
        ASSERT_TRUE(dtau_impulse > 0);
        ASSERT_TRUE(dtau_aux > 0);
        factorizer.aux[impulse_index].backwardStateConstraintFactorization(
            factorization_ref.T(i+1),
            kkt_matrix_ref.aux[impulse_index], 
            dtau_aux, 
            factorization_ref.T_aux(impulse_index));
        factorizer.impulse[impulse_index].backwardStateConstraintFactorization(
            factorization_ref.T_aux(impulse_index),
            kkt_matrix_ref.impulse[impulse_index], 
            factorization_ref.T_impulse(impulse_index));
        factorizer[i].backwardStateConstraintFactorization(
            factorization_ref.T_impulse(impulse_index),
            kkt_matrix_ref[i], 
            dtau_impulse, 
            factorization_ref.T(i));
      }
      else if (contact_sequence.existLiftStage(i)) {
        const int lift_index = contact_sequence.liftIndex(i);
        const double dtau_lift = contact_sequence.liftTime(lift_index) - i * dtau;
        const double dtau_aux = dtau - dtau_lift;
        ASSERT_TRUE(dtau_lift > 0);
        ASSERT_TRUE(dtau_aux > 0);
        factorizer.lift[lift_index].backwardStateConstraintFactorization(
            factorization_ref.T(i+1), 
            kkt_matrix_ref.lift[lift_index], 
            dtau_aux, 
            factorization_ref.T_lift(lift_index));
        factorizer[i].backwardStateConstraintFactorization(
            factorization_ref.T_lift(lift_index), 
            kkt_matrix_ref[i], 
            dtau_lift, 
            factorization_ref.T(i));
      }
      else {
        factorizer[i].backwardStateConstraintFactorization(
            factorization_ref.T(i+1), 
            kkt_matrix_ref[i], 
            dtau, 
            factorization_ref.T(i));
      }
    }
  }
  for (int i=0; i<max_num_impulse; ++i) {
    testIsConstraintFactorizationSame(constraint_factorization[i], constraint_factorization_ref[i]);
  }
}



TEST_F(RiccatiRecursionTest, fixedBase) {
  testRiccatiRecursion(fixed_base_robot);
  testStateConstraintFactorization(fixed_base_robot);
}


TEST_F(RiccatiRecursionTest, floating_base) {
  testRiccatiRecursion(floating_base_robot);
  testStateConstraintFactorization(floating_base_robot);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}