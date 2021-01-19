#include <string>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/ocp/split_kkt_residual.hpp"


namespace idocp {

class SplitKKTResidualTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
  }

  static void testSize(const Robot& robot, const ContactStatus& contact_status);
  static void testIsApprox(const Robot& robot, const ContactStatus& contact_status);

  virtual void TearDown() {
  }

  std::string fixed_base_urdf, floating_base_urdf;
};


void SplitKKTResidualTest::testSize(const Robot& robot, 
                                    const ContactStatus& contact_status) {
  SplitKKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
  const int dimv = robot.dimv();
  const int dimu = robot.dimu();
  const int dimf = contact_status.dimf();
  EXPECT_EQ(kkt_residual.dimf(), dimf);
  EXPECT_EQ(kkt_residual.dimKKT(), 4*dimv+dimu);
  EXPECT_EQ(kkt_residual.splitKKTResidual().size(), 4*dimv+dimu);
  EXPECT_EQ(kkt_residual.Fq().size(), dimv);
  EXPECT_EQ(kkt_residual.Fv().size(), dimv);
  EXPECT_EQ(kkt_residual.lu().size(), dimu);
  EXPECT_EQ(kkt_residual.lq().size(), dimv);
  EXPECT_EQ(kkt_residual.lv().size(), dimv);
  EXPECT_EQ(kkt_residual.lx().size(), 2*dimv);
  EXPECT_EQ(kkt_residual.lf().size(), contact_status.dimf());
  EXPECT_EQ(kkt_residual.dimf(), contact_status.dimf());
  EXPECT_EQ(kkt_residual.la.size(), dimv);
  EXPECT_EQ(kkt_residual.lu_passive.size(), 6);
  kkt_residual.splitKKTResidual() = Eigen::VectorXd::Random(kkt_residual.dimKKT());
  const Eigen::VectorXd la_ref = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd lf_ref = Eigen::VectorXd::Random(contact_status.dimf());
  kkt_residual.la = la_ref;
  kkt_residual.lf() = lf_ref;
  const Eigen::VectorXd Fq_ref = kkt_residual.splitKKTResidual().segment(0, dimv);
  const Eigen::VectorXd Fv_ref = kkt_residual.splitKKTResidual().segment(dimv, dimv);
  const Eigen::VectorXd lu_ref = kkt_residual.splitKKTResidual().segment(2*dimv, dimu);
  const Eigen::VectorXd lq_ref = kkt_residual.splitKKTResidual().segment(2*dimv+dimu, dimv);
  const Eigen::VectorXd lv_ref = kkt_residual.splitKKTResidual().segment(3*dimv+dimu, dimv);
  const Eigen::VectorXd lx_ref = kkt_residual.splitKKTResidual().segment(2*dimv+dimu, 2*dimv);
  EXPECT_TRUE(kkt_residual.Fq().isApprox(Fq_ref));
  EXPECT_TRUE(kkt_residual.Fv().isApprox(Fv_ref));
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
                                        const ContactStatus& contact_status) {
  SplitKKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(contact_status);
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
  std::vector<int> contact_frames = {18};
  Robot robot(fixed_base_urdf, contact_frames);
  std::random_device rnd;
  ContactStatus contact_status(contact_frames.size());
  contact_status.setContactStatus({false});
  testSize(robot, contact_status);
  testIsApprox(robot, contact_status);
  contact_status.setContactStatus({true});
  testSize(robot, contact_status);
  testIsApprox(robot, contact_status);
}


TEST_F(SplitKKTResidualTest, floatingBase) {
  std::vector<int> contact_frames = {14, 24, 34, 44};
  Robot robot(floating_base_urdf, contact_frames);
  std::vector<bool> is_contact_active = {false, false, false, false};
  ContactStatus contact_status(contact_frames.size());
  contact_status.setContactStatus(is_contact_active);
  testSize(robot, contact_status);
  testIsApprox(robot, contact_status);
  is_contact_active.clear();
  std::random_device rnd;
  for (const auto frame : contact_frames) {
    is_contact_active.push_back(rnd()%2==0);
  }
  contact_status.setContactStatus(is_contact_active);
  testSize(robot, contact_status);
  testIsApprox(robot, contact_status);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}