#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/robot/impulse_status.hpp"
#include "idocp/ocp/split_kkt_residual.hpp"

#include "robot_factory.hpp"


namespace idocp {

class SplitKKTResidualTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    dt = std::abs(Eigen::VectorXd::Random(1)[0]);
  }

  static void testSize(const Robot& robot, 
                       const ContactStatus& contact_status, 
                       const ImpulseStatus& impulse_status);
  static void testIsApprox(const Robot& robot, 
                           const ContactStatus& contact_status, 
                           const ImpulseStatus& impulse_status);

  virtual void TearDown() {
  }

  double dt;
};


void SplitKKTResidualTest::testSize(const Robot& robot, 
                                    const ContactStatus& contact_status,
                                    const ImpulseStatus& impulse_status) {
  SplitKKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  const int dimf = contact_status.dimf();
  const int dimi = impulse_status.dimf();
  EXPECT_EQ(kkt_residual.dimf(), dimf);
  EXPECT_EQ(kkt_residual.dimi(), 0);
  EXPECT_EQ(kkt_residual.splitKKTResidual().size(), 4*dimv+dimu);
  EXPECT_EQ(kkt_residual.Fq().size(), dimv);
  EXPECT_EQ(kkt_residual.Fv().size(), dimv);
  EXPECT_EQ(kkt_residual.P().size(), 0);
  EXPECT_EQ(kkt_residual.lu().size(), dimu);
  EXPECT_EQ(kkt_residual.lq().size(), dimv);
  EXPECT_EQ(kkt_residual.lv().size(), dimv);
  EXPECT_EQ(kkt_residual.lx().size(), 2*dimv);
  EXPECT_EQ(kkt_residual.lf().size(), contact_status.dimf());
  EXPECT_EQ(kkt_residual.dimf(), contact_status.dimf());
  EXPECT_EQ(kkt_residual.la.size(), dimv);
  EXPECT_EQ(kkt_residual.lu_passive.size(), 6);
  kkt_residual.setImpulseStatus(impulse_status);
  EXPECT_EQ(kkt_residual.dimf(), dimf);
  EXPECT_EQ(kkt_residual.dimi(), dimi);
  EXPECT_EQ(kkt_residual.splitKKTResidual().size(), 4*dimv+dimu+dimi);
  EXPECT_EQ(kkt_residual.Fq().size(), dimv);
  EXPECT_EQ(kkt_residual.Fv().size(), dimv);
  EXPECT_EQ(kkt_residual.P().size(), dimi);
  EXPECT_EQ(kkt_residual.lu().size(), dimu);
  EXPECT_EQ(kkt_residual.lq().size(), dimv);
  EXPECT_EQ(kkt_residual.lv().size(), dimv);
  EXPECT_EQ(kkt_residual.lx().size(), 2*dimv);
  EXPECT_EQ(kkt_residual.lf().size(), contact_status.dimf());
  EXPECT_EQ(kkt_residual.dimf(), contact_status.dimf());
  EXPECT_EQ(kkt_residual.la.size(), dimv);
  EXPECT_EQ(kkt_residual.lu_passive.size(), 6);
  kkt_residual.splitKKTResidual().setRandom();
  const Eigen::VectorXd la_ref = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd lf_ref = Eigen::VectorXd::Random(contact_status.dimf());
  kkt_residual.la = la_ref;
  kkt_residual.lf() = lf_ref;
  const Eigen::VectorXd Fq_ref = kkt_residual.splitKKTResidual().segment(0, dimv);
  const Eigen::VectorXd Fv_ref = kkt_residual.splitKKTResidual().segment(dimv, dimv);
  const Eigen::VectorXd P_ref  = kkt_residual.splitKKTResidual().segment(2*dimv, dimi);
  const Eigen::VectorXd lu_ref = kkt_residual.splitKKTResidual().segment(2*dimv+dimi, dimu);
  const Eigen::VectorXd lq_ref = kkt_residual.splitKKTResidual().segment(2*dimv+dimi+dimu, dimv);
  const Eigen::VectorXd lv_ref = kkt_residual.splitKKTResidual().segment(3*dimv+dimi+dimu, dimv);
  const Eigen::VectorXd lx_ref = kkt_residual.splitKKTResidual().segment(2*dimv+dimi+dimu, 2*dimv);
  EXPECT_TRUE(kkt_residual.Fq().isApprox(Fq_ref));
  EXPECT_TRUE(kkt_residual.Fv().isApprox(Fv_ref));
  EXPECT_TRUE(kkt_residual.P().isApprox(P_ref));
  EXPECT_TRUE(kkt_residual.lu().isApprox(lu_ref));
  EXPECT_TRUE(kkt_residual.lq().isApprox(lq_ref));
  EXPECT_TRUE(kkt_residual.lv().isApprox(lv_ref));
  EXPECT_TRUE(kkt_residual.lx().isApprox(lx_ref));
  EXPECT_TRUE(kkt_residual.la.isApprox(la_ref));
  EXPECT_TRUE(kkt_residual.lf().isApprox(lf_ref));
  SplitKKTResidual kkt_residual_ref = kkt_residual;
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual_ref.splitKKTResidual().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
}


void SplitKKTResidualTest::testIsApprox(const Robot& robot, 
                                        const ContactStatus& contact_status,
                                        const ImpulseStatus& impulse_status) {
  SplitKKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  kkt_residual.setImpulseStatus(impulse_status);
  kkt_residual.splitKKTResidual().setRandom();
  kkt_residual.la.setRandom();
  kkt_residual.lf().setRandom();
  SplitKKTResidual kkt_residual_ref = kkt_residual;
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.Fq().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.Fq() = kkt_residual_ref.Fq();
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.Fv().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.Fv() = kkt_residual_ref.Fv();
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.Fx().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.Fx() = kkt_residual_ref.Fx();
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  if (impulse_status.dimf() > 0) {
    kkt_residual.P().setRandom();
    EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
    kkt_residual.P() = kkt_residual_ref.P();
    EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  }
  kkt_residual.lu().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.lu() = kkt_residual_ref.lu();
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.lq().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.lq() = kkt_residual_ref.lq();
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.lv().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.lv() = kkt_residual_ref.lv();
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.lx().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.lx() = kkt_residual_ref.lx();
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.la.setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual.la = kkt_residual_ref.la;
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  if (contact_status.hasActiveContacts()) {
    kkt_residual_ref.lf().setRandom();
    EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  }
  else {
    kkt_residual_ref.lf().setRandom();
    EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  }
  kkt_residual_ref = kkt_residual;
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  if (robot.hasFloatingBase()) {
    kkt_residual_ref.lu_passive.setRandom();
    EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
  }
  else {
    kkt_residual_ref.lu_passive.setRandom();
    EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  }
  kkt_residual_ref = kkt_residual;
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  kkt_residual_ref.splitKKTResidual().setRandom();
  EXPECT_FALSE(kkt_residual.isApprox(kkt_residual_ref));
}


TEST_F(SplitKKTResidualTest, fixedBase) {
  auto robot = testhelper::CreateFixedBaseRobot(dt);
  auto contact_status = robot.createContactStatus();
  auto impulse_status = robot.createImpulseStatus();
  contact_status.deactivateContact(0);
  impulse_status.deactivateImpulse(0);
  testSize(robot, contact_status, impulse_status);
  testIsApprox(robot, contact_status, impulse_status);
  contact_status.activateContact(0);
  impulse_status.deactivateImpulse(0);
  testSize(robot, contact_status, impulse_status);
  testIsApprox(robot, contact_status, impulse_status);
  contact_status.deactivateContact(0);
  impulse_status.activateImpulse(0);
  testSize(robot, contact_status, impulse_status);
  testIsApprox(robot, contact_status, impulse_status);
  contact_status.activateContact(0);
  impulse_status.activateImpulse(0);
  testSize(robot, contact_status, impulse_status);
  testIsApprox(robot, contact_status, impulse_status);
}


TEST_F(SplitKKTResidualTest, floatingBase) {
  auto robot = testhelper::CreateFloatingBaseRobot(dt);
  auto contact_status = robot.createContactStatus();
  auto impulse_status = robot.createImpulseStatus();
  // Both contact and impulse are inactive
  contact_status.deactivateContacts();
  impulse_status.deactivateImpulses();
  testSize(robot, contact_status, impulse_status);
  testIsApprox(robot, contact_status, impulse_status);
  // Contacts are active and impulse are inactive
  contact_status.setRandom();
  if (!contact_status.hasActiveContacts()) {
    contact_status.activateContact(0);
  }
  impulse_status.deactivateImpulses();
  testSize(robot, contact_status, impulse_status);
  testIsApprox(robot, contact_status, impulse_status);
  // Contacts are inactive and impulse are active
  contact_status.deactivateContacts();
  impulse_status.setRandom();
  if (!impulse_status.hasActiveImpulse()) {
    impulse_status.activateImpulse(0);
  }
  testSize(robot, contact_status, impulse_status);
  testIsApprox(robot, contact_status, impulse_status);
  // Both contact and impulse are active
  contact_status.setRandom();
  if (!contact_status.hasActiveContacts()) {
    contact_status.activateContact(0);
  }
  impulse_status.setRandom();
  if (!impulse_status.hasActiveImpulse()) {
    impulse_status.activateImpulse(0);
  }
  testSize(robot, contact_status, impulse_status);
  testIsApprox(robot, contact_status, impulse_status);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}