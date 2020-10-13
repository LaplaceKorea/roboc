#include <string>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/impulse/impulse_split_direction.hpp"


namespace idocp {

class ImpulseSplitDirectionTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf_ = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf_ = "../urdf/anymal/anymal.urdf";
  }

  virtual void TearDown() {
  }

  double dtau_;
  std::string fixed_base_urdf_, floating_base_urdf_;
};


TEST_F(ImpulseSplitDirectionTest, fixed_base) {
  Robot robot(fixed_base_urdf_);
  std::random_device rnd;
  ImpulseSplitDirection d(robot);
  EXPECT_EQ(d.dimf(), 0);
  EXPECT_EQ(d.dimc(), 0);
  EXPECT_EQ(d.dimKKT(), 5*robot.dimv());
  EXPECT_EQ(d.max_dimKKT(), 5*robot.dimv());
  const Eigen::VectorXd split_direction = Eigen::VectorXd::Random(d.dimKKT());
  d.split_direction() = split_direction;
  const int dimc = robot.dim_passive();
  const Eigen::VectorXd dlmd = split_direction.segment(                0,  robot.dimv());
  const Eigen::VectorXd dgmm = split_direction.segment(     robot.dimv(),  robot.dimv());
  const Eigen::VectorXd dmu = split_direction.segment(    2*robot.dimv(),          dimc);
  const Eigen::VectorXd da = split_direction.segment(2*robot.dimv()+dimc,  robot.dimv());
  const Eigen::VectorXd df = split_direction.segment(3*robot.dimv()+dimc,             0);
  const Eigen::VectorXd dq = split_direction.segment(3*robot.dimv()+dimc,  robot.dimv());
  const Eigen::VectorXd dv = split_direction.segment(4*robot.dimv()+dimc,  robot.dimv());
  const Eigen::VectorXd dx = split_direction.segment(3*robot.dimv()+dimc, 2*robot.dimv());
  EXPECT_TRUE(dlmd.isApprox(d.dlmd()));
  EXPECT_TRUE(dgmm.isApprox(d.dgmm()));
  EXPECT_TRUE(dmu.isApprox(d.dmu()));
  EXPECT_TRUE(da.isApprox(d.da()));
  EXPECT_TRUE(df.isApprox(d.df()));
  EXPECT_TRUE(dq.isApprox(d.dq()));
  EXPECT_TRUE(dv.isApprox(d.dv()));
  EXPECT_TRUE(dx.isApprox(d.dx()));
  d.setZero();
  EXPECT_TRUE(d.split_direction().isZero());
  const ImpulseSplitDirection d_random = ImpulseSplitDirection::Random(robot);
  EXPECT_EQ(d_random.dlmd().size(), robot.dimv());
  EXPECT_EQ(d_random.dgmm().size(), robot.dimv());
  EXPECT_EQ(d_random.dmu().size(), dimc);
  EXPECT_EQ(d_random.da().size(), robot.dimv());
  EXPECT_EQ(d_random.df().size(), 0);
  EXPECT_EQ(d_random.dq().size(), robot.dimv());
  EXPECT_EQ(d_random.dv().size(), robot.dimv());
  EXPECT_EQ(d_random.du.size(), robot.dimv());
  EXPECT_EQ(d_random.dbeta.size(), robot.dimv());
  EXPECT_FALSE(d_random.dlmd().isZero());
  EXPECT_FALSE(d_random.dgmm().isZero());
  EXPECT_FALSE(d_random.da().isZero());
  EXPECT_FALSE(d_random.dq().isZero());
  EXPECT_FALSE(d_random.dv().isZero());
  EXPECT_FALSE(d_random.du.isZero());
  EXPECT_FALSE(d_random.dbeta.isZero());
  EXPECT_EQ(d_random.dimf(), 0);
  EXPECT_EQ(d_random.dimc(), 0);
  EXPECT_EQ(d_random.dimKKT(), 5*robot.dimv());
  EXPECT_EQ(d_random.max_dimKKT(), 5*robot.dimv());
}


TEST_F(ImpulseSplitDirectionTest, fixed_base_contact) {
  std::vector<int> contact_frames = {18};
  Robot robot(fixed_base_urdf_, contact_frames);
  std::random_device rnd;
  std::vector<bool> is_contact_active = {true};
  ContactStatus contact_status(robot.max_point_contacts());
  contact_status.setContactStatus(is_contact_active);
  ImpulseSplitDirection d(robot);
  EXPECT_EQ(d.dimf(), 0);
  EXPECT_EQ(d.dimc(), robot.dim_passive());
  EXPECT_EQ(d.dimKKT(), 5*robot.dimv());
  EXPECT_EQ(d.max_dimKKT(), 5*robot.dimv()+2*robot.max_dimf());
  d.setContactStatus(contact_status);
  EXPECT_EQ(d.dimf(), contact_status.dimf());
  EXPECT_EQ(d.dimc(), robot.dim_passive()+contact_status.dimf());
  EXPECT_EQ(d.dimKKT(), 5*robot.dimv()+2*contact_status.dimf());
  EXPECT_EQ(d.max_dimKKT(), 5*robot.dimv()+2*robot.max_dimf());
  const Eigen::VectorXd split_direction = Eigen::VectorXd::Random(d.dimKKT());
  d.split_direction() = split_direction;
  const int dimf = contact_status.dimf();
  const int dimc = robot.dim_passive() + dimf;
  const Eigen::VectorXd dlmd = split_direction.segment(                     0,  robot.dimv());
  const Eigen::VectorXd dgmm = split_direction.segment(          robot.dimv(),  robot.dimv());
  const Eigen::VectorXd dmu = split_direction.segment(         2*robot.dimv(),          dimc);
  const Eigen::VectorXd da = split_direction.segment(     2*robot.dimv()+dimc,  robot.dimv());
  const Eigen::VectorXd df = split_direction.segment(     3*robot.dimv()+dimc,          dimf);
  const Eigen::VectorXd dq = split_direction.segment(3*robot.dimv()+dimc+dimf,  robot.dimv());
  const Eigen::VectorXd dv = split_direction.segment(4*robot.dimv()+dimc+dimf,  robot.dimv());
  const Eigen::VectorXd dx = split_direction.segment(3*robot.dimv()+dimc+dimf, 2*robot.dimv());
  EXPECT_TRUE(dlmd.isApprox(d.dlmd()));
  EXPECT_TRUE(dgmm.isApprox(d.dgmm()));
  EXPECT_TRUE(dmu.isApprox(d.dmu()));
  EXPECT_TRUE(da.isApprox(d.da()));
  EXPECT_TRUE(df.isApprox(d.df()));
  EXPECT_TRUE(dq.isApprox(d.dq()));
  EXPECT_TRUE(dv.isApprox(d.dv()));
  EXPECT_TRUE(dx.isApprox(d.dx()));
  d.setZero();
  EXPECT_TRUE(d.split_direction().isZero());
  const ImpulseSplitDirection d_random = ImpulseSplitDirection::Random(robot, contact_status);
  EXPECT_EQ(d_random.dlmd().size(), robot.dimv());
  EXPECT_EQ(d_random.dgmm().size(), robot.dimv());
  EXPECT_EQ(d_random.dmu().size(), robot.dim_passive()+dimf);
  EXPECT_EQ(d_random.da().size(), robot.dimv());
  EXPECT_EQ(d_random.df().size(), dimf);
  EXPECT_EQ(d_random.dq().size(), robot.dimv());
  EXPECT_EQ(d_random.dv().size(), robot.dimv());
  EXPECT_EQ(d_random.du.size(), robot.dimv());
  EXPECT_EQ(d_random.dbeta.size(), robot.dimv());
  EXPECT_FALSE(d_random.dlmd().isZero());
  EXPECT_FALSE(d_random.dgmm().isZero());
  EXPECT_FALSE(d_random.dmu().isZero());
  EXPECT_FALSE(d_random.da().isZero());
  EXPECT_FALSE(d_random.df().isZero());
  EXPECT_FALSE(d_random.dq().isZero());
  EXPECT_FALSE(d_random.dv().isZero());
  EXPECT_FALSE(d_random.du.isZero());
  EXPECT_FALSE(d_random.dbeta.isZero());
  EXPECT_EQ(d_random.dimf(), 3);
  EXPECT_EQ(d_random.dimc(), 3);
  EXPECT_EQ(d_random.dimKKT(), 5*robot.dimv()+2*dimf);
  EXPECT_EQ(d_random.max_dimKKT(), 5*robot.dimv()+2*robot.max_dimf());
}


TEST_F(ImpulseSplitDirectionTest, floating_base) {
  std::vector<int> contact_frames = {14, 24, 34, 44};
  Robot robot(floating_base_urdf_, contact_frames);
  std::random_device rnd;
  std::vector<bool> is_contact_active;
  for (const auto frame : contact_frames) {
    is_contact_active.push_back(rnd()%2==0);
  }
  ContactStatus contact_status(robot.max_point_contacts());
  contact_status.setContactStatus(is_contact_active);
  ImpulseSplitDirection d(robot);
  EXPECT_EQ(d.dimKKT(), 5*robot.dimv()+robot.dim_passive());
  EXPECT_EQ(d.max_dimKKT(), 5*robot.dimv()+robot.dim_passive()+2*robot.max_dimf());
  d.setContactStatus(contact_status);
  EXPECT_EQ(d.dimKKT(), 5*robot.dimv()+robot.dim_passive()+2*contact_status.dimf());
  EXPECT_EQ(d.max_dimKKT(), 5*robot.dimv()+robot.dim_passive()+2*robot.max_dimf());
  const Eigen::VectorXd split_direction = Eigen::VectorXd::Random(d.dimKKT());
  d.split_direction() = split_direction;
  const int dimf = contact_status.dimf();
  const int dimc = robot.dim_passive() + dimf;
  const Eigen::VectorXd dlmd = split_direction.segment(                     0,  robot.dimv());
  const Eigen::VectorXd dgmm = split_direction.segment(          robot.dimv(),  robot.dimv());
  const Eigen::VectorXd dmu = split_direction.segment(         2*robot.dimv(),          dimc);
  const Eigen::VectorXd da = split_direction.segment(     2*robot.dimv()+dimc,  robot.dimv());
  const Eigen::VectorXd df = split_direction.segment(     3*robot.dimv()+dimc,          dimf);
  const Eigen::VectorXd dq = split_direction.segment(3*robot.dimv()+dimc+dimf,  robot.dimv());
  const Eigen::VectorXd dv = split_direction.segment(4*robot.dimv()+dimc+dimf,  robot.dimv());
  const Eigen::VectorXd dx = split_direction.segment(3*robot.dimv()+dimc+dimf, 2*robot.dimv());
  EXPECT_TRUE(dlmd.isApprox(d.dlmd()));
  EXPECT_TRUE(dgmm.isApprox(d.dgmm()));
  EXPECT_TRUE(dmu.isApprox(d.dmu()));
  EXPECT_TRUE(da.isApprox(d.da()));
  EXPECT_TRUE(df.isApprox(d.df()));
  EXPECT_TRUE(dq.isApprox(d.dq()));
  EXPECT_TRUE(dv.isApprox(d.dv()));
  EXPECT_TRUE(dx.isApprox(d.dx()));
  d.setZero();
  EXPECT_TRUE(d.split_direction().isZero());
  const ImpulseSplitDirection d_random = ImpulseSplitDirection::Random(robot, contact_status);
  EXPECT_EQ(d_random.dlmd().size(), robot.dimv());
  EXPECT_EQ(d_random.dgmm().size(), robot.dimv());
  EXPECT_EQ(d_random.dmu().size(), robot.dim_passive()+dimf);
  EXPECT_EQ(d_random.da().size(), robot.dimv());
  EXPECT_EQ(d_random.df().size(), dimf);
  EXPECT_EQ(d_random.dq().size(), robot.dimv());
  EXPECT_EQ(d_random.dv().size(), robot.dimv());
  EXPECT_EQ(d_random.du.size(), robot.dimv());
  EXPECT_EQ(d_random.dbeta.size(), robot.dimv());
  EXPECT_FALSE(d_random.dlmd().isZero());
  EXPECT_FALSE(d_random.dgmm().isZero());
  EXPECT_FALSE(d_random.dmu().isZero());
  EXPECT_FALSE(d_random.da().isZero());
  if (dimf > 0) {
    EXPECT_FALSE(d_random.df().isZero());
  }
  EXPECT_FALSE(d_random.dq().isZero());
  EXPECT_FALSE(d_random.dv().isZero());
  EXPECT_FALSE(d_random.du.isZero());
  EXPECT_FALSE(d_random.dbeta.isZero());
  EXPECT_EQ(d_random.dimf(), dimf);
  EXPECT_EQ(d_random.dimc(), 6+dimf);
  EXPECT_EQ(d_random.dimKKT(), 5*robot.dimv()+6+2*dimf);
  EXPECT_EQ(d_random.max_dimKKT(), 5*robot.dimv()+6+2*robot.max_dimf());
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}