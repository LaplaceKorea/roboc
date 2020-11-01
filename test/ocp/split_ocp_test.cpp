#include <string>
#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/ocp/split_ocp.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/ocp/riccati_solution.hpp"
#include "idocp/cost/cost_function.hpp"
#include "idocp/cost/cost_function_data.hpp"
#include "idocp/cost/joint_space_cost.hpp"
#include "idocp/cost/task_space_3d_cost.hpp"
#include "idocp/cost/contact_force_cost.hpp"
#include "idocp/constraints/constraints.hpp"
#include "idocp/constraints/joint_position_lower_limit.hpp"
#include "idocp/constraints/joint_position_upper_limit.hpp"
#include "idocp/constraints/joint_velocity_lower_limit.hpp"
#include "idocp/constraints/joint_velocity_upper_limit.hpp"


namespace idocp {

class SplitOCPTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
  }

  virtual void TearDown() {
  }

  static std::shared_ptr<CostFunction> createCost(const Robot& robot);

  static std::shared_ptr<Constraints> createConstraints(const Robot& robot);

  static SplitSolution generateFeasibleSolution(
      Robot& robot, const ContactStatus& contact_staus,
      const std::shared_ptr<Constraints>& constraints);

  static void testLinearizeOCPAndRiccatiRecursion(
      Robot& robot, const ContactStatus& contact_status, 
      const std::shared_ptr<CostFunction>& cost,
      const std::shared_ptr<Constraints>& constraints);

  static void testComputeKKTResidualEmptyCostAndEmptyConstraints(
      Robot& robot, const ContactStatus& contact_status);

  static void testComputeKKTResidualEmptyCost(
      Robot& robot, const ContactStatus& contact_status, 
      const std::shared_ptr<Constraints>& constraints);

  static void testComputeKKTResidualEmptyConstraints(
      Robot& robot, const ContactStatus& contact_status, 
      const std::shared_ptr<CostFunction>& cost);

  static void testComputeKKTResidual(
      Robot& robot, const ContactStatus& contact_status, 
      const std::shared_ptr<CostFunction>& cost,
      const std::shared_ptr<Constraints>& constraints);

  static void testCostAndConstraintViolation(
      Robot& robot, const ContactStatus& contact_status, 
      const std::shared_ptr<CostFunction>& cost,
      const std::shared_ptr<Constraints>& constraints);

  std::string fixed_base_urdf, floating_base_urdf;
};


std::shared_ptr<CostFunction> SplitOCPTest::createCost(const Robot& robot) {
  auto joint_cost = std::make_shared<JointSpaceCost>(robot);
  const Eigen::VectorXd q_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  Eigen::VectorXd q_ref = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_ref);
  const Eigen::VectorXd v_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd a_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  const Eigen::VectorXd a_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd u_weight = Eigen::VectorXd::Random(robot.dimu()).array().abs();
  const Eigen::VectorXd u_ref = Eigen::VectorXd::Random(robot.dimu());
  const Eigen::VectorXd qf_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  const Eigen::VectorXd vf_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  joint_cost->set_q_weight(q_weight);
  joint_cost->set_q_ref(q_ref);
  joint_cost->set_v_weight(v_weight);
  joint_cost->set_v_ref(v_ref);
  joint_cost->set_a_weight(a_weight);
  joint_cost->set_a_ref(a_ref);
  joint_cost->set_u_weight(u_weight);
  joint_cost->set_u_ref(u_ref);
  joint_cost->set_qf_weight(qf_weight);
  joint_cost->set_vf_weight(vf_weight);
  const int task_frame = 10;
  auto task_space_3d_cost = std::make_shared<TaskSpace3DCost >(robot, task_frame);
  const Eigen::Vector3d q_3d_weight = Eigen::Vector3d::Random().array().abs();
  const Eigen::Vector3d qf_3d_weight = Eigen::Vector3d::Random().array().abs();
  const Eigen::Vector3d q_3d_ref = Eigen::Vector3d::Random();
  task_space_3d_cost->set_q_3d_weight(q_3d_weight);
  task_space_3d_cost->set_qf_3d_weight(qf_3d_weight);
  task_space_3d_cost->set_q_3d_ref(q_3d_ref);
  auto contact_force_cost = std::make_shared<idocp::ContactForceCost>(robot);
  std::vector<Eigen::Vector3d> f_weight;
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    f_weight.push_back(Eigen::Vector3d::Constant(0.001));
  }
  contact_force_cost->set_f_weight(f_weight);
  auto cost = std::make_shared<CostFunction>();
  cost->push_back(joint_cost);
  cost->push_back(task_space_3d_cost);
  cost->push_back(contact_force_cost);
  return cost;
}


std::shared_ptr<Constraints> SplitOCPTest::createConstraints(const Robot& robot) {
  auto joint_lower_limit = std::make_shared<JointPositionLowerLimit>(robot);
  auto joint_upper_limit = std::make_shared<JointPositionUpperLimit>(robot);
  auto velocity_lower_limit = std::make_shared<JointVelocityLowerLimit>(robot);
  auto velocity_upper_limit = std::make_shared<JointVelocityUpperLimit>(robot);
  auto constraints = std::make_shared<Constraints>();
  constraints->push_back(joint_upper_limit); 
  constraints->push_back(joint_lower_limit);
  constraints->push_back(velocity_lower_limit); 
  constraints->push_back(velocity_upper_limit);
  return constraints;
}


SplitSolution SplitOCPTest::generateFeasibleSolution(
    Robot& robot, const ContactStatus& contact_staus,
    const std::shared_ptr<Constraints>& constraints) {
  auto empty_cost = std::make_shared<CostFunction>();
  SplitOCP ocp(robot, empty_cost, constraints);
  SplitSolution s = SplitSolution::Random(robot, contact_staus);
  while (!ocp.isFeasible(robot, s)) {
    s = SplitSolution::Random(robot, contact_staus);
  }
  return s;
}


void SplitOCPTest::testLinearizeOCPAndRiccatiRecursion(
    Robot& robot, const ContactStatus& contact_status, 
    const std::shared_ptr<CostFunction>& cost,
    const std::shared_ptr<Constraints>& constraints) {
  const SplitSolution s_prev = SplitSolution::Random(robot, contact_status);
  const SplitSolution s = generateFeasibleSolution(robot, contact_status, constraints);
  const SplitSolution s_next = SplitSolution::Random(robot, contact_status);
  SplitOCP ocp(robot, cost, constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
  ocp.initConstraints(robot, 10, dtau, s);
  RiccatiSolution riccati_next(robot), riccati(robot), riccati_ref(robot);
  SplitDirection d(robot), d_ref(robot), d_next(robot), d_next_ref(robot);
  d.setContactStatus(contact_status);
  d_ref.setContactStatus(contact_status);
  const int dimv = robot.dimv();
  const Eigen::MatrixXd seed_mat = Eigen::MatrixXd::Random(2*dimv, 2*dimv);
  const Eigen::MatrixXd P = seed_mat * seed_mat.transpose() + Eigen::MatrixXd::Identity(2*dimv, 2*dimv);
  riccati_next.Pqq = P.topLeftCorner(dimv, dimv);
  riccati_next.Pqv = P.topRightCorner(dimv, dimv);
  riccati_next.Pvq = riccati_next.Pqv.transpose();
  riccati_next.Pvv = P.bottomRightCorner(dimv, dimv);
  ocp.linearizeOCP(robot, contact_status, t, dtau, s_prev.q, s, s_next);
  ocp.backwardRiccatiRecursion(dtau, riccati_next, riccati);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(contact_status);
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  auto cost_data = cost->createCostFunctionData(robot);
  auto constraints_data = constraints->createConstraintsData(robot, 10);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  robot.updateKinematics(s.q, s.v, s.a);
  cost->computeStageCostDerivatives(robot, cost_data, t, dtau, s, kkt_residual);
  cost->computeStageCostHessian(robot, cost_data, t, dtau, s, kkt_matrix);
  constraints->augmentDualResidual(robot, constraints_data, dtau, s, kkt_residual);
  constraints->condenseSlackAndDual(robot, constraints_data, dtau, s, kkt_matrix, kkt_residual);
  stateequation::LinearizeForwardEuler(robot, dtau, s_prev.q, s, s_next, kkt_matrix, kkt_residual);
  ContactDynamics cd(robot);
  robot.updateKinematics(s.q, s.v, s.a);
  cd.linearizeContactDynamics(robot, contact_status, dtau, s, kkt_matrix, kkt_residual);
  cd.condenseContactDynamics(robot, contact_status, dtau, kkt_matrix, kkt_residual);
  RiccatiFactorizer riccati_factorizer(robot);
  RiccatiGain riccati_gain(robot);
  riccati_factorizer.factorizeBackwardRicursion(riccati_next, dtau, kkt_matrix, kkt_residual, riccati_gain, riccati_ref);
  EXPECT_TRUE(riccati.Pqq.isApprox(riccati_ref.Pqq));
  EXPECT_TRUE(riccati.Pqv.isApprox(riccati_ref.Pqv));
  EXPECT_TRUE(riccati.Pvq.isApprox(riccati_ref.Pvq));
  EXPECT_TRUE(riccati.Pvv.isApprox(riccati_ref.Pvv));
  EXPECT_TRUE(riccati.sq.isApprox(riccati_ref.sq));
  EXPECT_TRUE(riccati.sv.isApprox(riccati_ref.sv));
  d.dx().setRandom();
  d_ref = d;
  ocp.forwardRiccatiRecursion(dtau, d, d_next);
  riccati_factorizer.computeControlInputDirection(riccati_gain, d_ref);
  riccati_factorizer.factorizeForwardRicursion(kkt_matrix, kkt_residual, d_ref, dtau, d_next_ref);
  EXPECT_TRUE(d_next.isApprox(d_next_ref));
  ocp.computeCondensedPrimalDirection(robot, dtau, riccati_ref, s, d);
  riccati_factorizer.computeCostateDirection(riccati, d_ref);
  cd.computeCondensedPrimalDirection(robot, d_ref);
  constraints->computeSlackAndDualDirection(robot, constraints_data, dtau, s, d_ref);
  EXPECT_TRUE(d.isApprox(d_ref));
  ocp.computeCondensedDualDirection(robot, dtau, d_next, d);
  cd.computeCondensedDualDirection(robot, dtau, kkt_matrix, kkt_residual, 
                                   d_next_ref.dgmm(), d_ref);
  EXPECT_TRUE(d.isApprox(d_ref));
}


void SplitOCPTest::testComputeKKTResidualEmptyCostAndEmptyConstraints(
    Robot& robot, const ContactStatus& contact_status) {
  auto empty_cost = std::make_shared<CostFunction>();
  auto empty_constraints = std::make_shared<Constraints>();
  const SplitSolution s_prev = SplitSolution::Random(robot, contact_status);
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  const SplitSolution s_next = SplitSolution::Random(robot, contact_status);
  SplitOCP ocp(robot, empty_cost, empty_constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
  ocp.initConstraints(robot, 10, dtau, s);
  ocp.computeKKTResidual(robot, contact_status, t, dtau, s_prev.q, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual(dtau);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(contact_status);
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  if (robot.has_floating_base()) {
    robot.subtractConfiguration(s.q, s_next.q, kkt_residual.Fq());
    robot.dSubtractdConfigurationPlus(s.q, s_next.q, kkt_matrix.Fqq());
    robot.dSubtractdConfigurationMinus(s_prev.q, s.q, kkt_matrix.Fqq_prev);
    kkt_residual.Fq() += dtau * s.v;
    kkt_residual.Fv() = s.v + dtau * s.a - s_next.v;
    kkt_residual.lq() = kkt_matrix.Fqq().transpose() * s_next.lmd + kkt_matrix.Fqq_prev.transpose() * s.lmd;
    kkt_residual.lv() = dtau * s_next.lmd + s_next.gmm - s.gmm;
    kkt_residual.la = dtau * s_next.gmm;
  }
  else {
    kkt_residual.Fq() = s.q + dtau * s.v - s_next.q;
    kkt_residual.Fv() = s.v + dtau * s.a - s_next.v;
    kkt_residual.lq() = s_next.lmd - s.lmd;
    kkt_residual.lv() = dtau * s_next.lmd + s_next.gmm - s.gmm;
    kkt_residual.la = dtau * s_next.gmm;
  }
  Eigen::VectorXd ID = Eigen::VectorXd::Zero(robot.dimv());
  Eigen::MatrixXd dID_dq = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd dID_dv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd dID_da = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.setContactForces(contact_status, s.f);
  robot.RNEA(s.q, s.v, s.a, ID);
  if (robot.has_floating_base()) {
    ID.head(robot.dim_passive()) -= s.u_passive;
    ID.tail(robot.dimu()) -= s.u;
  }
  else {
    ID -= s.u;
  }
  robot.RNEADerivatives(s.q, s.v, s.a, dID_dq, dID_dv, dID_da);
  kkt_residual.lq() += dtau * dID_dq.transpose() * s.beta;
  kkt_residual.lv() += dtau * dID_dv.transpose() * s.beta;
  kkt_residual.la += dtau * dID_da.transpose() * s.beta;
  if (robot.has_floating_base()) {
    kkt_residual.lu() -= dtau * s.beta.tail(robot.dimu());
    kkt_residual.lu_passive -= dtau * s.beta.head(robot.dim_passive());
    kkt_residual.lu_passive += dtau * s.nu_passive;
  }
  else {
    kkt_residual.lu() -= dtau * s.beta;
  }
  Eigen::VectorXd C = Eigen::VectorXd::Zero(contact_status.dimf());
  if (contact_status.hasActiveContacts()) {
    Eigen::MatrixXd dID_df = Eigen::MatrixXd::Zero(robot.dimv(), contact_status.dimf());
    robot.updateKinematics(s.q, s.v, s.a);
    robot.dRNEAPartialdFext(contact_status, dID_df);
    kkt_residual.lf() += dtau * dID_df.transpose() * s.beta;
    Eigen::MatrixXd dC_dq = Eigen::MatrixXd::Zero(contact_status.dimf(), robot.dimv());
    Eigen::MatrixXd dC_dv = Eigen::MatrixXd::Zero(contact_status.dimf(), robot.dimv());
    Eigen::MatrixXd dC_da = Eigen::MatrixXd::Zero(contact_status.dimf(), robot.dimv());
    robot.updateKinematics(s.q, s.v, s.a);
    robot.computeBaumgarteResidual(contact_status, dtau, C);
    robot.computeBaumgarteDerivatives(contact_status, dtau, dC_dq, dC_dv, dC_da);
    kkt_residual.lq() += dtau * dC_dq.transpose() * s.mu_stack();
    kkt_residual.lv() += dtau * dC_dv.transpose() * s.mu_stack();
    kkt_residual.la += dtau * dC_da.transpose() * s.mu_stack();
  }
  double kkt_error_ref = kkt_residual.Fx().squaredNorm()
                         + kkt_residual.lx().squaredNorm()
                         + kkt_residual.la.squaredNorm()
                         + kkt_residual.lf().squaredNorm()
                         + kkt_residual.lu().squaredNorm()
                         + dtau * dtau * ID.squaredNorm();
  if (robot.has_floating_base()) {
    kkt_error_ref += kkt_residual.lu_passive.squaredNorm();
    kkt_error_ref += dtau * dtau * s.u_passive.squaredNorm();
  }
  if (contact_status.hasActiveContacts()) {
    kkt_error_ref += dtau * dtau * C.squaredNorm();
  }
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
  double constraint_violation_ref = kkt_residual.Fx().lpNorm<1>() + dtau * ID.lpNorm<1>();
  if (robot.has_floating_base()) {
    constraint_violation_ref += dtau * s.u_passive.lpNorm<1>();
  }
  if (contact_status.hasActiveContacts()) {
    constraint_violation_ref += dtau * C.lpNorm<1>();
  }
}


void SplitOCPTest::testComputeKKTResidualEmptyCost(
    Robot& robot, const ContactStatus& contact_status, 
    const std::shared_ptr<Constraints>& constraints) {
  auto empty_cost = std::make_shared<CostFunction>();
  const SplitSolution s_prev = SplitSolution::Random(robot, contact_status);
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  const SplitSolution s_next = SplitSolution::Random(robot, contact_status);
  SplitOCP ocp(robot, empty_cost, constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
  ocp.initConstraints(robot, 10, dtau, s);
  ocp.computeKKTResidual(robot, contact_status, t, dtau, s_prev.q, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual(dtau);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(contact_status);
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  auto constraints_data = constraints->createConstraintsData(robot, 10);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  constraints->augmentDualResidual(robot, constraints_data, dtau, s, kkt_residual);
  stateequation::LinearizeForwardEuler(robot, dtau, s_prev.q, s, s_next, kkt_matrix, kkt_residual);
  ContactDynamics cd(robot);
  robot.updateKinematics(s.q, s.v, s.a);
  cd.linearizeContactDynamics(robot, contact_status, dtau, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = kkt_residual.Fx().squaredNorm()
                         + kkt_residual.lx().squaredNorm()
                         + kkt_residual.la.squaredNorm()
                         + kkt_residual.lf().squaredNorm()
                         + kkt_residual.lu().squaredNorm()
                         + cd.squaredNormContactDynamicsResidual(dtau)
                         + constraints->squaredNormPrimalAndDualResidual(constraints_data);
  if (robot.has_floating_base()) {
    kkt_error_ref += kkt_residual.lu_passive.squaredNorm();
  }
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


void SplitOCPTest::testComputeKKTResidualEmptyConstraints(
    Robot& robot, const ContactStatus& contact_status, 
    const std::shared_ptr<CostFunction>& cost) {
  auto empty_constraints = std::make_shared<Constraints>();
  const SplitSolution s_prev = SplitSolution::Random(robot, contact_status);
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  const SplitSolution s_next = SplitSolution::Random(robot, contact_status);
  SplitOCP ocp(robot, cost, empty_constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
  ocp.initConstraints(robot, 10, dtau, s);
  ocp.computeKKTResidual(robot, contact_status, t, dtau, s_prev.q, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual(dtau);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(contact_status);
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  auto cost_data = cost->createCostFunctionData(robot);
  robot.updateKinematics(s.q, s.v, s.a);
  cost->computeStageCostDerivatives(robot, cost_data, t, dtau, s, kkt_residual);
  stateequation::LinearizeForwardEuler(robot, dtau, s_prev.q, s, s_next, kkt_matrix, kkt_residual);
  ContactDynamics cd(robot);
  robot.updateKinematics(s.q, s.v, s.a);
  cd.linearizeContactDynamics(robot, contact_status, dtau, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = kkt_residual.Fx().squaredNorm()
                         + kkt_residual.lx().squaredNorm()
                         + kkt_residual.la.squaredNorm()
                         + kkt_residual.lf().squaredNorm()
                         + kkt_residual.lu().squaredNorm()
                         + cd.squaredNormContactDynamicsResidual(dtau);
  if (robot.has_floating_base()) {
    kkt_error_ref += kkt_residual.lu_passive.squaredNorm();
  }
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


void SplitOCPTest::testComputeKKTResidual(
    Robot& robot, const ContactStatus& contact_status, 
    const std::shared_ptr<CostFunction>& cost,
    const std::shared_ptr<Constraints>& constraints) {
  const SplitSolution s_prev = SplitSolution::Random(robot, contact_status);
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  const SplitSolution s_next = SplitSolution::Random(robot, contact_status);
  SplitOCP ocp(robot, cost, constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
  ocp.initConstraints(robot, 10, dtau, s);
  ocp.computeKKTResidual(robot, contact_status, t, dtau, s_prev.q, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual(dtau);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(contact_status);
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  auto cost_data = cost->createCostFunctionData(robot);
  auto constraints_data = constraints->createConstraintsData(robot, 10);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  robot.updateKinematics(s.q, s.v, s.a);
  cost->computeStageCostDerivatives(robot, cost_data, t, dtau, s, kkt_residual);
  constraints->augmentDualResidual(robot, constraints_data, dtau, s, kkt_residual);
  stateequation::LinearizeForwardEuler(robot, dtau, s_prev.q, s, s_next, kkt_matrix, kkt_residual);
  ContactDynamics cd(robot);
  robot.updateKinematics(s.q, s.v, s.a);
  cd.linearizeContactDynamics(robot, contact_status, dtau, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = kkt_residual.Fx().squaredNorm()
                         + kkt_residual.lx().squaredNorm()
                         + kkt_residual.la.squaredNorm()
                         + kkt_residual.lf().squaredNorm()
                         + kkt_residual.lu().squaredNorm()
                         + cd.squaredNormContactDynamicsResidual(dtau)
                         + constraints->squaredNormPrimalAndDualResidual(constraints_data);
  if (robot.has_floating_base()) {
    kkt_error_ref += kkt_residual.lu_passive.squaredNorm();
  }
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


void SplitOCPTest::testCostAndConstraintViolation(
    Robot& robot, const ContactStatus& contact_status, 
    const std::shared_ptr<CostFunction>& cost,
    const std::shared_ptr<Constraints>& constraints) {
  const SplitSolution s_prev = SplitSolution::Random(robot, contact_status);
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  const SplitSolution s_next = SplitSolution::Random(robot, contact_status);
  const SplitDirection d_prev = SplitDirection::Random(robot, contact_status);
  const SplitDirection d = SplitDirection::Random(robot, contact_status);
  const SplitDirection d_next = SplitDirection::Random(robot, contact_status);
  SplitOCP ocp(robot, cost, constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
  ocp.initConstraints(robot, 10, dtau, s);
  ocp.computeKKTResidual(robot, contact_status, t, dtau, s_prev.q, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual(dtau);
  const double step_size = 0.3;
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(contact_status);
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  auto cost_data = cost->createCostFunctionData(robot);
  auto constraints_data = constraints->createConstraintsData(robot, 10);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  SplitSolution s_tmp = s;
  s_tmp.integrate(robot, step_size, d);
  robot.updateKinematics(s_tmp.q, s_tmp.v, s_tmp.a);
  double cost_ref = 0;
  cost_ref += cost->l(robot, cost_data, t, dtau, s_tmp);
  cost_ref += constraints->costSlackBarrier(constraints_data, step_size);
  constraints->computePrimalAndDualResidual(robot, constraints_data, dtau, s_tmp);
  stateequation::ComputeForwardEulerResidual(robot, step_size, dtau, s_tmp,  
                                             s_next.q, s_next.v, d_next.dq(), 
                                             d_next.dv(), kkt_residual);
  ContactDynamics cd(robot);
  cd.computeContactDynamicsResidual(robot, contact_status, dtau, s_tmp);
}


TEST_F(SplitOCPTest, fixedBase) {
  std::vector<int> contact_frames = {18};
  ContactStatus contact_status(contact_frames.size());
  Robot robot(fixed_base_urdf, contact_frames);
  contact_status.setContactStatus({false});
  const auto cost = createCost(robot);
  const auto constraints = createConstraints(robot);
  testLinearizeOCPAndRiccatiRecursion(robot, contact_status, cost, constraints);
  testComputeKKTResidualEmptyCostAndEmptyConstraints(robot, contact_status);
  testComputeKKTResidualEmptyCost(robot, contact_status, constraints);
  testComputeKKTResidualEmptyConstraints(robot, contact_status, cost);
  testComputeKKTResidual(robot, contact_status, cost, constraints);
  testCostAndConstraintViolation(robot, contact_status, cost, constraints);
  contact_status.setContactStatus({true});
  testLinearizeOCPAndRiccatiRecursion(robot, contact_status, cost, constraints);
  testComputeKKTResidualEmptyCostAndEmptyConstraints(robot, contact_status);
  testComputeKKTResidualEmptyCost(robot, contact_status, constraints);
  testComputeKKTResidualEmptyConstraints(robot, contact_status, cost);
  testComputeKKTResidual(robot, contact_status, cost, constraints);
  testCostAndConstraintViolation(robot, contact_status, cost, constraints);
}


TEST_F(SplitOCPTest, floatingBase) {
  std::vector<int> contact_frames = {14, 24, 34, 44};
  ContactStatus contact_status(contact_frames.size());
  Robot robot(floating_base_urdf, contact_frames);
  contact_status.setContactStatus({false, false, false, false});
  const auto cost = createCost(robot);
  const auto constraints = createConstraints(robot);
  testLinearizeOCPAndRiccatiRecursion(robot, contact_status, cost, constraints);
  testComputeKKTResidualEmptyCostAndEmptyConstraints(robot, contact_status);
  testComputeKKTResidualEmptyCost(robot, contact_status, constraints);
  testComputeKKTResidualEmptyConstraints(robot, contact_status, cost);
  testComputeKKTResidual(robot, contact_status, cost, constraints);
  testCostAndConstraintViolation(robot, contact_status, cost, constraints);
  std::random_device rnd;
  std::vector<bool> is_contact_active;
  for (const auto frame : contact_frames) {
    is_contact_active.push_back(rnd()%2==0);
  }
  if (!contact_status.hasActiveContacts()) {
    contact_status.activateContact(0);
  }
  contact_status.setContactStatus(is_contact_active);
  testLinearizeOCPAndRiccatiRecursion(robot, contact_status, cost, constraints);
  testComputeKKTResidualEmptyCostAndEmptyConstraints(robot, contact_status);
  testComputeKKTResidualEmptyCost(robot, contact_status, constraints);
  testComputeKKTResidualEmptyConstraints(robot, contact_status, cost);
  testComputeKKTResidual(robot, contact_status, cost, constraints);
  testCostAndConstraintViolation(robot, contact_status, cost, constraints);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}