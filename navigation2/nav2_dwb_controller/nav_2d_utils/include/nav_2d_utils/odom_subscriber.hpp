/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2017, Locus Robotics
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NAV_2D_UTILS__ODOM_SUBSCRIBER_HPP_
#define NAV_2D_UTILS__ODOM_SUBSCRIBER_HPP_

#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include "nav_2d_msgs/msg/twist2_d_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "nav2_util/lifecycle_node.hpp"
#include "rclcpp/rclcpp.hpp"

namespace nav_2d_utils
{

/**
 * @class OdomSubscriber
 * Wrapper for some common odometry operations. Subscribes to the topic with a mutex.
 */
class OdomSubscriber
{
public:
  /**
   * @brief Constructor that subscribes to an Odometry topic
   *
   * @param nh NodeHandle for creating subscriber
   * @param default_topic Name of the topic that will be loaded of the odom_topic param is not set.
   */
  explicit OdomSubscriber(nav2_util::LifecycleNode & nh, std::string default_topic = "odom")
  {
    std::string odom_topic;
    nh.get_parameter_or("odom_topic", odom_topic, default_topic);
    odom_sub_ =
      nh.create_subscription<nav_msgs::msg::Odometry>(odom_topic,
        rclcpp::SystemDefaultsQoS(),
        std::bind(&OdomSubscriber::odomCallback, this, std::placeholders::_1));

    nh.declare_parameter("min_x_velocity_threshold", rclcpp::ParameterValue(0.0001));
    nh.declare_parameter("min_y_velocity_threshold", rclcpp::ParameterValue(0.0001));
    nh.declare_parameter("min_theta_velocity_threshold", rclcpp::ParameterValue(0.0001));

    nh.get_parameter("min_x_velocity_threshold", min_x_velocity_threshold_);
    nh.get_parameter("min_y_velocity_threshold", min_y_velocity_threshold_);
    nh.get_parameter("min_theta_velocity_threshold", min_theta_velocity_threshold_);
  }

  inline nav_2d_msgs::msg::Twist2D getTwist() {return odom_vel_.velocity;}
  inline nav_2d_msgs::msg::Twist2DStamped getTwistStamped() {return odom_vel_;}

protected:
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    // ROS_INFO_ONCE("odom received!");
    std::lock_guard<std::mutex> lock(odom_mutex_);
    odom_vel_.header = msg->header;
    odom_vel_.velocity.x =
      thresholded_velocity(msg->twist.twist.linear.x, min_x_velocity_threshold_);
    odom_vel_.velocity.y =
      thresholded_velocity(msg->twist.twist.linear.y, min_y_velocity_threshold_);
    odom_vel_.velocity.theta =
      thresholded_velocity(msg->twist.twist.angular.z, min_theta_velocity_threshold_);
  }

  double thresholded_velocity(double velocity, double threshold)
  {
    return (std::abs(velocity) > threshold) ? velocity : 0.0;
  }

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  nav_2d_msgs::msg::Twist2DStamped odom_vel_;
  std::mutex odom_mutex_;

  double min_x_velocity_threshold_;
  double min_y_velocity_threshold_;
  double min_theta_velocity_threshold_;
};

}  // namespace nav_2d_utils

#endif  // NAV_2D_UTILS__ODOM_SUBSCRIBER_HPP_
