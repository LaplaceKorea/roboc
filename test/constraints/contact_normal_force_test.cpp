#include <string>
#include <random>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/split_kkt_matrix.hpp"
#include "idocp/ocp/split_kkt_residual.hpp"
#include "idocp/constraints/pdipm.hpp"
#include "idocp/constraints/contact_normal_force.hpp"

namespace idocp {

class ContactNormalForceTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
    barrier = 1.0e-04;
    dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
    fraction_to_boundary_rate = 0.995;
  }

  virtual void TearDown() {
  }

  void testKinematics(Robot& robot, const ContactStatus& contact_status) const;
  void testIsFeasible(Robot& robot, const ContactStatus& contact_status) const;
  void testSetSlackAndDual(Robot& robot, const ContactStatus& contact_status) const;
  void testAugmentDualResidual(Robot& robot, const ContactStatus& contact_status) const;
  void testComputePrimalAndDualResidual(Robot& robot, const ContactStatus& contact_status) const;
  void testCondenseSlackAndDual(Robot& robot, const ContactStatus& contact_status) const;
  void testComputeSlackAndDualDirection(Robot& robot, const ContactStatus& contact_status) const;

  double barrier, dtau, fraction_to_boundary_rate;
  std::string fixed_base_urdf, floating_base_urdf;
};


void ContactNormalForceTest::testKinematics(Robot& robot, const ContactStatus& contact_status) const {
  ContactNormalForce limit(robot); 
  EXPECT_FALSE(limit.useKinematics());
  EXPECT_TRUE(limit.kinematicsLevel() == KinematicsLevel::AccelerationLevel);
}


void ContactNormalForceTest::testIsFeasible(Robot& robot, const ContactStatus& contact_status) const {
  ContactNormalForce limit(robot); 
  ConstraintComponentData data(limit.dimc());
  EXPECT_EQ(limit.dimc(), contact_status.maxPointContacts());
  SplitSolution s(robot);
  s.setContactStatus(contact_status);
  s.f_stack().setZero();
  s.set_f_vector();
  if (contact_status.hasActiveContacts()) {
    EXPECT_FALSE(limit.isFeasible(robot, data, s));
  }
  s.f_stack().setConstant(1.0);
  s.set_f_vector();
  EXPECT_TRUE(limit.isFeasible(robot, data, s));
  if (contact_status.hasActiveContacts()) {
    s.f_stack().setConstant(-1.0);
    s.set_f_vector();
    EXPECT_FALSE(limit.isFeasible(robot, data, s));
  }
}


void ContactNormalForceTest::testSetSlackAndDual(Robot& robot, const ContactStatus& contact_status) const {
  ContactNormalForce limit(robot); 
  ConstraintComponentData data(limit.dimc()), data_ref(limit.dimc());
  const int dimc = limit.dimc();
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  limit.setSlackAndDual(robot, data, s);
  for (int i=0; i<contact_status.maxPointContacts(); ++i) {
    data_ref.slack.coeffRef(i) = s.f[i].coeff(2);
  }
  pdipm::SetSlackAndDualPositive(barrier, data_ref);
  EXPECT_TRUE(data.isApprox(data_ref));
}


void ContactNormalForceTest::testAugmentDualResidual(Robot& robot, const ContactStatus& contact_status) const {
  ContactNormalForce limit(robot); 
  ConstraintComponentData data(limit.dimc());
  const int dimc = limit.dimc();
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  limit.setSlackAndDual(robot, data, s);
  ConstraintComponentData data_ref = data;
  SplitKKTResidual kkt_res(robot);
  kkt_res.setContactStatus(contact_status);
  kkt_res.lf().setRandom();
  SplitKKTResidual kkt_res_ref = kkt_res;
  limit.augmentDualResidual(robot, data, dtau, s, kkt_res);
  int dimf_stack = 0;
  for (int i=0; i<contact_status.maxPointContacts(); ++i) {
    if (contact_status.isContactActive(i)) {
      kkt_res_ref.lf().segment<3>(dimf_stack).coeffRef(2) -= dtau * data_ref.dual.coeff(i);
      dimf_stack += 3;
    }
  }
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
}


void ContactNormalForceTest::testComputePrimalAndDualResidual(Robot& robot, const ContactStatus& contact_status) const {
  ContactNormalForce limit(robot); 
  const int dimc = limit.dimc();
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  ConstraintComponentData data(limit.dimc());
  data.slack.setRandom();
  data.dual.setRandom();
  ConstraintComponentData data_ref = data;
  limit.computePrimalAndDualResidual(robot, data, s);
  int dimf_stack = 0;
  for (int i=0; i<contact_status.maxPointContacts(); ++i) {
    if (contact_status.isContactActive(i)) {
      data_ref.residual.coeffRef(i) = - s.f[i].coeff(2) + data_ref.slack.coeff(i);
      data_ref.duality.coeffRef(i) = data_ref.slack.coeff(i) * data_ref.dual.coeff(i) - barrier;
    }
  }
  EXPECT_TRUE(data.isApprox(data_ref));
}


void ContactNormalForceTest::testCondenseSlackAndDual(Robot& robot, const ContactStatus& contact_status) const {
  ContactNormalForce limit(robot); 
  ConstraintComponentData data(limit.dimc());
  const int dimc = limit.dimc();
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  limit.setSlackAndDual(robot, data, s);
  ConstraintComponentData data_ref = data;
  SplitKKTMatrix kkt_mat(robot);
  kkt_mat.setContactStatus(contact_status);
  kkt_mat.Qff().setRandom();
  SplitKKTResidual kkt_res(robot);
  kkt_res.setContactStatus(contact_status);
  kkt_res.lf().setRandom();
  SplitKKTMatrix kkt_mat_ref = kkt_mat;
  SplitKKTResidual kkt_res_ref = kkt_res;
  limit.condenseSlackAndDual(robot, data, dtau, s, kkt_mat, kkt_res);
  int dimf_stack = 0;
  for (int i=0; i<contact_status.maxPointContacts(); ++i) {
    if (contact_status.isContactActive(i)) {
      data_ref.residual.coeffRef(i) = - s.f[i].coeff(2) + data_ref.slack.coeff(i);
      data_ref.duality.coeffRef(i) = data_ref.slack.coeff(i) * data_ref.dual.coeff(i) - barrier;
      dimf_stack += 3;
    }
  }
  dimf_stack = 0;
  for (int i=0; i<contact_status.maxPointContacts(); ++i) {
    if (contact_status.isContactActive(i)) {
      kkt_res_ref.lf().segment<3>(dimf_stack).coeffRef(2) 
          -= dtau * (data_ref.dual.coeff(i)*data_ref.residual.coeff(i)-data_ref.duality.coeff(i)) 
                  / data_ref.slack.coeff(i);
      kkt_mat_ref.Qff().diagonal().segment<3>(dimf_stack).coeffRef(2)
          += dtau * data_ref.dual.coeff(i) / data_ref.slack.coeff(i);
      dimf_stack += 3;
    }
  }
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
}


void ContactNormalForceTest::testComputeSlackAndDualDirection(Robot& robot, const ContactStatus& contact_status) const {
  ContactNormalForce limit(robot); 
  ConstraintComponentData data(limit.dimc());
  const int dimc = limit.dimc();
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  limit.setSlackAndDual(robot, data, s);
  data.residual.setRandom();
  data.duality.setRandom();
  ConstraintComponentData data_ref = data;
  const SplitDirection d = SplitDirection::Random(robot, contact_status);
  limit.computeSlackAndDualDirection(robot, data, s, d);
  int dimf_stack = 0;
  for (int i=0; i<contact_status.maxPointContacts(); ++i) {
    if (contact_status.isContactActive(i)) {
      data_ref.dslack.coeffRef(i) = d.df().segment<3>(dimf_stack).coeff(2) - data_ref.residual.coeff(i);
      data_ref.ddual.coeffRef(i) = - (data_ref.dual.coeff(i)*data_ref.dslack.coeff(i)+data_ref.duality.coeff(i))
                                      / data_ref.slack.coeff(i);
      dimf_stack += 3;
    }
    else {
      data_ref.residual.coeffRef(i) = 0;
      data_ref.duality.coeffRef(i)  = 0;
      data_ref.slack.coeffRef(i)    = 1.0;
      data_ref.dslack.coeffRef(i)   = fraction_to_boundary_rate;
      data_ref.dual.coeffRef(i)     = 1.0;
      data_ref.ddual.coeffRef(i)    = fraction_to_boundary_rate;
    }
  }
  EXPECT_TRUE(data.isApprox(data_ref));
}


TEST_F(ContactNormalForceTest, fixedBase) {
  const std::vector<int> frames = {18};
  Robot robot(fixed_base_urdf, frames);
  ContactStatus contact_status = robot.createContactStatus();
  contact_status.setContactStatus({false});
  testKinematics(robot, contact_status);
  testIsFeasible(robot, contact_status);
  testSetSlackAndDual(robot, contact_status);
  testAugmentDualResidual(robot, contact_status);
  testComputePrimalAndDualResidual(robot, contact_status);
  testCondenseSlackAndDual(robot, contact_status);
  testComputeSlackAndDualDirection(robot, contact_status);
  contact_status.setContactStatus({true});
  testKinematics(robot, contact_status);
  testIsFeasible(robot, contact_status);
  testSetSlackAndDual(robot, contact_status);
  testAugmentDualResidual(robot, contact_status);
  testComputePrimalAndDualResidual(robot, contact_status);
  testCondenseSlackAndDual(robot, contact_status);
  testComputeSlackAndDualDirection(robot, contact_status);
}


TEST_F(ContactNormalForceTest, floatingBase) {
  const std::vector<int> frames = {14, 24, 34, 44};
  Robot robot(floating_base_urdf, frames);
  ContactStatus contact_status = robot.createContactStatus();
  contact_status.setContactStatus({false, false, false, false});
  testKinematics(robot, contact_status);
  testIsFeasible(robot, contact_status);
  testSetSlackAndDual(robot, contact_status);
  testAugmentDualResidual(robot, contact_status);
  testComputePrimalAndDualResidual(robot, contact_status);
  testCondenseSlackAndDual(robot, contact_status);
  testComputeSlackAndDualDirection(robot, contact_status);
  contact_status.setRandom();
  testKinematics(robot, contact_status);
  testIsFeasible(robot, contact_status);
  testSetSlackAndDual(robot, contact_status);
  testAugmentDualResidual(robot, contact_status);
  testComputePrimalAndDualResidual(robot, contact_status);
  testCondenseSlackAndDual(robot, contact_status);
  testComputeSlackAndDualDirection(robot, contact_status);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}