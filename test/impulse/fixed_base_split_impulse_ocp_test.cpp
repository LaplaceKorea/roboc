#include <string>
#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/impulse/split_impulse_ocp.hpp"
#include "idocp/impulse/impulse_split_solution.hpp"
#include "idocp/impulse/impulse_split_direction.hpp"
#include "idocp/impulse/impulse_kkt_residual.hpp"
#include "idocp/impulse/impulse_kkt_matrix.hpp"
#include "idocp/cost/impulse_cost_function.hpp"
#include "idocp/cost/cost_function_data.hpp"
#include "idocp/cost/joint_space_impulse_cost.hpp"
#include "idocp/cost/impulse_force_cost.hpp"
#include "idocp/constraints/impulse_constraints.hpp"

#include "idocp/ocp/riccati_factorization.hpp"
#include "idocp/impulse/impulse_riccati_matrix_factorizer.hpp"


namespace idocp {

class FixedBaseSplitImpulseOCPTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    urdf = "../urdf/iiwa14/iiwa14.urdf";
    std::vector<int> contact_frames = {18};
    robot = Robot(urdf, contact_frames);
    std::random_device rnd;
    std::vector<bool> is_contact_active = {true};
    contact_status = ContactStatus(robot.max_point_contacts());
    contact_status.setContactStatus(is_contact_active);
    q_prev = Eigen::VectorXd::Zero(robot.dimq());
    robot.generateFeasibleConfiguration(q_prev);
    s = ImpulseSplitSolution::Random(robot, contact_status);
    s_next = SplitSolution::Random(robot, contact_status);
    s_tmp = ImpulseSplitSolution::Random(robot, contact_status);
    d = ImpulseSplitDirection::Random(robot, contact_status);
    d_next = SplitDirection::Random(robot, contact_status);
    t = std::abs(Eigen::VectorXd::Random(1)[0]);
    auto joint_cost = std::make_shared<JointSpaceImpulseCost>(robot);
    auto impulse_cost = std::make_shared<ImpulseForceCost>(robot);
    const Eigen::VectorXd q_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    Eigen::VectorXd q_ref = Eigen::VectorXd::Random(robot.dimq());
    robot.normalizeConfiguration(q_ref);
    const Eigen::VectorXd v_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(robot.dimv());
    const Eigen::VectorXd dv_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    const Eigen::VectorXd dv_ref = Eigen::VectorXd::Random(robot.dimv());
    joint_cost->set_q_weight(q_weight);
    joint_cost->set_q_ref(q_ref);
    joint_cost->set_v_weight(v_weight);
    joint_cost->set_v_ref(v_ref);
    joint_cost->set_dv_weight(dv_weight);
    joint_cost->set_dv_ref(dv_ref);
    std::vector<Eigen::Vector3d> f_weight, f_ref;
    for (int i=0; i<robot.max_point_contacts(); ++i) {
      f_weight.push_back(Eigen::Vector3d::Constant(0.01));
      f_ref.push_back(Eigen::Vector3d::Zero());
    }
    impulse_cost->set_f_weight(f_weight);
    impulse_cost->set_f_ref(f_ref);
    cost = std::make_shared<ImpulseCostFunction>();
    cost->push_back(joint_cost);
    cost->push_back(impulse_cost);
    cost_data = cost->createCostFunctionData(robot);
    constraints = std::make_shared<ImpulseConstraints>();
    constraints_data = constraints->createConstraintsData(robot);
    kkt_matrix = ImpulseKKTMatrix(robot);
    kkt_residual = ImpulseKKTResidual(robot);
    kkt_matrix.setContactStatus(contact_status);
    kkt_residual.setContactStatus(contact_status);
    impulse_dynamics = ImpulseDynamicsForwardEuler(robot);
    factorizer = ImpulseRiccatiMatrixFactorizer(robot);
  }

  virtual void TearDown() {
  }

  double dtau, t;
  std::string urdf;
  Robot robot;
  ContactStatus contact_status;
  std::shared_ptr<ImpulseCostFunction> cost;
  CostFunctionData cost_data;
  std::shared_ptr<ImpulseConstraints> constraints;
  ConstraintsData constraints_data;
  Eigen::VectorXd q_prev;
  ImpulseSplitSolution s, s_tmp;
  SplitSolution s_next;
  ImpulseSplitDirection d;
  SplitDirection d_next;
  ImpulseKKTMatrix kkt_matrix;
  ImpulseKKTResidual kkt_residual;
  ImpulseDynamicsForwardEuler impulse_dynamics;
  ImpulseRiccatiMatrixFactorizer factorizer;
};


TEST_F(FixedBaseSplitImpulseOCPTest, isFeasible) {
  SplitImpulseOCP ocp(robot, cost, constraints);
  EXPECT_EQ(constraints->isFeasible(robot, constraints_data, s),
            ocp.isFeasible(robot, s));
}


TEST_F(FixedBaseSplitImpulseOCPTest, KKTErrorNormStateEquation) {
  auto empty_cost = std::make_shared<ImpulseCostFunction>();
  auto empty_constraints = std::make_shared<ImpulseConstraints>();
  SplitImpulseOCP ocp(robot, empty_cost, empty_constraints);
  ocp.initConstraints(robot, s);
  s.beta.setZero();
  s.mu_stack().setZero();
  constraints->setSlackAndDual(robot, constraints_data, s);
  ocp.computeKKTResidual(robot, contact_status, t, q_prev, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual();
  kkt_residual.Fq() = s.q - s_next.q;
  kkt_residual.Fv() = s.v + s.dv - s_next.v;
  kkt_residual.lq() = s_next.lmd - s.lmd;
  kkt_residual.lv() = s_next.gmm - s.gmm;
  kkt_residual.ldv = s_next.gmm;
  Eigen::VectorXd dv_res = Eigen::VectorXd::Zero(robot.dimv());
  Eigen::MatrixXd ddv_dq = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd ddv_ddv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd ddv_df = Eigen::MatrixXd::Zero(robot.dimv(), contact_status.dimf());
  robot.setContactForces(contact_status, s.f);
  robot.RNEAImpulse(s.q, s.dv, kkt_residual.dv_res);
  robot.updateKinematics(s.q, s.v);
  robot.computeContactVelocityResidual(contact_status, kkt_residual.C());
  double kkt_error_ref = kkt_residual.Fq().squaredNorm()
                         + kkt_residual.Fv().squaredNorm()
                         + kkt_residual.lq().squaredNorm()
                         + kkt_residual.lv().squaredNorm()
                         + kkt_residual.ldv.squaredNorm()
                         + kkt_residual.lf().squaredNorm()
                         + kkt_residual.dv_res.squaredNorm()
                         + kkt_residual.C().squaredNorm();
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
  auto pair = ocp.costAndConstraintViolation(robot, t, s);
  EXPECT_DOUBLE_EQ(pair.first, 0);
  const double violation_ref = kkt_residual.Fx().lpNorm<1>() 
                                + kkt_residual.dv_res.lpNorm<1>()
                                + kkt_residual.C().lpNorm<1>();
  EXPECT_DOUBLE_EQ(pair.second, violation_ref);
}


TEST_F(FixedBaseSplitImpulseOCPTest, KKTErrorNormStateEquationAndRobotDynamics) {
  auto empty_cost = std::make_shared<ImpulseCostFunction>();
  auto empty_constraints = std::make_shared<ImpulseConstraints>();
  SplitImpulseOCP ocp(robot, empty_cost, empty_constraints);
  ocp.initConstraints(robot, s);
  constraints->setSlackAndDual(robot, constraints_data, s);
  ocp.computeKKTResidual(robot, contact_status, t, q_prev, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual();
  kkt_residual.Fq() = s.q - s_next.q;
  kkt_residual.Fv() = s.v + s.dv - s_next.v;
  kkt_residual.lq() = s_next.lmd - s.lmd;
  kkt_residual.lv() = s_next.gmm - s.gmm;
  kkt_residual.ldv = s_next.gmm;
  Eigen::VectorXd dv_res = Eigen::VectorXd::Zero(robot.dimv());
  Eigen::MatrixXd ddv_dq = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd ddv_ddv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd ddv_df = Eigen::MatrixXd::Zero(robot.dimv(), contact_status.dimf());
  robot.setContactForces(contact_status, s.f);
  robot.RNEAImpulse(s.q, s.dv, kkt_residual.dv_res);
  robot.RNEAImpulseDerivatives(s.q, s.dv, ddv_dq, ddv_ddv);
  robot.updateKinematics(s.q, s.v);
  robot.dRNEAPartialdFext(contact_status, ddv_df);
  kkt_residual.lq() += ddv_dq.transpose() * s.beta;
  kkt_residual.ldv += ddv_ddv.transpose() * s.beta;
  kkt_residual.lf() += ddv_df.transpose() * s.beta;
  robot.computeContactVelocityResidual(contact_status, kkt_residual.C());
  robot.computeContactVelocityDerivatives(contact_status, kkt_matrix.Cq(), 
                                          kkt_matrix.Cv());
  kkt_residual.lq() += kkt_matrix.Cq().transpose() * s.mu_stack();
  kkt_residual.lv() += kkt_matrix.Cv().transpose() * s.mu_stack();
  kkt_residual.ldv += kkt_matrix.Cv().transpose() * s.mu_stack();
  double kkt_error_ref = kkt_residual.Fq().squaredNorm()
                         + kkt_residual.Fv().squaredNorm()
                         + kkt_residual.lq().squaredNorm()
                         + kkt_residual.lv().squaredNorm()
                         + kkt_residual.ldv.squaredNorm()
                         + kkt_residual.lf().squaredNorm()
                         + kkt_residual.dv_res.squaredNorm()
                         + kkt_residual.C().squaredNorm();
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
  auto pair = ocp.costAndConstraintViolation(robot, t, s);
  EXPECT_DOUBLE_EQ(pair.first, 0);
  const double violation_ref = kkt_residual.Fx().lpNorm<1>() 
                                + kkt_residual.dv_res.lpNorm<1>()
                                + kkt_residual.C().lpNorm<1>();
  EXPECT_DOUBLE_EQ(pair.second, violation_ref);
}


TEST_F(FixedBaseSplitImpulseOCPTest, KKTErrorNormEmptyCost) {
  auto empty_cost = std::make_shared<ImpulseCostFunction>();
  SplitImpulseOCP ocp(robot, empty_cost, constraints);
  ocp.initConstraints(robot, s);
  constraints->setSlackAndDual(robot, constraints_data, s);
  ocp.computeKKTResidual(robot, contact_status, t, q_prev, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual();
  kkt_residual.setContactStatus(contact_status);
  kkt_matrix.setContactStatus(contact_status);
  s.setContactStatus(contact_status);
  constraints->augmentDualResidual(robot, constraints_data, s, kkt_residual);
  stateequation::LinearizeImpulseForwardEuler(robot, q_prev, s, s_next, kkt_matrix, kkt_residual);
  impulse_dynamics.linearizeImpulseDynamics(robot, contact_status, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = 0;
  kkt_error_ref += kkt_residual.lq().squaredNorm();
  kkt_error_ref += kkt_residual.lv().squaredNorm();
  kkt_error_ref += kkt_residual.ldv.squaredNorm();
  kkt_error_ref += kkt_residual.lf().squaredNorm();
  kkt_error_ref += stateequation::SquaredNormStateEuqationResidual(kkt_residual);
  kkt_error_ref += impulse_dynamics.squaredNormImpulseDynamicsResidual(kkt_residual);
  kkt_error_ref += constraints->squaredNormPrimalAndDualResidual(constraints_data);
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


TEST_F(FixedBaseSplitImpulseOCPTest, KKTErrorNormEmptyConstraints) {
  auto empty_constraints = std::make_shared<ImpulseConstraints>();
  SplitImpulseOCP ocp(robot, cost, empty_constraints);
  ocp.initConstraints(robot, s);
  constraints->setSlackAndDual(robot, constraints_data, s);
  ocp.computeKKTResidual(robot, contact_status, t, q_prev, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual();
  kkt_residual.setContactStatus(contact_status);
  kkt_matrix.setContactStatus(contact_status);
  s.setContactStatus(contact_status);
  cost->computeStageCostDerivatives(robot, cost_data, t, s, kkt_residual);
  stateequation::LinearizeImpulseForwardEuler(robot, q_prev, s, s_next, kkt_matrix, kkt_residual);
  impulse_dynamics.linearizeImpulseDynamics(robot, contact_status, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = 0;
  kkt_error_ref += kkt_residual.lq().squaredNorm();
  kkt_error_ref += kkt_residual.lv().squaredNorm();
  kkt_error_ref += kkt_residual.ldv.squaredNorm();
  kkt_error_ref += kkt_residual.lf().squaredNorm();
  kkt_error_ref += stateequation::SquaredNormStateEuqationResidual(kkt_residual);
  kkt_error_ref += impulse_dynamics.squaredNormImpulseDynamicsResidual(kkt_residual);
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


TEST_F(FixedBaseSplitImpulseOCPTest, KKTErrorNorm) {
  SplitImpulseOCP ocp(robot, cost, constraints);
  ocp.initConstraints(robot, s);
  constraints->setSlackAndDual(robot, constraints_data, s);
  ocp.computeKKTResidual(robot, contact_status, t, q_prev, s, s_next);
  const double kkt_error = ocp.squaredNormKKTResidual();
  kkt_matrix.setContactStatus(contact_status);
  kkt_residual.setContactStatus(contact_status);
  kkt_residual.setZero();
  robot.updateKinematics(s.q, s.v);
  cost->computeStageCostDerivatives(robot, cost_data, t, s, kkt_residual);
  constraints->augmentDualResidual(robot, constraints_data, s, kkt_residual);
  stateequation::LinearizeImpulseForwardEuler(robot, q_prev, s, s_next, kkt_matrix, kkt_residual);
  impulse_dynamics.linearizeImpulseDynamics(robot, contact_status, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = 0;
  kkt_error_ref += kkt_residual.lq().squaredNorm();
  kkt_error_ref += kkt_residual.lv().squaredNorm();
  kkt_error_ref += kkt_residual.ldv.squaredNorm();
  kkt_error_ref += kkt_residual.lf().squaredNorm();
  kkt_error_ref += stateequation::SquaredNormStateEuqationResidual(kkt_residual);
  kkt_error_ref += impulse_dynamics.squaredNormImpulseDynamicsResidual(kkt_residual);
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


TEST_F(FixedBaseSplitImpulseOCPTest, costAndConstraintViolation) {
  SplitImpulseOCP ocp(robot, cost, constraints);
  ocp.initConstraints(robot, s);
  constraints->setSlackAndDual(robot, constraints_data, s);
  ocp.computeKKTResidual(robot, contact_status, t, q_prev, s, s_next);
  const auto pair = ocp.costAndConstraintViolation(robot, t, s); 
  const double cost_ref 
      = cost->l(robot, cost_data, t, s) 
          + constraints->costSlackBarrier(constraints_data);
  EXPECT_DOUBLE_EQ(pair.first, cost_ref);
  robot.subtractConfiguration(s.q, s_next.q, kkt_residual.Fq());
  kkt_residual.Fv() = s.v + s.dv - s_next.v;
  robot.setContactForces(contact_status, s.f);
  robot.RNEAImpulse(s.q, s.dv, kkt_residual.dv_res);
  robot.updateKinematics(s.q, s.v);
  robot.computeContactVelocityResidual(contact_status, kkt_residual.C());
  constraints->computePrimalAndDualResidual(robot, constraints_data, s);
  const double violation_ref 
      = kkt_residual.Fx().lpNorm<1>()  
          + kkt_residual.dv_res.lpNorm<1>() 
          + kkt_residual.C().lpNorm<1>()
          + constraints->l1NormPrimalResidual(constraints_data);
  EXPECT_DOUBLE_EQ(pair.second, violation_ref);
}


TEST_F(FixedBaseSplitImpulseOCPTest, costAndConstraintViolationWithStepSize) {
  SplitImpulseOCP ocp(robot, cost, constraints);
  ocp.initConstraints(robot, s);
  constraints->setSlackAndDual(robot, constraints_data, s);
  const double step_size = 0.3;
  const auto pair = ocp.costAndConstraintViolation(robot, contact_status, 
                                                   step_size, t, s, d, 
                                                   s_next, d_next); 
  robot.integrateConfiguration(s.q, d.dq(), step_size, s_tmp.q);
  s_tmp.v = s.v + step_size * d.dv();
  s_tmp.dv = s.dv + step_size * d.ddv;
  s_tmp.f_stack() = s.f_stack() + step_size * d.df();
  s_tmp.set_f();
  robot.updateKinematics(s_tmp.q, s_tmp.v);
  const double cost_ref 
      = cost->l(robot, cost_data, t, s_tmp) 
          + constraints->costSlackBarrier(constraints_data, step_size);
  EXPECT_DOUBLE_EQ(pair.first, cost_ref);
  robot.subtractConfiguration(s_tmp.q, s_next.q, kkt_residual.Fq());
  kkt_residual.Fq() -= step_size * d_next.dq();
  kkt_residual.Fv() = s_tmp.v + s_tmp.dv - s_next.v - step_size * d_next.dv();
  robot.setContactForces(contact_status, s_tmp.f);
  robot.RNEAImpulse(s_tmp.q, s_tmp.dv, kkt_residual.dv_res);
  robot.updateKinematics(s_tmp.q, s_tmp.v);
  robot.computeContactVelocityResidual(contact_status, kkt_residual.C());
  constraints->computePrimalAndDualResidual(robot, constraints_data, s_tmp);
  double violation_ref = 0;
  violation_ref += kkt_residual.Fx().lpNorm<1>();
  violation_ref += kkt_residual.dv_res.lpNorm<1>();
  violation_ref += kkt_residual.C().lpNorm<1>();
  violation_ref += constraints->l1NormPrimalResidual(constraints_data);
  EXPECT_DOUBLE_EQ(pair.second, violation_ref);
}


TEST_F(FixedBaseSplitImpulseOCPTest, riccatiRecursion) {
  const int dimv = robot.dimv();
  const int dimf = contact_status.dimf();
  const int dimc = contact_status.dimf();
  SplitImpulseOCP ocp(robot, cost, constraints);
  while (!ocp.isFeasible(robot, s)) {
    s.v = Eigen::VectorXd::Random(dimv);
    s.dv = Eigen::VectorXd::Random(dimv);
  }
  ASSERT_TRUE(ocp.isFeasible(robot, s));
  ocp.initConstraints(robot, s);
  ocp.linearizeOCP(robot, contact_status, t, q_prev, s, s_next);
  const auto pair = ocp.costAndConstraintViolation(robot, t, s); 
  RiccatiFactorization riccati_next(robot);
  const Eigen::MatrixXd seed_mat = Eigen::MatrixXd::Random(2*dimv, 2*dimv);
  const Eigen::MatrixXd P = seed_mat * seed_mat.transpose() + Eigen::MatrixXd::Identity(2*dimv, 2*dimv);
  riccati_next.Pqq = P.topLeftCorner(dimv, dimv);
  riccati_next.Pqv = P.topRightCorner(dimv, dimv);
  riccati_next.Pvq = riccati_next.Pqv.transpose();
  riccati_next.Pvv = P.bottomRightCorner(dimv, dimv);
  RiccatiFactorization riccati(robot);
  ocp.backwardRiccatiRecursion(riccati_next, riccati);
  ocp.forwardRiccatiRecursion(d, d_next);
  robot.updateKinematics(s.q, s.v);
  cost_->computeStageCostHessian(robot, cost_data_, t, s, kkt_matrix_);
  cost_->computeStageCostDerivatives(robot, cost_data_, t, s, kkt_residual_);
  constraints->setSlackAndDual(robot, constraints_data, s);
  constraints->augmentDualResidual(robot, constraints_data, s, kkt_residual);
  constraints->condenseSlackAndDual(robot, constraints_data, s, kkt_matrix, kkt_residual);
  Eigen::MatrixXd ddv_dq = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd ddv_ddv = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd ddv_df = Eigen::MatrixXd::Zero(dimv, dimf);
  robot.subtractConfiguration(s.q, s_next.q, kkt_residual.Fq());
  kkt_residual.Fv() = s.v + s.dv - s_next.v;
  robot.setContactForces(contact_status, s.f);
  robot.RNEAImpulse(s.q, s.v, kkt_residual.dv_res);
  robot.RNEAImpulseDerivatives(s.q, s.v, ddv_dq, ddv_ddv);
  robot.dRNEAPartialdFext(contact_status, ddv_df);
  robot.computeVelocityResidual(contact_status, kkt_residual.C());
  robot.computeVelocityDerivatives(contact_status, kkt_matrix.Cq(), kkt_matrix.Cv());
  kkt_residual.lq() += ddv_dq.transpose() * s.beta;
  kkt_residual.ldv += ddv_ddv.transpose() * s.beta;
  kkt_residual.lf() += ddv_df.transpose() * s.beta;
  kkt_residual.lq() += s_next.lmd - s.lmd;
  kkt_residual.lv() += s_next.gmm - s.gmm;
  kkt_residual.la() += s_next.gmm;
  kkt_residual.lq() += kkt_matrix.Cq().transpose() * s.mu_stack();
  kkt_residual.lv() += kkt_matrix.Cv().transpose() * s.mu_stack();
  kkt_residual.ldv += kkt_matrix.Cv().transpose() * s.mu_stack();
  Eigen::MatrixXd MJTJinv = Eigen::MatrixXd::Zero(dimv+dimf, dimv+dimf);
  MJTJinv.topLeftCorner(dimv, dimv) = ddv_ddv;


  kkt_matrix.Qqq() = du_dq.transpose() * kkt_matrix.Quu * du_dq;
  kkt_matrix.Qqv() = du_dq.transpose() * kkt_matrix.Quu * du_dv;
  kkt_matrix.Qqa() = du_dq.transpose() * kkt_matrix.Quu * du_da;
  kkt_matrix.Qvv() = du_dv.transpose() * kkt_matrix.Quu * du_dv;
  kkt_matrix.Qva() = du_dv.transpose() * kkt_matrix.Quu * du_da;
  kkt_matrix.Qaa() = du_da.transpose() * kkt_matrix.Quu * du_da;
  kkt_matrix.Qqf() = du_dq.transpose() * kkt_matrix.Quu * du_df;
  kkt_matrix.Qvf() = du_dv.transpose() * kkt_matrix.Quu * du_df;
  kkt_matrix.Qaf() = du_da.transpose() * kkt_matrix.Quu * du_df;
  kkt_matrix.Qff() = du_df.transpose() * kkt_matrix.Quu * du_df;
  cost->computeStageCostHessian(robot, cost_data, t, dtau, s, kkt_matrix);
  inverter.setContactStatus(contact_status);
  gain.setContactStatus(contact_status);
  kkt_matrix.Qvq() = kkt_matrix.Qqv().transpose();
  kkt_matrix.Qaq() = kkt_matrix.Qqa().transpose();
  kkt_matrix.Qav() = kkt_matrix.Qva().transpose();
  if (dimf > 0) {
    kkt_matrix.Qfq() = kkt_matrix.Qqf().transpose();
    kkt_matrix.Qfv() = kkt_matrix.Qvf().transpose();
    kkt_matrix.Qfa() = kkt_matrix.Qaf().transpose();
  }
  factorizer.factorize_F(dtau, riccati_next.Pqq, riccati_next.Pqv, 
                         riccati_next.Pvq, riccati_next.Pvv, 
                         kkt_matrix.Qqq(), kkt_matrix.Qqv(), 
                         kkt_matrix.Qvq(), kkt_matrix.Qvv());
  factorizer.factorize_H(dtau, riccati_next.Pqv, riccati_next.Pvv, 
                         kkt_matrix.Qqa(), kkt_matrix.Qva());
  factorizer.factorize_G(dtau, riccati_next.Pvv, kkt_matrix.Qaa());
  factorizer.factorize_la(dtau, riccati_next.Pvq, riccati_next.Pvv, 
                          kkt_residual.Fq(), kkt_residual.Fv(), 
                          riccati_next.sv, kkt_residual.la());
  kkt_matrix.Qaq() = kkt_matrix.Qqa().transpose();
  kkt_matrix.Qav() = kkt_matrix.Qva().transpose();

  Eigen::MatrixXd G = Eigen::MatrixXd::Zero(dimv+dimf+dimc, dimv+dimf+dimc);
  G.topLeftCorner(dimv+dimf, dimv+dimf) = kkt_matrix.Qafaf();
  G.topRightCorner(dimv+dimf, dimc) = kkt_matrix.Caf().transpose();
  G.bottomLeftCorner(dimc, dimv+dimf) = kkt_matrix.Caf();
  const Eigen::MatrixXd Ginv = G.inverse();
  // Eigen::MatrixXd Ginv = Eigen::MatrixXd::Zero(dimv+dimf+dimc, dimv+dimf+dimc);
  // inverter.invert(kkt_matrix.Qafaf(), kkt_matrix.Caf(), Ginv);
  Eigen::MatrixXd Qqvaf = Eigen::MatrixXd::Zero(2*dimv, dimv+dimf);
  Qqvaf.topLeftCorner(dimv, dimv) = kkt_matrix.Qqa();
  Qqvaf.topRightCorner(dimv, dimf) = kkt_matrix.Qqf();
  Qqvaf.bottomLeftCorner(dimv, dimv) = kkt_matrix.Qva();
  Qqvaf.bottomRightCorner(dimv, dimf) = kkt_matrix.Qvf();
  EXPECT_TRUE(Qqvaf.transpose().isApprox(kkt_matrix.Qafqv()));
  gain.computeFeedbackGain(Ginv, Qqvaf.transpose(), kkt_matrix.Cqv());
  gain.computeFeedforward(Ginv, kkt_residual.laf(), kkt_residual.C());
  Eigen::MatrixXd Pqq_ref = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Pqv_ref = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Pvq_ref = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Pvv_ref = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::VectorXd sq_ref = Eigen::VectorXd::Zero(dimv);
  Eigen::VectorXd sv_ref = Eigen::VectorXd::Zero(dimv);
  Pqq_ref = kkt_matrix.Qqq();
  Pqq_ref += gain.Kaq().transpose() * kkt_matrix.Qqa().transpose();
  Pqv_ref = kkt_matrix.Qqv();
  Pqv_ref += gain.Kaq().transpose() * kkt_matrix.Qva().transpose();
  Pvq_ref = kkt_matrix.Qvq();
  Pvq_ref += gain.Kav().transpose() * kkt_matrix.Qqa().transpose();
  Pvv_ref = kkt_matrix.Qvv();
  Pvv_ref += gain.Kav().transpose() * kkt_matrix.Qva().transpose();
  Pqq_ref += gain.Kfq().transpose() * kkt_matrix.Qqf().transpose();
  Pqv_ref += gain.Kfq().transpose() * kkt_matrix.Qvf().transpose();
  Pvq_ref += gain.Kfv().transpose() * kkt_matrix.Qqf().transpose();
  Pvv_ref += gain.Kfv().transpose() * kkt_matrix.Qvf().transpose();
  Pqq_ref += gain.Kmuq().transpose() * kkt_matrix.Cq();
  Pqv_ref += gain.Kmuq().transpose() * kkt_matrix.Cv();
  Pvq_ref += gain.Kmuv().transpose() * kkt_matrix.Cq();
  Pvv_ref += gain.Kmuv().transpose() * kkt_matrix.Cv();
  sq_ref = riccati_next.sq - kkt_residual.lq();
  sq_ref -= riccati_next.Pqq * kkt_residual.Fq();
  sq_ref -= riccati_next.Pqv * kkt_residual.Fv();
  sq_ref -= kkt_matrix.Qqa() * gain.ka();
  sv_ref = dtau * riccati_next.sq + riccati_next.sv - kkt_residual.lv();
  sv_ref -= dtau * riccati_next.Pqq * kkt_residual.Fq();
  sv_ref -= riccati_next.Pvq * kkt_residual.Fq();
  sv_ref -= dtau * riccati_next.Pqv * kkt_residual.Fv();
  sv_ref -= riccati_next.Pvv * kkt_residual.Fv();
  sv_ref -= kkt_matrix.Qva() * gain.ka();
  sq_ref -= kkt_matrix.Qqf() * gain.kf();
  sv_ref -= kkt_matrix.Qvf() * gain.kf();
  sq_ref -= kkt_matrix.Cq().transpose() * gain.kmu();
  sv_ref -= kkt_matrix.Cv().transpose() * gain.kmu();
  EXPECT_TRUE(riccati.Pqq.isApprox(Pqq_ref, 1.0e-10));
  EXPECT_TRUE(riccati.Pqv.isApprox(Pqv_ref, 1.0e-10));
  EXPECT_TRUE(riccati.Pvq.isApprox(Pvq_ref, 1.0e-10));
  EXPECT_TRUE(riccati.Pvv.isApprox(Pvv_ref, 1.0e-10));
  EXPECT_TRUE(riccati.sq.isApprox(sq_ref, 1.0e-10));
  EXPECT_TRUE(riccati.sv.isApprox(sv_ref, 1.0e-10));
  std::cout << riccati.Pqq - Pqq_ref << std::endl;
  std::cout << riccati.Pqv - Pqv_ref << std::endl;
  std::cout << riccati.Pvq - Pvq_ref << std::endl;
  std::cout << riccati.Pvv - Pvv_ref << std::endl;
  std::cout << riccati.sq - sq_ref << std::endl;
  std::cout << riccati.sv - sv_ref << std::endl;
  EXPECT_TRUE(riccati.Pqq.transpose().isApprox(riccati.Pqq));
  EXPECT_TRUE(riccati.Pqv.transpose().isApprox(riccati.Pvq));
  EXPECT_TRUE(riccati.Pvv.transpose().isApprox(riccati.Pvv));
  const Eigen::VectorXd da_ref = gain.ka() + gain.Kaq() * d.dq() + gain.Kav() * d.dv(); 
  const Eigen::VectorXd dq_next_ref = d.dq() + dtau * d.dv() + kkt_residual.Fq();
  const Eigen::VectorXd dv_next_ref = d.dv() + dtau * d.da() + kkt_residual.Fv();
  EXPECT_TRUE(d_next.dq().isApprox(dq_next_ref, 1.0e-10));
  EXPECT_TRUE(d_next.dv().isApprox(dv_next_ref, 1.0e-10));
  gain.computeFeedbackGain(Ginv, Qqvaf.transpose(), kkt_matrix.Cqv());
  gain.computeFeedforward(Ginv, kkt_residual.laf(), kkt_residual.C());
  ocp.computeCondensedDirection(robot, dtau, s, d);
  EXPECT_TRUE(d.df().isApprox(gain.kf()+gain.Kfq()*d.dq()+gain.Kfv()*d.dv(), 1.0e-10));
  EXPECT_TRUE(d.dmu().isApprox(gain.kmu()+gain.Kmuq()*d.dq()+gain.Kmuv()*d.dv(), 1.0e-10));
  Eigen::MatrixXd Kuq = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd Kuv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  ocp.getStateFeedbackGain(Kuq, Kuv);
  Eigen::MatrixXd Kuq_ref = du_dq + du_da * gain.Kaq();
  if (dimf > 0) {
    Kuq_ref += du_df * gain.Kfq();
  }
  Eigen::MatrixXd Kuv_ref = du_dv + du_da * gain.Kav();
  if (dimf > 0) {
    Kuv_ref += du_df * gain.Kfv();
  }
  EXPECT_TRUE(Kuq.isApprox(Kuq_ref, 1.0e-10));
  EXPECT_TRUE(Kuv.isApprox(Kuv_ref, 1.0e-10));
  ocp.computeKKTResidual(robot, contact_status, t, dtau, q_prev, s, s_next);
  const auto pair_ref = ocp.costAndConstraintViolation(robot, t, dtau, s); 
  EXPECT_DOUBLE_EQ(pair.first, pair_ref.first);
  EXPECT_DOUBLE_EQ(pair.second, pair_ref.second);
  const double step_size = 0.3;
  const Eigen::VectorXd lmd_ref 
      = s.lmd + step_size * (riccati.Pqq * d.dq() + riccati.Pqv * d.dv() - riccati.sq);
  const Eigen::VectorXd gmm_ref 
      = s.gmm + step_size * (riccati.Pvq * d.dq() + riccati.Pvv * d.dv() - riccati.sv);
  Eigen::VectorXd q_ref = s.q;
  robot.integrateConfiguration(d.dq(), step_size, q_ref);
  const Eigen::VectorXd v_ref = s.v + step_size * d.dv();
  const Eigen::VectorXd a_ref = s.a + step_size * d.da();
  const Eigen::VectorXd f_ref = s.f_stack() + step_size * d.df();
  const Eigen::VectorXd mu_ref = s.mu_stack() + step_size * d.dmu();
  const Eigen::VectorXd u_ref = s.u + step_size * d.du;
  const Eigen::VectorXd beta_ref = s.beta + step_size * d.dbeta;
  ocp.updatePrimal(robot, step_size, dtau, riccati, d, s);
  EXPECT_TRUE(lmd_ref.isApprox(s.lmd));
  EXPECT_TRUE(gmm_ref.isApprox(s.gmm));
  EXPECT_TRUE(q_ref.isApprox(s.q));
  EXPECT_TRUE(v_ref.isApprox(s.v));
  EXPECT_TRUE(a_ref.isApprox(s.a));
  EXPECT_TRUE(f_ref.isApprox(s.f_stack()));
  EXPECT_TRUE(mu_ref.isApprox(s.mu_stack()));
  EXPECT_TRUE(u_ref.isApprox(s.u));
  EXPECT_TRUE(beta_ref.isApprox(s.beta));
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}