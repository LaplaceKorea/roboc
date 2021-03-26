#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/impulse_status.hpp"
#include "idocp/impulse/impulse_split_solution.hpp"
#include "idocp/impulse/impulse_split_direction.hpp"
#include "idocp/impulse/impulse_split_kkt_matrix.hpp"
#include "idocp/impulse/impulse_split_kkt_residual.hpp"
#include "idocp/constraints/pdipm.hpp"
#include "idocp/constraints/impulse_friction_cone.hpp"

#include "robot_factory.hpp"

namespace idocp {

class ImpulseFrictionConeTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    barrier = 1.0e-04;
    dt = std::abs(Eigen::VectorXd::Random(1)[0]);
    mu = 0.7;
    fraction_to_boundary_rate = 0.995;
    cone.resize(5, 3);
    cone.setZero();
    cone <<  0,  0, -1, 
             1,  0, -(mu/std::sqrt(2)),
            -1,  0, -(mu/std::sqrt(2)),
             0,  1, -(mu/std::sqrt(2)),
             0, -1, -(mu/std::sqrt(2));
  }

  virtual void TearDown() {
  }

  void testKinematics(Robot& robot, const ImpulseStatus& impulse_status) const;
  void testfLocal2World(Robot& robot, const ImpulseStatus& impulse_status) const;
  void testIsFeasible(Robot& robot, const ImpulseStatus& impulse_status) const;
  void testSetSlackAndDual(Robot& robot, 
                           const ImpulseStatus& impulse_status) const;
  void testAugmentDualResidual(Robot& robot, 
                               const ImpulseStatus& impulse_status) const;
  void testComputePrimalAndDualResidual(Robot& robot, 
                                        const ImpulseStatus& impulse_status) const;
  void testCondenseSlackAndDual(Robot& robot, 
                                const ImpulseStatus& impulse_status) const;
  void testComputeSlackAndDualDirection(Robot& robot, 
                                        const ImpulseStatus& impulse_status) const;

  double barrier, dt, mu, fraction_to_boundary_rate;
  Eigen::MatrixXd cone;
};


void ImpulseFrictionConeTest::testKinematics(Robot& robot, 
                                             const ImpulseStatus& impulse_status) const {
  ImpulseFrictionCone limit(robot, mu); 
  EXPECT_TRUE(limit.kinematicsLevel() == KinematicsLevel::AccelerationLevel);
}


void ImpulseFrictionConeTest::testfLocal2World(Robot& robot, 
                                               const ImpulseStatus& impulse_status) const {
  const Eigen::Vector3d f_local = Eigen::Vector3d::Random();
  const Eigen::VectorXd q = robot.generateFeasibleConfiguration();
  robot.updateFrameKinematics(q);
  for (const auto frame : robot.contactFrames()) {
    const Eigen::Vector3d f_world_ref = robot.frameRotation(frame) * f_local;
    Eigen::Vector3d f_world = Eigen::Vector3d::Random();
    ImpulseFrictionCone::fLocal2World(robot, frame, f_local, f_world);
    EXPECT_TRUE(f_world.isApprox(f_world_ref));
  }
}


void ImpulseFrictionConeTest::testIsFeasible(Robot& robot, 
                                             const ImpulseStatus& impulse_status) const {
  ImpulseFrictionCone limit(robot, mu); 
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  limit.allocateExtraData(data);
  EXPECT_EQ(limit.dimc(), 5*impulse_status.maxPointContacts());
  const auto s = ImpulseSplitSolution::Random(robot, impulse_status);
  robot.updateFrameKinematics(s.q);
  if (impulse_status.hasActiveImpulse()) {
    bool feasible = true;
    for (int i=0; i<impulse_status.maxPointContacts(); ++i) {
      if (impulse_status.isImpulseActive(i)) {
        Eigen::Vector3d f_world = Eigen::Vector3d::Zero();
        ImpulseFrictionCone::fLocal2World(robot, robot.contactFrames()[i], s.f[i], f_world);
        Eigen::VectorXd res =  Eigen::VectorXd::Zero(5);
        ImpulseFrictionCone::frictionConeResidual(mu, f_world, res);
        if (res.maxCoeff() > 0) {
          feasible = false;
        }
      }
    }
    EXPECT_EQ(limit.isFeasible(robot, data, s), feasible);
  }
}


void ImpulseFrictionConeTest::testSetSlackAndDual(Robot& robot, 
                                                  const ImpulseStatus& impulse_status) const {
  ImpulseFrictionCone limit(robot, mu); 
  ConstraintComponentData data(limit.dimc(), limit.barrier()), data_ref(limit.dimc(), limit.barrier());
  limit.allocateExtraData(data);
  limit.allocateExtraData(data_ref);
  const int dimc = limit.dimc();
  const auto s = ImpulseSplitSolution::Random(robot, impulse_status);
  robot.updateFrameKinematics(s.q);
  limit.setSlackAndDual(robot, data, s);
  for (int i=0; i<impulse_status.maxPointContacts(); ++i) {
    Eigen::Vector3d f_world = Eigen::Vector3d::Zero();
    ImpulseFrictionCone::fLocal2World(robot, robot.contactFrames()[i], s.f[i], f_world);
    ImpulseFrictionCone::frictionConeResidual(mu, f_world, data_ref.residual.segment(5*i, 5));
    data_ref.slack.segment(5*i, 5) = - data_ref.residual.segment(5*i, 5);
  }
  pdipm::SetSlackAndDualPositive(barrier, data_ref);
  EXPECT_TRUE(data.isApprox(data_ref));
}


void ImpulseFrictionConeTest::testAugmentDualResidual(Robot& robot, 
                                                      const ImpulseStatus& impulse_status) const {
  ImpulseFrictionCone limit(robot, mu); 
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  limit.allocateExtraData(data);
  const int dimc = limit.dimc();
  const auto s = ImpulseSplitSolution::Random(robot, impulse_status);
  robot.updateKinematics(s.q);
  limit.setSlackAndDual(robot, data, s);
  data.slack.setRandom();
  data.dual.setRandom();
  ConstraintComponentData data_ref = data;
  ImpulseSplitKKTResidual kkt_res(robot);
  kkt_res.setImpulseStatus(impulse_status);
  kkt_res.lq().setRandom();
  kkt_res.lf().setRandom();
  ImpulseSplitKKTResidual kkt_res_ref = kkt_res;
  limit.augmentDualResidual(robot, data, s, kkt_res);
  int dimf_stack = 0;
  for (int i=0; i<impulse_status.maxPointContacts(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      Eigen::Vector3d f_world = Eigen::Vector3d::Zero();
      ImpulseFrictionCone::fLocal2World(robot, robot.contactFrames()[i], s.f[i], f_world);
      Eigen::MatrixXd J = Eigen::MatrixXd::Zero(6, robot.dimv());
      robot.getFrameJacobian(robot.contactFrames()[i], J);
      Eigen::MatrixXd dfW_dq = Eigen::MatrixXd::Zero(3, robot.dimv());
      for (int j=0; j<robot.dimv(); ++j) {
        dfW_dq.col(j) = J.template bottomRows<3>().col(j).cross(f_world);
      }
      const Eigen::MatrixXd dg_dq = cone * dfW_dq;
      const Eigen::MatrixXd dg_df = cone * robot.frameRotation(robot.contactFrames()[i]);
      kkt_res_ref.lq().noalias() 
          += dg_dq.transpose() * data_ref.dual.segment(5*i, 5);
      kkt_res_ref.lf().segment(dimf_stack, 3) 
          += dg_df.transpose() * data_ref.dual.segment(5*i, 5);
      dimf_stack += 3;
    }
  }
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
}


void ImpulseFrictionConeTest::testComputePrimalAndDualResidual(Robot& robot, 
                                                               const ImpulseStatus& impulse_status) const {
  ImpulseFrictionCone limit(robot, mu); 
  const int dimc = limit.dimc();
  const auto s = ImpulseSplitSolution::Random(robot, impulse_status);
  robot.updateKinematics(s.q);
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  limit.allocateExtraData(data);
  data.slack.setRandom();
  data.dual.setRandom();
  data.residual.setRandom();
  data.duality.setRandom();
  ConstraintComponentData data_ref = data;
  limit.computePrimalAndDualResidual(robot, data, s);
  data_ref.residual.setZero();
  data_ref.duality.setZero();
  for (int i=0; i<impulse_status.maxPointContacts(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      Eigen::Vector3d f_world = Eigen::Vector3d::Zero();
      ImpulseFrictionCone::fLocal2World(robot, robot.contactFrames()[i], s.f[i], f_world);
      ImpulseFrictionCone::frictionConeResidual(mu, f_world, data_ref.residual.segment(5*i, 5));
      data_ref.residual.template segment<5>(5*i) += data_ref.slack.segment(5*i, 5);
      for (int j=0; j<5; ++j) {
        data_ref.duality.coeffRef(5*i+j) 
            = data_ref.slack.coeff(5*i+j) * data_ref.dual.coeff(5*i+j) - barrier;
      }
    }
  }
  EXPECT_TRUE(data.isApprox(data_ref));
}


void ImpulseFrictionConeTest::testCondenseSlackAndDual(Robot& robot, 
                                                       const ImpulseStatus& impulse_status) const {
  ImpulseFrictionCone limit(robot, mu); 
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  limit.allocateExtraData(data);
  const int dimc = limit.dimc();
  const auto s = ImpulseSplitSolution::Random(robot, impulse_status);
  robot.updateKinematics(s.q);
  limit.setSlackAndDual(robot, data, s);
  data.slack.setRandom();
  data.dual.setRandom();
  data.residual.setRandom();
  data.duality.setRandom();
  ConstraintComponentData data_ref = data;
  ImpulseSplitKKTMatrix kkt_mat(robot);
  kkt_mat.setImpulseStatus(impulse_status);
  kkt_mat.Qqq().setRandom();
  kkt_mat.Qqf().setRandom();
  kkt_mat.Qff().setRandom();
  ImpulseSplitKKTResidual kkt_res(robot);
  kkt_res.setImpulseStatus(impulse_status);
  kkt_res.lq().setRandom();
  kkt_res.lf().setRandom();
  limit.augmentDualResidual(robot, data, s, kkt_res);
  ImpulseSplitKKTMatrix kkt_mat_ref = kkt_mat;
  ImpulseSplitKKTResidual kkt_res_ref = kkt_res;
  limit.condenseSlackAndDual(robot, data, s, kkt_mat, kkt_res);
  data_ref.residual.setZero();
  data_ref.duality.setZero();
  for (int i=0; i<impulse_status.maxPointContacts(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      Eigen::Vector3d f_world = Eigen::Vector3d::Zero();
      ImpulseFrictionCone::fLocal2World(robot, robot.contactFrames()[i], s.f[i], f_world);
      ImpulseFrictionCone::frictionConeResidual(mu, f_world, data_ref.residual.segment(5*i, 5));
      data_ref.residual.template segment<5>(5*i) += data_ref.slack.segment(5*i, 5);
      for (int j=0; j<5; ++j) {
        data_ref.duality.coeffRef(5*i+j) 
            = data_ref.slack.coeff(5*i+j) * data_ref.dual.coeff(5*i+j) - barrier;
      }
    }
  }
  int dimf_stack = 0;
  for (int i=0; i<impulse_status.maxPointContacts(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      Eigen::Vector3d f_world = Eigen::Vector3d::Zero();
      ImpulseFrictionCone::fLocal2World(robot, robot.contactFrames()[i], s.f[i], f_world);
      Eigen::MatrixXd J = Eigen::MatrixXd::Zero(6, robot.dimv());
      robot.getFrameJacobian(robot.contactFrames()[i], J);
      Eigen::MatrixXd dfW_dq = Eigen::MatrixXd::Zero(3, robot.dimv());
      for (int j=0; j<robot.dimv(); ++j) {
        dfW_dq.col(j) = J.template bottomRows<3>().col(j).cross(f_world);
      }
      const Eigen::MatrixXd dg_dq = cone * dfW_dq;
      const Eigen::MatrixXd dg_df = cone * robot.frameRotation(robot.contactFrames()[i]);
      Eigen::VectorXd r(5);
      r.array() = (data_ref.dual.segment(5*i, 5).array()*data_ref.residual.segment(5*i, 5).array()-data_ref.duality.segment(5*i, 5).array()) 
                  / data_ref.slack.segment(5*i, 5).array();
      kkt_res_ref.lq() += dg_dq.transpose() * r;
      kkt_res_ref.lf().template segment<3>(dimf_stack) += dg_df.transpose() * r;
      r.array() = data_ref.dual.segment(5*i, 5).array() 
                   / data_ref.slack.segment(5*i, 5).array();
      kkt_mat_ref.Qqq()
          += dg_dq.transpose() * r.asDiagonal() * dg_dq;
      kkt_mat_ref.Qqf().middleCols(dimf_stack, 3)
          += dg_dq.transpose() * r.asDiagonal() * dg_df;
      kkt_mat_ref.Qff().block(dimf_stack, dimf_stack, 3, 3)
          += dg_df.transpose() * r.asDiagonal() * dg_df;
      dimf_stack += 3;
    }
  }
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
}


void ImpulseFrictionConeTest::testComputeSlackAndDualDirection(Robot& robot, 
                                                               const ImpulseStatus& impulse_status) const {
  ImpulseFrictionCone limit(robot, mu); 
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  limit.allocateExtraData(data);
  const int dimc = limit.dimc();
  const auto s = ImpulseSplitSolution::Random(robot, impulse_status);
  limit.setSlackAndDual(robot, data, s);
  data.slack.setRandom();
  data.dual.setRandom();
  data.residual.setRandom();
  data.duality.setRandom();
  data.dslack.setRandom();
  data.ddual.setRandom();
  const auto d = ImpulseSplitDirection::Random(robot, impulse_status);
  ImpulseSplitKKTMatrix kkt_mat(robot);
  kkt_mat.setImpulseStatus(impulse_status);
  kkt_mat.Qqq().setRandom();
  kkt_mat.Qqf().setRandom();
  kkt_mat.Qff().setRandom();
  ImpulseSplitKKTResidual kkt_res(robot);
  kkt_res.setImpulseStatus(impulse_status);
  kkt_res.lq().setRandom();
  kkt_res.lf().setRandom();
  limit.augmentDualResidual(robot, data, s, kkt_res);
  limit.condenseSlackAndDual(robot, data, s, kkt_mat, kkt_res);
  ConstraintComponentData data_ref = data;
  limit.computeSlackAndDualDirection(robot, data, s, d);
  data_ref.dslack.fill(1.0);
  data_ref.ddual.fill(1.0);
  int dimf_stack = 0;
  for (int i=0; i<impulse_status.maxPointContacts(); ++i) {
    if (impulse_status.isImpulseActive(i)) {
      Eigen::Vector3d f_world = Eigen::Vector3d::Zero();
      ImpulseFrictionCone::fLocal2World(robot, robot.contactFrames()[i], s.f[i], f_world);
      Eigen::MatrixXd J = Eigen::MatrixXd::Zero(6, robot.dimv());
      robot.getFrameJacobian(robot.contactFrames()[i], J);
      Eigen::MatrixXd dfW_dq = Eigen::MatrixXd::Zero(3, robot.dimv());
      for (int j=0; j<robot.dimv(); ++j) {
        dfW_dq.col(j) = J.template bottomRows<3>().col(j).cross(f_world);
      }
      const Eigen::MatrixXd dg_dq = cone * dfW_dq;
      const Eigen::MatrixXd dg_df = cone * robot.frameRotation(robot.contactFrames()[i]);
      data_ref.dslack.segment(5*i, 5)
          = - dg_dq * d.dq() - dg_df * d.df().segment(dimf_stack, 3) 
            - data_ref.residual.segment(5*i, 5);
      for (int j=0; j<5; ++j) {
        data_ref.ddual(5*i+j) 
          = - (data_ref.dual(5*i+j)*data_ref.dslack(5*i+j)+data_ref.duality(5*i+j))
              / data_ref.slack(5*i+j);
      }
      dimf_stack += 3;
    }
  }
  EXPECT_TRUE(data.isApprox(data_ref));
}


TEST_F(ImpulseFrictionConeTest, frictionConeResidual) {
  const Eigen::Vector3d f = Eigen::Vector3d::Random();
  Eigen::VectorXd res_ref = Eigen::VectorXd::Zero(5);
  res_ref(0) = - f(2);
  res_ref(1) =   f(0) - mu * f(2) / std::sqrt(2);
  res_ref(2) = - f(0) - mu * f(2) / std::sqrt(2);
  res_ref(3) =   f(1) - mu * f(2) / std::sqrt(2);
  res_ref(4) = - f(1) - mu * f(2) / std::sqrt(2);
  Eigen::VectorXd res = Eigen::VectorXd::Zero(5);
  ImpulseFrictionCone::frictionConeResidual(mu, f, res);
  EXPECT_TRUE(res.isApprox(res_ref));
}


TEST_F(ImpulseFrictionConeTest, fixedBase) {
  const double dt = 0.01;
  auto robot = testhelper::CreateFixedBaseRobot(dt);
  auto impulse_status = robot.createImpulseStatus();
  testKinematics(robot, impulse_status);
  testfLocal2World(robot, impulse_status);
  testIsFeasible(robot, impulse_status);
  testSetSlackAndDual(robot, impulse_status);
  testAugmentDualResidual(robot, impulse_status);
  testComputePrimalAndDualResidual(robot, impulse_status);
  testCondenseSlackAndDual(robot, impulse_status);
  testComputeSlackAndDualDirection(robot, impulse_status);
  impulse_status.activateImpulse(0);
  testKinematics(robot, impulse_status);
  testfLocal2World(robot, impulse_status);
  testIsFeasible(robot, impulse_status);
  testSetSlackAndDual(robot, impulse_status);
  testAugmentDualResidual(robot, impulse_status);
  testComputePrimalAndDualResidual(robot, impulse_status);
  testCondenseSlackAndDual(robot, impulse_status);
  testComputeSlackAndDualDirection(robot, impulse_status);
}


TEST_F(ImpulseFrictionConeTest, floatingBase) {
  const double dt = 0.01;
  auto robot = testhelper::CreateFloatingBaseRobot(dt);
  auto impulse_status = robot.createImpulseStatus();
  testKinematics(robot, impulse_status);
  testfLocal2World(robot, impulse_status);
  testIsFeasible(robot, impulse_status);
  testSetSlackAndDual(robot, impulse_status);
  testAugmentDualResidual(robot, impulse_status);
  testComputePrimalAndDualResidual(robot, impulse_status);
  testCondenseSlackAndDual(robot, impulse_status);
  testComputeSlackAndDualDirection(robot, impulse_status);
  impulse_status.setRandom();
  testKinematics(robot, impulse_status);
  testfLocal2World(robot, impulse_status);
  testIsFeasible(robot, impulse_status);
  testSetSlackAndDual(robot, impulse_status);
  testAugmentDualResidual(robot, impulse_status);
  testComputePrimalAndDualResidual(robot, impulse_status);
  testCondenseSlackAndDual(robot, impulse_status);
  testComputeSlackAndDualDirection(robot, impulse_status);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}