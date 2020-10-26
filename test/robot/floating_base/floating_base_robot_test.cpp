#include <vector>
#include <string>
#include <random>
#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"
#include "pinocchio/multibody/model.hpp"
#include "pinocchio/multibody/data.hpp"
#include "pinocchio/parsers/urdf.hpp"
#include "pinocchio/algorithm/joint-configuration.hpp"
#include "pinocchio/algorithm/crba.hpp"
#include "pinocchio/algorithm/rnea.hpp"
#include "pinocchio/algorithm/rnea-derivatives.hpp"

#include "idocp/robot/point_contact.hpp"
#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"


namespace idocp {

class FloatingBaseRobotTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    urdf_ = "../../urdf/anymal/anymal.urdf";
    pinocchio::urdf::buildModel(urdf_, model_);
    data_ = pinocchio::Data(model_);
    dimq_ = model_.nq;
    dimv_ = model_.nv;
    contact_frames_ = {14, 24, 34, 44};
    contact_status_ = ContactStatus(contact_frames_.size());
    q_ = pinocchio::randomConfiguration(model_, -Eigen::VectorXd::Ones(dimq_), 
                                        Eigen::VectorXd::Ones(dimq_));
    v_ = Eigen::VectorXd::Random(dimv_);
    a_ = Eigen::VectorXd::Random(dimv_);
  }

  virtual void TearDown() {
  }

  std::string urdf_;
  pinocchio::Model model_;
  pinocchio::Data data_;
  int dimq_, dimv_, max_dimf_;
  std::vector<int> contact_frames_;
  ContactStatus contact_status_;
  Eigen::VectorXd q_, v_, a_;
};


TEST_F(FloatingBaseRobotTest, constructor) {
  // Default constructor
  Robot robot_empty;
  EXPECT_EQ(robot_empty.dimq(), 0);
  EXPECT_EQ(robot_empty.dimv(), 0);
  EXPECT_EQ(robot_empty.dimu(), 0);
  EXPECT_EQ(robot_empty.max_dimf(), 0);
  EXPECT_EQ(robot_empty.dim_passive(), 0);
  EXPECT_EQ(robot_empty.max_point_contacts(), 0);
  EXPECT_FALSE(robot_empty.has_floating_base());
  Robot robot(urdf_);
  EXPECT_EQ(robot.dimq(), dimq_);
  EXPECT_EQ(robot.dimv(), dimv_);
  EXPECT_EQ(robot.dimu(), dimv_-6);
  EXPECT_EQ(robot.max_dimf(), 0);
  EXPECT_EQ(robot.dim_passive(), 6);
  EXPECT_EQ(robot.max_point_contacts(), 0);
  EXPECT_TRUE(robot.has_floating_base());
  robot.printRobotModel();
  Robot robot_contact(urdf_, contact_frames_);
  EXPECT_EQ(robot_contact.dimq(), dimq_);
  EXPECT_EQ(robot_contact.dimv(), dimv_);
  EXPECT_EQ(robot_contact.dimu(), dimv_-6);
  EXPECT_EQ(robot_contact.max_dimf(), 12);
  EXPECT_EQ(robot_contact.dim_passive(), 6);
  EXPECT_EQ(robot_contact.max_point_contacts(), 4);
  EXPECT_TRUE(robot_contact.has_floating_base());
  EXPECT_DOUBLE_EQ(robot_contact.frictionCoefficient(0), 0.8);
  EXPECT_DOUBLE_EQ(robot_contact.restitutionCoefficient(0), 0);
  std::vector<double> mu_tmp;
  for (int i=0; i<robot_contact.max_point_contacts(); ++i) {
    mu_tmp.push_back(std::abs(Eigen::VectorXd::Random(2)[0]));
  }
  robot_contact.setFrictionCoefficient(mu_tmp);
  for (int i=0; i<robot_contact.max_point_contacts(); ++i) {
    EXPECT_DOUBLE_EQ(robot_contact.frictionCoefficient(i), mu_tmp[i]);
  }
  std::vector<double> rest_tmp;
  for (int i=0; i<robot_contact.max_point_contacts(); ++i) {
    rest_tmp.push_back(std::abs(Eigen::VectorXd::Random(2)[0]));
  }
  robot_contact.setRestitutionCoefficient(rest_tmp);
  for (int i=0; i<robot_contact.max_point_contacts(); ++i) {
    EXPECT_DOUBLE_EQ(robot_contact.restitutionCoefficient(i), rest_tmp[i]);
  }
  robot_contact.printRobotModel();
  Eigen::VectorXd effort_limit, velocity_limit, lower_position_limit, 
                  upper_position_limit;
  effort_limit = Eigen::VectorXd::Constant(dimv_-6, 80);
  velocity_limit = Eigen::VectorXd::Constant(dimv_-6, 15);
  lower_position_limit = Eigen::VectorXd::Constant(dimv_-6, -9.42);
  upper_position_limit = Eigen::VectorXd::Constant(dimv_-6, 9.42);
  EXPECT_TRUE(robot.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(
      robot.lowerJointPositionLimit()
      .isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot.upperJointPositionLimit()
      .isApprox(upper_position_limit));
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(
      robot_contact.jointVelocityLimit()
      .isApprox(velocity_limit));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit()
      .isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit()
      .isApprox(upper_position_limit));
  robot.setJointEffortLimit(effort_limit);
  robot.setJointVelocityLimit(velocity_limit);
  robot.setLowerJointPositionLimit(lower_position_limit);
  robot.setUpperJointPositionLimit(upper_position_limit);
  EXPECT_TRUE(robot.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(robot.lowerJointPositionLimit().isApprox(lower_position_limit));
  EXPECT_TRUE(robot.upperJointPositionLimit().isApprox(upper_position_limit));
}


TEST_F(FloatingBaseRobotTest, moveAssign) {
  // Default constructor
  Robot robot_empty;
  EXPECT_EQ(robot_empty.dimq(), 0);
  EXPECT_EQ(robot_empty.dimv(), 0);
  EXPECT_EQ(robot_empty.dimu(), 0);
  EXPECT_EQ(robot_empty.max_dimf(), 0);
  EXPECT_EQ(robot_empty.dim_passive(), 0);
  EXPECT_EQ(robot_empty.max_point_contacts(), 0);
  EXPECT_FALSE(robot_empty.has_floating_base());
  Robot robot(urdf_);
  EXPECT_EQ(robot.dimq(), dimq_);
  EXPECT_EQ(robot.dimv(), dimv_);
  EXPECT_EQ(robot.dimu(), dimv_-6);
  EXPECT_EQ(robot.max_dimf(), 0);
  EXPECT_EQ(robot.dim_passive(), 6);
  EXPECT_EQ(robot.max_point_contacts(), 0);
  EXPECT_TRUE(robot.has_floating_base());
  Robot robot_contact(urdf_, contact_frames_);
  EXPECT_EQ(robot_contact.dimq(), dimq_);
  EXPECT_EQ(robot_contact.dimv(), dimv_);
  EXPECT_EQ(robot_contact.dimu(), dimv_-6);
  EXPECT_EQ(robot_contact.max_dimf(), 3*contact_frames_.size());
  EXPECT_EQ(robot_contact.dim_passive(), 6);
  EXPECT_EQ(robot_contact.max_point_contacts(), contact_frames_.size());
  EXPECT_TRUE(robot_contact.has_floating_base());
  Eigen::VectorXd effort_limit, velocity_limit, lower_position_limit, 
                  upper_position_limit;
  effort_limit = Eigen::VectorXd::Constant(dimv_-6, 80);
  velocity_limit = Eigen::VectorXd::Constant(dimv_-6, 15);
  lower_position_limit = Eigen::VectorXd::Constant(dimv_-6, -9.42);
  upper_position_limit = Eigen::VectorXd::Constant(dimv_-6, 9.42);
  EXPECT_TRUE(robot_contact.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot_contact.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit()
      .isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit()
      .isApprox(upper_position_limit));
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(
      robot_contact.jointVelocityLimit()
      .isApprox(velocity_limit));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit()
      .isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit()
      .isApprox(upper_position_limit));
  Robot robot_ref = robot_contact; 
  robot_empty = std::move(robot_ref);
  EXPECT_EQ(robot_contact.dimq(), robot_empty.dimq());
  EXPECT_EQ(robot_contact.dimv(), robot_empty.dimv());
  EXPECT_EQ(robot_contact.dimu(), robot_empty.dimu());
  EXPECT_EQ(robot_contact.max_dimf(), robot_empty.max_dimf());
  EXPECT_EQ(robot_contact.dim_passive(), robot_empty.dim_passive());
  EXPECT_EQ(robot_contact.max_point_contacts(), robot_empty.max_point_contacts());
  EXPECT_TRUE(robot_empty.has_floating_base());
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(robot_empty.jointEffortLimit()));
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(robot_empty.jointEffortLimit()));
  EXPECT_TRUE(
      robot_contact.jointVelocityLimit().isApprox(robot_empty.jointVelocityLimit()));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit().isApprox(robot_empty.lowerJointPositionLimit()));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit().isApprox(robot_empty.upperJointPositionLimit()));
}


TEST_F(FloatingBaseRobotTest, moveConstructor) {
  // Default constructor
  Robot robot_contact(urdf_, contact_frames_);
  EXPECT_EQ(robot_contact.dimq(), dimq_);
  EXPECT_EQ(robot_contact.dimv(), dimv_);
  EXPECT_EQ(robot_contact.dimu(), dimv_-6);
  EXPECT_EQ(robot_contact.max_dimf(), 3*contact_frames_.size());
  EXPECT_EQ(robot_contact.dim_passive(), 6);
  EXPECT_EQ(robot_contact.max_point_contacts(), contact_frames_.size());
  EXPECT_TRUE(robot_contact.has_floating_base());
  Eigen::VectorXd effort_limit, velocity_limit, lower_position_limit, 
                  upper_position_limit;
  effort_limit = Eigen::VectorXd::Constant(dimv_-6, 80);
  velocity_limit = Eigen::VectorXd::Constant(dimv_-6, 15);
  lower_position_limit = Eigen::VectorXd::Constant(dimv_-6, -9.42);
  upper_position_limit = Eigen::VectorXd::Constant(dimv_-6, 9.42);
  EXPECT_TRUE(robot_contact.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot_contact.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit()
      .isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit()
      .isApprox(upper_position_limit));
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(
      robot_contact.jointVelocityLimit()
      .isApprox(velocity_limit));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit()
      .isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit()
      .isApprox(upper_position_limit));
  Robot robot_ref = robot_contact; 
  Robot robot_empty(std::move(robot_ref));
  EXPECT_EQ(robot_contact.dimq(), robot_empty.dimq());
  EXPECT_EQ(robot_contact.dimv(), robot_empty.dimv());
  EXPECT_EQ(robot_contact.dimu(), robot_empty.dimu());
  EXPECT_EQ(robot_contact.max_dimf(), robot_empty.max_dimf());
  EXPECT_EQ(robot_contact.dim_passive(), robot_empty.dim_passive());
  EXPECT_EQ(robot_contact.max_point_contacts(), robot_empty.max_point_contacts());
  EXPECT_TRUE(robot_empty.has_floating_base());
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(robot_empty.jointEffortLimit()));
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(robot_empty.jointEffortLimit()));
  EXPECT_TRUE(
      robot_contact.jointVelocityLimit().isApprox(robot_empty.jointVelocityLimit()));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit().isApprox(robot_empty.lowerJointPositionLimit()));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit().isApprox(robot_empty.upperJointPositionLimit()));
}


TEST_F(FloatingBaseRobotTest, integrateConfiguration) {
  Robot robot(urdf_);
  Eigen::VectorXd q_ref = q_;
  Eigen::VectorXd q_integrated = q_;
  const double integration_length = Eigen::VectorXd::Random(2)[0];
  robot.integrateConfiguration(q_, v_, integration_length, q_integrated);
  pinocchio::integrate(model_, q_, integration_length*v_, q_ref);
  EXPECT_TRUE(q_integrated.isApprox(q_ref));
  Eigen::VectorXd q = q_;
  robot.integrateConfiguration(v_, integration_length, q);
  EXPECT_TRUE(q.isApprox(q_ref));
}


TEST_F(FloatingBaseRobotTest, subtractConfiguration) {
  Robot robot(urdf_);
  Eigen::VectorXd q = q_;
  Eigen::VectorXd v_ref = v_;
  const double integration_length = std::abs(Eigen::VectorXd::Random(2)[0]);
  robot.integrateConfiguration(v_, integration_length, q);
  robot.subtractConfiguration(q, q_, v_ref);
  v_ref = v_ref / integration_length;
  EXPECT_TRUE(v_.isApprox(v_ref));
}


TEST_F(FloatingBaseRobotTest, framePosition) {
  Robot robot(urdf_);
  const int contact_frame_id = contact_frames_[0];
  robot.updateKinematics(q_, v_, a_);
  auto frame_position = robot.framePosition(contact_frame_id);
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  auto frame_position_ref = data_.oMf[contact_frame_id].translation();
  EXPECT_TRUE(frame_position.isApprox(frame_position_ref));
}


TEST_F(FloatingBaseRobotTest, frameRotation) {
  Robot robot(urdf_);
  const int contact_frame_id = contact_frames_[0];
  robot.updateKinematics(q_, v_, a_);
  auto frame_rotation = robot.frameRotation(contact_frame_id);
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  auto frame_rotation_ref = data_.oMf[contact_frame_id].rotation();
  EXPECT_TRUE(frame_rotation.isApprox(frame_rotation_ref));
}


TEST_F(FloatingBaseRobotTest, framePlacement) {
  Robot robot(urdf_);
  const int contact_frame_id = contact_frames_[0];
  robot.updateKinematics(q_, v_, a_);
  auto frame_placement = robot.framePlacement(contact_frame_id);
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  auto frame_placement_ref = data_.oMf[contact_frame_id];
  EXPECT_TRUE(frame_placement.isApprox(frame_placement_ref));
}


TEST_F(FloatingBaseRobotTest, frameJacobian) {
  Robot robot(urdf_);
  const int contact_frame_id = contact_frames_[0];
  robot.updateKinematics(q_, v_, a_);
  Eigen::MatrixXd J = Eigen::MatrixXd::Zero(6, robot.dimv());
  robot.getFrameJacobian(contact_frame_id, J);
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  Eigen::MatrixXd J_ref = Eigen::MatrixXd::Zero(6, robot.dimv());
  pinocchio::getFrameJacobian(model_, data_, contact_frame_id, pinocchio::LOCAL, J_ref);
  EXPECT_TRUE(J.isApprox(J_ref));
}


TEST_F(FloatingBaseRobotTest, baumgarteResidualAndDerivatives) {
  std::vector<PointContact> contacts_ref; 
  for (int i=0; i<contact_frames_.size(); ++i) {
    contacts_ref.push_back(PointContact(model_, contact_frames_[i]));
  }
  Robot robot(urdf_, contact_frames_);
  std::random_device rnd;
  const int segment_begin = rnd() % 5;
  Eigen::VectorXd residual 
      = Eigen::VectorXd::Zero(segment_begin+robot.max_dimf());
  Eigen::VectorXd residual_ref 
      = Eigen::VectorXd::Zero(segment_begin+robot.max_dimf());
  std::vector<bool> is_contacts_active(contacts_ref.size(), true);
  contact_status_.setContactStatus(is_contacts_active);
  EXPECT_EQ(contact_status_.dimf(), robot.max_dimf());
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    EXPECT_EQ(contact_status_.isContactActive(i), true);
  }
  const double time_step = std::abs(Eigen::Vector2d::Random(2)[0]);
  robot.updateKinematics(q_, v_, a_);
  robot.setContactPointsByCurrentKinematics();
  robot.computeBaumgarteResidual(contact_status_, time_step, residual.segment(segment_begin, robot.max_dimf()));
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].setContactPointByCurrentKinematics(data_);
  }
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeBaumgarteResidual(
        model_, data_, time_step, residual_ref.segment<3>(segment_begin+3*i));
  }
  EXPECT_TRUE(residual.isApprox(residual_ref));
  const double coeff = Eigen::VectorXd::Random(1)[0];
  robot.computeBaumgarteResidual(contact_status_, coeff, time_step, residual.segment(segment_begin, robot.max_dimf()));
  EXPECT_TRUE(residual.isApprox(coeff*residual_ref));
  Eigen::MatrixXd baumgarte_partial_q_ref
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimv_);
  Eigen::MatrixXd baumgarte_partial_v_ref 
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimv_);
  Eigen::MatrixXd baumgarte_partial_a_ref 
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimv_);
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeBaumgarteDerivatives(
        model_, data_, time_step,
        baumgarte_partial_q_ref.block(3*i, 0, 3, dimv_), 
        baumgarte_partial_v_ref.block(3*i, 0, 3, dimv_), 
        baumgarte_partial_a_ref.block(3*i, 0, 3, dimv_));
  }
  const int block_rows_begin = rnd() % 5;
  const int block_cols_begin = rnd() % 5;
  Eigen::MatrixXd baumgarte_partial_q 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  Eigen::MatrixXd baumgarte_partial_v 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  Eigen::MatrixXd baumgarte_partial_a 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  robot.computeBaumgarteDerivatives(
      contact_status_, time_step,
      baumgarte_partial_q.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv()), 
      baumgarte_partial_v.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv()), 
      baumgarte_partial_a.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv()));
  EXPECT_TRUE(
      baumgarte_partial_q.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv())
      .isApprox(baumgarte_partial_q_ref));
  EXPECT_TRUE(
      baumgarte_partial_v.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv())
      .isApprox(baumgarte_partial_v_ref));
  EXPECT_TRUE(
      baumgarte_partial_a.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv())
      .isApprox(baumgarte_partial_a_ref));
  Eigen::MatrixXd baumgarte_partial_q_coeff
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  Eigen::MatrixXd baumgarte_partial_v_coeff 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  Eigen::MatrixXd baumgarte_partial_a_coeff 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  robot.computeBaumgarteDerivatives(
      contact_status_, coeff, time_step, 
      baumgarte_partial_q_coeff.block(block_rows_begin, block_cols_begin, 
                                      robot.max_dimf(), robot.dimv()), 
      baumgarte_partial_v_coeff.block(block_rows_begin, block_cols_begin, 
                                      robot.max_dimf(), robot.dimv()), 
      baumgarte_partial_a_coeff.block(block_rows_begin, block_cols_begin, 
                                      robot.max_dimf(), robot.dimv()));
  EXPECT_TRUE(baumgarte_partial_q_coeff.isApprox(coeff*baumgarte_partial_q));
  EXPECT_TRUE(baumgarte_partial_v_coeff.isApprox(coeff*baumgarte_partial_v));
  EXPECT_TRUE(baumgarte_partial_a_coeff.isApprox(coeff*baumgarte_partial_a));
  std::cout << baumgarte_partial_q << std::endl;
  std::cout << baumgarte_partial_q_coeff << std::endl;
  std::cout << baumgarte_partial_v << std::endl;
  std::cout << baumgarte_partial_v_coeff << std::endl;
  std::cout << baumgarte_partial_a << std::endl;
  std::cout << baumgarte_partial_a_coeff << std::endl;
}


TEST_F(FloatingBaseRobotTest, contactVelocityResidualAndDerivatives) {
  std::vector<PointContact> contacts_ref; 
  for (int i=0; i<contact_frames_.size(); ++i) {
    contacts_ref.push_back(PointContact(model_, contact_frames_[i]));
  }
  Robot robot(urdf_, contact_frames_);
  std::random_device rnd;
  const int segment_begin = rnd() % 5;
  Eigen::VectorXd residual 
      = Eigen::VectorXd::Zero(segment_begin+robot.max_dimf());
  Eigen::VectorXd residual_ref 
      = Eigen::VectorXd::Zero(segment_begin+robot.max_dimf());
  std::vector<bool> is_contacts_active(contacts_ref.size(), true);
  contact_status_.setContactStatus(is_contacts_active);
  EXPECT_EQ(contact_status_.dimf(), robot.max_dimf());
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    EXPECT_EQ(contact_status_.isContactActive(i), true);
  }
  robot.updateKinematics(q_, v_);
  robot.setContactPointsByCurrentKinematics();
  robot.computeContactVelocityResidual(contact_status_, residual.segment(segment_begin, robot.max_dimf()));
  std::cout << residual.segment(segment_begin, robot.max_dimf()).transpose() << std::endl;
  EXPECT_FALSE(residual.segment(segment_begin, robot.max_dimf()).isZero());
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].setContactPointByCurrentKinematics(data_);
  }
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeContactVelocityResidual(
        model_, data_, residual_ref.segment<3>(segment_begin+3*i));
  }
  EXPECT_TRUE(residual.isApprox(residual_ref));
  Eigen::MatrixXd vel_partial_q_ref
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimv_);
  Eigen::MatrixXd vel_partial_v_ref 
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimv_);
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeContactVelocityDerivatives(
        model_, data_, vel_partial_q_ref.block(3*i, 0, 3, dimv_), 
        vel_partial_v_ref.block(3*i, 0, 3, dimv_));
  }
  const int block_rows_begin = rnd() % 5;
  const int block_cols_begin = rnd() % 5;
  Eigen::MatrixXd vel_partial_q 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimv_);
  Eigen::MatrixXd vel_partial_v 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimv_);
  robot.computeContactVelocityDerivatives(
      contact_status_, 
      vel_partial_q.block(block_rows_begin, block_cols_begin, 
                          robot.max_dimf(), robot.dimv()), 
      vel_partial_v.block(block_rows_begin, block_cols_begin, 
                          robot.max_dimf(), robot.dimv()));
  EXPECT_TRUE(
      vel_partial_q.block(block_rows_begin, block_cols_begin, 
                          robot.max_dimf(), robot.dimv())
      .isApprox(vel_partial_q_ref));
  EXPECT_TRUE(
      vel_partial_v.block(block_rows_begin, block_cols_begin, 
                          robot.max_dimf(), robot.dimv())
      .isApprox(vel_partial_v_ref));
}


TEST_F(FloatingBaseRobotTest, contactResidualAndDerivatives) {
  std::vector<PointContact> contacts_ref; 
  for (int i=0; i<contact_frames_.size(); ++i) {
    contacts_ref.push_back(PointContact(model_, contact_frames_[i]));
  }
  Robot robot(urdf_, contact_frames_);
  std::random_device rnd;
  const int segment_begin = rnd() % 5;
  Eigen::VectorXd residual 
      = Eigen::VectorXd::Zero(segment_begin+robot.max_dimf());
  Eigen::VectorXd residual_ref 
      = Eigen::VectorXd::Zero(segment_begin+robot.max_dimf());
  std::vector<bool> is_contacts_active(contacts_ref.size(), true);
  contact_status_.setContactStatus(is_contacts_active);
  EXPECT_EQ(contact_status_.dimf(), robot.max_dimf());
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    EXPECT_EQ(contact_status_.isContactActive(i), true);
  }
  const double time_step = std::abs(Eigen::Vector2d::Random(2)[0]);
  robot.updateKinematics(q_, v_, a_);
  robot.setContactPointsByCurrentKinematics();
  robot.computeContactResidual(contact_status_, residual.segment(segment_begin, robot.max_dimf()));
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].setContactPointByCurrentKinematics(data_);
  }
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeContactResidual(
        model_, data_, residual_ref.segment<3>(segment_begin+3*i));
  }
  EXPECT_TRUE(residual.isApprox(residual_ref));
  const double coeff = Eigen::VectorXd::Random(1)[0];
  robot.computeContactResidual(contact_status_, coeff, residual.segment(segment_begin, robot.max_dimf()));
  EXPECT_TRUE(residual.isApprox(coeff*residual_ref));
  Eigen::MatrixXd contact_partial_q_ref
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimv_);
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeContactDerivative(
        model_, data_, contact_partial_q_ref.block(3*i, 0, 3, dimv_));
  }
  const int block_rows_begin = rnd() % 5;
  const int block_cols_begin = rnd() % 5;
  Eigen::MatrixXd contact_partial_q 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimv_);
  robot.computeContactDerivative(
      contact_status_, 
      contact_partial_q.block(block_rows_begin, block_cols_begin, 
                              robot.max_dimf(), robot.dimv()));
  EXPECT_TRUE(
      contact_partial_q.block(block_rows_begin, block_cols_begin, 
                              robot.max_dimf(), robot.dimv())
      .isApprox(contact_partial_q_ref));
  Eigen::MatrixXd contact_partial_q_coeff
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimv_);
  robot.computeContactDerivative(
      contact_status_, coeff,  
      contact_partial_q_coeff.block(block_rows_begin, block_cols_begin, 
                                    robot.max_dimf(), robot.dimv()));
  EXPECT_TRUE(contact_partial_q_coeff.isApprox(coeff*contact_partial_q));
}


TEST_F(FloatingBaseRobotTest, RNEA) {
  // without contact
  Robot robot(urdf_);
  Eigen::VectorXd tau = Eigen::VectorXd::Zero(dimv_);
  robot.RNEA(q_, v_, a_, tau);
  Eigen::VectorXd tau_ref = pinocchio::rnea(model_, data_, q_, v_, a_);
  EXPECT_TRUE(tau_ref.isApprox(tau));
  Robot robot_contact(urdf_, contact_frames_);
  tau = Eigen::VectorXd::Zero(dimv_);
  tau_ref = Eigen::VectorXd::Zero(dimv_);
  std::vector<Eigen::Vector3d> fext;
  for (int i=0; i<contact_frames_.size(); ++i) {
    fext.push_back(Eigen::Vector3d::Random());
  }
  robot_contact.RNEA(q_, v_, a_, tau);
  tau_ref = pinocchio::rnea(model_, data_, q_, v_, a_);
  EXPECT_TRUE(tau_ref.isApprox(tau));
  // with contact
  std::vector<PointContact> contacts_ref; 
  for (int i=0; i<contact_frames_.size(); ++i) {
    contacts_ref.push_back(PointContact(model_, contact_frames_[i]));
  }
  std::vector<bool> is_contacts_active(contacts_ref.size(), true);
  contact_status_.setContactStatus(is_contacts_active);
  robot_contact.setContactForces(contact_status_, fext);
  robot_contact.RNEA(q_, v_, a_, tau);
  pinocchio::container::aligned_vector<pinocchio::Force> fjoint 
      = pinocchio::container::aligned_vector<pinocchio::Force>(
                 model_.joints.size(), pinocchio::Force::Zero());
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeJointForceFromContactForce(fext[i], fjoint);
  }
  tau_ref = pinocchio::rnea(model_, data_, q_, v_, a_, fjoint);
  EXPECT_TRUE(tau_ref.isApprox(tau));
}


TEST_F(FloatingBaseRobotTest, RNEADerivativesWithoutFext) {
  Robot robot(urdf_);
  Eigen::MatrixXd dRNEA_dq = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dv = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_da = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dq_ref = dRNEA_dq;
  Eigen::MatrixXd dRNEA_dv_ref = dRNEA_dv;
  Eigen::MatrixXd dRNEA_da_ref = dRNEA_da;
  robot.RNEADerivatives(q_, v_, a_, dRNEA_dq, dRNEA_dv, dRNEA_da);
  pinocchio::computeRNEADerivatives(model_, data_, q_, v_, a_, dRNEA_dq_ref, 
                                    dRNEA_dv_ref, dRNEA_da_ref);
  dRNEA_da_ref.triangularView<Eigen::StrictlyLower>() 
      = dRNEA_da_ref.transpose().triangularView<Eigen::StrictlyLower>();
  EXPECT_TRUE(dRNEA_dq.isApprox(dRNEA_dq_ref));
  EXPECT_TRUE(dRNEA_dv.isApprox(dRNEA_dv_ref));
  EXPECT_TRUE(dRNEA_da.isApprox(dRNEA_da_ref));
}


TEST_F(FloatingBaseRobotTest, RNEADerivativesWithContacts) {
  Robot robot(urdf_, contact_frames_);
  std::vector<Eigen::Vector3d> fext;
  for (int i=0; i<contact_frames_.size(); ++i) {
    fext.push_back(Eigen::Vector3d::Random());
  }
  Eigen::MatrixXd dRNEA_dq = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dv = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_da = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dfext = Eigen::MatrixXd::Zero(dimv_, robot.max_dimf());
  Eigen::MatrixXd dRNEA_dq_ref = dRNEA_dq;
  Eigen::MatrixXd dRNEA_dv_ref = dRNEA_dv;
  Eigen::MatrixXd dRNEA_da_ref = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dfext_ref 
      = Eigen::MatrixXd::Zero(dimv_, robot.max_dimf());
  std::vector<PointContact> contacts_ref; 
  for (int i=0; i<contact_frames_.size(); ++i) {
    contacts_ref.push_back(PointContact(model_, contact_frames_[i]));
  }
  std::vector<bool> is_contacts_active(contacts_ref.size(), true);
  contact_status_.setContactStatus(is_contacts_active);
  robot.setContactForces(contact_status_, fext);
  robot.updateKinematics(q_, v_, a_);
  robot.RNEADerivatives(q_, v_, a_, dRNEA_dq, dRNEA_dv, dRNEA_da);
  pinocchio::container::aligned_vector<pinocchio::Force> fjoint 
      = pinocchio::container::aligned_vector<pinocchio::Force>(
                 model_.joints.size(), pinocchio::Force::Zero());
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeJointForceFromContactForce(fext[i], fjoint);
  }
  pinocchio::computeRNEADerivatives(model_, data_, q_, v_, a_, fjoint, 
                                    dRNEA_dq_ref, dRNEA_dv_ref, dRNEA_da_ref);
  dRNEA_da_ref.triangularView<Eigen::StrictlyLower>() 
      = dRNEA_da_ref.transpose().triangularView<Eigen::StrictlyLower>();
  EXPECT_TRUE(dRNEA_dq.isApprox(dRNEA_dq_ref));
  EXPECT_TRUE(dRNEA_dv.isApprox(dRNEA_dv_ref));
  EXPECT_TRUE(dRNEA_da.isApprox(dRNEA_da_ref));
  robot.dRNEAPartialdFext(contact_status_, dRNEA_dfext);
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  const bool transpose_jacobian = true;
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].getContactJacobian(model_, data_, -1, 
                                       dRNEA_dfext_ref.block(0, 3*i, dimv_, 3),
                                       transpose_jacobian);
  }
  EXPECT_TRUE(dRNEA_dfext.isApprox(dRNEA_dfext_ref));
  Eigen::MatrixXd Minv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.computeMinv(dRNEA_da, Minv);
  EXPECT_TRUE((Minv*dRNEA_da).isIdentity());
}


TEST_F(FloatingBaseRobotTest, RNEAImpulse) {
  Robot robot(urdf_, contact_frames_);
  std::vector<Eigen::Vector3d> fext;
  std::vector<bool> is_contacts_active;
  for (int i=0; i<contact_frames_.size(); ++i) {
    fext.push_back(Eigen::Vector3d::Random());
    is_contacts_active.push_back(true);
  }
  Eigen::VectorXd tau = Eigen::VectorXd::Zero(dimv_);
  Eigen::VectorXd tau_ref = Eigen::VectorXd::Zero(dimv_);
  contact_status_.setContactStatus(is_contacts_active);
  robot.setContactForces(contact_status_, fext);
  robot.RNEAImpulse(q_, a_, tau);
  pinocchio::container::aligned_vector<pinocchio::Force> fjoint 
      = pinocchio::container::aligned_vector<pinocchio::Force>(
                 model_.joints.size(), pinocchio::Force::Zero());
  std::vector<PointContact> contact_ref;
  for (int i=0; i<contact_frames_.size(); ++i) {
    contact_ref.push_back(PointContact(model_, contact_frames_[i]));
  }
  for (int i=0; i<contact_frames_.size(); ++i) {
    contact_ref[i].computeJointForceFromContactForce(fext[i], fjoint);
  }
  auto model_with_grav981 = model_;
  model_.gravity.setZero();
  tau_ref = pinocchio::rnea(model_, data_, q_, 
                            Eigen::VectorXd::Zero(robot.dimv()), a_, fjoint);
  EXPECT_TRUE(tau_ref.isApprox(tau));
  robot.RNEA(q_, v_, a_, tau);
  tau_ref = pinocchio::rnea(model_with_grav981, data_, q_, v_, a_, fjoint);
  EXPECT_TRUE(tau_ref.isApprox(tau));
}


TEST_F(FloatingBaseRobotTest, RNEAImpulseDerivatives) {
  Robot robot(urdf_, contact_frames_);
  std::vector<Eigen::Vector3d> fext;
  for (int i=0; i<contact_frames_.size(); ++i) {
    fext.push_back(Eigen::Vector3d::Random());
  }
  Eigen::MatrixXd dRNEA_dq = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dv = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_ddv = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dfext = Eigen::MatrixXd::Zero(dimv_, robot.max_dimf());
  Eigen::MatrixXd dRNEA_dq_ref = dRNEA_dq;
  Eigen::MatrixXd dRNEA_dv_ref = dRNEA_dv;
  Eigen::MatrixXd dRNEA_ddv_ref = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dfext_ref 
      = Eigen::MatrixXd::Zero(dimv_, robot.max_dimf());
  std::vector<PointContact> contacts_ref; 
  for (int i=0; i<contact_frames_.size(); ++i) {
    contacts_ref.push_back(PointContact(model_, contact_frames_[i]));
  }
  std::vector<bool> is_contacts_active(contacts_ref.size(), true);
  contact_status_.setContactStatus(is_contacts_active);
  robot.setContactForces(contact_status_, fext);
  robot.updateKinematics(q_, v_);
  robot.RNEAImpulseDerivatives(q_, a_, dRNEA_dq, dRNEA_ddv);
  pinocchio::container::aligned_vector<pinocchio::Force> fjoint 
      = pinocchio::container::aligned_vector<pinocchio::Force>(
                 model_.joints.size(), pinocchio::Force::Zero());
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].computeJointForceFromContactForce(fext[i], fjoint);
  }
  auto model_with_grav981 = model_;
  model_.gravity.setZero();
  pinocchio::computeRNEADerivatives(model_, data_, q_, 
                                    Eigen::VectorXd::Zero(robot.dimv()), a_, fjoint, 
                                    dRNEA_dq_ref, dRNEA_dv_ref, dRNEA_ddv_ref);
  dRNEA_ddv_ref.triangularView<Eigen::StrictlyLower>() 
      = dRNEA_ddv_ref.transpose().triangularView<Eigen::StrictlyLower>();
  EXPECT_TRUE(dRNEA_dq.isApprox(dRNEA_dq_ref));
  EXPECT_TRUE(dRNEA_ddv.isApprox(dRNEA_ddv_ref));
  robot.dRNEAPartialdFext(contact_status_, dRNEA_dfext);
  const bool transpose_jacobian = true;
  for (int i=0; i<contacts_ref.size(); ++i) {
    contacts_ref[i].getContactJacobian(model_, data_, -1, 
                                       dRNEA_dfext_ref.block(0, 3*i, dimv_, 3),
                                       transpose_jacobian);
  }
  EXPECT_TRUE(dRNEA_dfext.isApprox(dRNEA_dfext_ref));
  dRNEA_dq.setZero(); dRNEA_dv.setZero(); dRNEA_ddv.setZero();
  dRNEA_dq_ref.setZero(); dRNEA_dv_ref.setZero(); dRNEA_ddv_ref.setZero();
  robot.RNEADerivatives(q_, v_, a_, dRNEA_dq, dRNEA_dv, dRNEA_ddv);
  pinocchio::computeRNEADerivatives(model_with_grav981, data_, q_, v_, a_, 
                                    fjoint, dRNEA_dq_ref, dRNEA_dv_ref, dRNEA_ddv_ref);
  dRNEA_ddv_ref.triangularView<Eigen::StrictlyLower>() 
      = dRNEA_ddv_ref.transpose().triangularView<Eigen::StrictlyLower>();
  EXPECT_TRUE(dRNEA_dq.isApprox(dRNEA_dq_ref));
  EXPECT_TRUE(dRNEA_dv.isApprox(dRNEA_dv_ref));
  EXPECT_TRUE(dRNEA_ddv.isApprox(dRNEA_ddv_ref));
  Eigen::MatrixXd Minv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.computeMinv(dRNEA_ddv, Minv);
  EXPECT_TRUE((Minv*dRNEA_ddv).isIdentity());
  std::cout << dRNEA_ddv << std::endl;
  std::cout << dRNEA_ddv_ref << std::endl;
}


TEST_F(FloatingBaseRobotTest, computeMJtJinv) {
  Robot robot(urdf_, contact_frames_);
  std::vector<bool> is_contacts_active;
  std::random_device rnd;
  for (int i=0; i<contact_frames_.size(); ++i) {
    is_contacts_active.push_back(rnd()%2 == 0);
  }
  contact_status_.setContactStatus(is_contacts_active);
  const int dimf = contact_status_.dimf();
  Eigen::MatrixXd dRNEA_dq = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dv = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_da = Eigen::MatrixXd::Zero(dimv_, dimv_);
  Eigen::MatrixXd dRNEA_dfext = Eigen::MatrixXd::Zero(dimv_, dimf);
  robot.updateKinematics(q_, v_, a_);
  robot.RNEADerivatives(q_, v_, a_, dRNEA_dq, dRNEA_dv, dRNEA_da);
  robot.dRNEAPartialdFext(contact_status_, dRNEA_dfext);
  Eigen::MatrixXd MJtJinv = Eigen::MatrixXd::Zero(dimv_+dimf, dimv_+dimf);
  const Eigen::MatrixXd J = - dRNEA_dfext.transpose();
  robot.computeMJtJinv(contact_status_, dRNEA_da, J, MJtJinv);
  Eigen::MatrixXd MJtJ = Eigen::MatrixXd::Zero(dimv_+dimf, dimv_+dimf);
  MJtJ.topLeftCorner(dimv_, dimv_) = dRNEA_da;
  MJtJ.topRightCorner(dimv_, dimf) = J.transpose();
  MJtJ.bottomLeftCorner(dimf, dimv_) = J;
  const Eigen::MatrixXd MJtJinv_ref = MJtJ.inverse();
  EXPECT_TRUE(MJtJinv.isApprox(MJtJinv_ref));
  EXPECT_TRUE((MJtJinv*MJtJ).isIdentity());
}


TEST_F(FloatingBaseRobotTest, generateFeasibleConfiguration) {
  Robot robot(urdf_);
  Eigen::VectorXd q = Eigen::VectorXd::Zero(robot.dimq());
  robot.generateFeasibleConfiguration(q);
  Eigen::VectorXd qmin = robot.lowerJointPositionLimit();
  Eigen::VectorXd qmax = robot.upperJointPositionLimit();
  for (int i=0; i<robot.dimq()-robot.dim_passive()-1; ++i) {
    EXPECT_TRUE(q(robot.dim_passive()+1+i) >= qmin(i));
    EXPECT_TRUE(q(robot.dim_passive()+1+i) <= qmax(i));
  }
  q = Eigen::VectorXd::Random(robot.dimq());
  Eigen::VectorXd q_ref = q;
  robot.normalizeConfiguration(q);
  pinocchio::normalize(model_, q_ref);
  EXPECT_TRUE(q.isApprox(q_ref));
}

} // namespace idocp 


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}