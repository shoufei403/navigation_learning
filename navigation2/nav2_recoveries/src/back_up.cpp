// Copyright (c) 2018 Intel Corporation, 2019 Samsung Research America
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <ctime>
#include <memory>

#include "nav2_recoveries/back_up.hpp"

using namespace std::chrono_literals;

namespace nav2_recoveries
{

BackUp::BackUp(rclcpp::Node::SharedPtr & node, std::shared_ptr<tf2_ros::Buffer> tf)
: Recovery<BackUpAction>(node, "BackUp", tf)
{
  simulate_ahead_time_ = 2.0;
  node_->declare_parameter("simulate_ahead_time");
  node_->get_parameter("simulate_ahead_time", simulate_ahead_time_);
}

BackUp::~BackUp()
{
}

Status BackUp::onRun(const std::shared_ptr<const BackUpAction::Goal> command)
{
  if (command->target.y != 0.0 || command->target.z != 0.0) {
    RCLCPP_INFO(node_->get_logger(), "Backing up in Y and Z not supported, "
      "will only move in X.");
  }

  command_x_ = command->target.x;

  if (!nav2_util::getCurrentPose(initial_pose_, tf_, "odom")) {
    RCLCPP_ERROR(node_->get_logger(), "Initial robot pose is not available.");
    return Status::FAILED;
  }

  return Status::SUCCEEDED;
}

Status BackUp::onCycleUpdate()
{
  geometry_msgs::msg::PoseStamped current_pose;
  if (!nav2_util::getCurrentPose(current_pose, tf_, "odom")) {
    RCLCPP_ERROR(node_->get_logger(), "Current robot pose is not available.");
    return Status::FAILED;
  }

  double diff_x = initial_pose_.pose.position.x - current_pose.pose.position.x;
  double diff_y = initial_pose_.pose.position.y - current_pose.pose.position.y;
  double distance = sqrt(diff_x * diff_x + diff_y * diff_y);

  if (distance >= abs(command_x_)) {
    stopRobot();
    return Status::SUCCEEDED;
  }
  // TODO(mhpanah): cmd_vel value should be passed as a parameter
  geometry_msgs::msg::Twist cmd_vel;
  cmd_vel.linear.y = 0.0;
  cmd_vel.angular.z = 0.0;
  command_x_ < 0 ? cmd_vel.linear.x = -0.025 : cmd_vel.linear.x = 0.025;

  geometry_msgs::msg::Pose2D pose2d;
  pose2d.x = current_pose.pose.position.x;
  pose2d.y = current_pose.pose.position.y;
  pose2d.theta = tf2::getYaw(current_pose.pose.orientation);

  if (!isCollisionFree(distance, cmd_vel, pose2d)) {
    stopRobot();
    RCLCPP_WARN(node_->get_logger(), "Collision Ahead - Exiting BackUp");
    return Status::SUCCEEDED;
  }

  vel_pub_->publish(cmd_vel);

  return Status::RUNNING;
}

bool BackUp::isCollisionFree(
  const double & distance,
  const geometry_msgs::msg::Twist & cmd_vel,
  geometry_msgs::msg::Pose2D & pose2d)
{
  // Simulate ahead by simulate_ahead_time_ in cycle_frequency_ increments
  int cycle_count = 0;
  double sim_position_change;
  const double diff_dist = abs(command_x_) - distance;
  const int max_cycle_count = static_cast<int>(cycle_frequency_ * simulate_ahead_time_);

  while (cycle_count < max_cycle_count) {
    sim_position_change = cmd_vel.linear.x * (cycle_count / cycle_frequency_);
    pose2d.x += sim_position_change;
    cycle_count++;

    if (diff_dist - abs(sim_position_change) <= 0.) {
      break;
    }

    if (!collision_checker_->isCollisionFree(pose2d)) {
      return false;
    }
  }
  return true;
}

}  // namespace nav2_recoveries
