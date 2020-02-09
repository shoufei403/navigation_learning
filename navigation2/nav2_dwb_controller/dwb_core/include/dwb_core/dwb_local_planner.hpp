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

#ifndef DWB_CORE__DWB_LOCAL_PLANNER_HPP_
#define DWB_CORE__DWB_LOCAL_PLANNER_HPP_

#include <memory>
#include <string>
#include <vector>

#include "dwb_core/goal_checker.hpp"
#include "dwb_core/publisher.hpp"
#include "dwb_core/trajectory_critic.hpp"
#include "dwb_core/trajectory_generator.hpp"
#include "nav_2d_msgs/msg/pose2_d_stamped.hpp"
#include "nav_2d_msgs/msg/twist2_d_stamped.hpp"
#include "nav2_util/lifecycle_node.hpp"
#include "pluginlib/class_loader.hpp"

namespace dwb_core
{

/**
 * @class DWBLocalPlanner
 * @brief Plugin-based flexible local_planner
 */
class DWBLocalPlanner : public nav2_util::LifecycleHelperInterface
{
public:
  /**
   * @brief Constructor that brings up pluginlib loaders
   */
  DWBLocalPlanner(
    nav2_util::LifecycleNode::SharedPtr node, TFBufferPtr tf, CostmapROSPtr costmap_ros);

  virtual ~DWBLocalPlanner() {}

  nav2_util::CallbackReturn on_configure(const rclcpp_lifecycle::State & state) override;
  nav2_util::CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override;
  nav2_util::CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override;
  nav2_util::CallbackReturn on_cleanup(const rclcpp_lifecycle::State & state) override;

  /**
   * @brief nav_core2 setPlan - Sets the global plan
   * @param path The global plan
   */
  void setPlan(const nav_2d_msgs::msg::Path2D & path);

  /**
   * @brief nav_core2 computeVelocityCommands - calculates the best command given the current pose and velocity
   *
   * It is presumed that the global plan is already set.
   *
   * This is mostly a wrapper for the protected computeVelocityCommands
   * function which has additional debugging info.
   *
   * @param pose Current robot pose
   * @param velocity Current robot velocity
   * @return The best command for the robot to drive
   */
  nav_2d_msgs::msg::Twist2DStamped computeVelocityCommands(
    const nav_2d_msgs::msg::Pose2DStamped & pose,
    const nav_2d_msgs::msg::Twist2D & velocity);

  /**
   * @brief nav_core2 isGoalReached - Check whether the robot has reached its goal, given the current pose & velocity.
   *
   * The pose that it checks against is the last pose in the current global plan.
   * The calculation is delegated to the goal_checker plugin.
   *
   * @param pose Current pose
   * @param velocity Current velocity
   * @return True if the robot should be considered as having reached the goal.
   */
  bool isGoalReached(
    const nav_2d_msgs::msg::Pose2DStamped & pose,
    const nav_2d_msgs::msg::Twist2D & velocity);

  /**
   * @brief Score a given command. Can be used for testing.
   *
   * Given a trajectory, calculate the score where lower scores are better.
   * If the given (positive) score exceeds the best_score, calculation may be cut short, as the
   * score can only go up from there.
   *
   * @param traj Trajectory to check
   * @param best_score If positive, the threshold for early termination
   * @return The full scoring of the input trajectory
   */
  dwb_msgs::msg::TrajectoryScore scoreTrajectory(
    const dwb_msgs::msg::Trajectory2D & traj,
    double best_score = -1);

  /**
   * @brief Compute the best command given the current pose and velocity, with possible debug information
   *
   * Same as above computeVelocityCommands, but with debug results.
   * If the results pointer is not null, additional information about the twists
   * evaluated will be in results after the call.
   *
   * @param pose      Current robot pose
   * @param velocity  Current robot velocity
   * @param results   Output param, if not NULL, will be filled in with full evaluation results
   * @return          Best command
   */
  nav_2d_msgs::msg::Twist2DStamped computeVelocityCommands(
    const nav_2d_msgs::msg::Pose2DStamped & pose,
    const nav_2d_msgs::msg::Twist2D & velocity,
    std::shared_ptr<dwb_msgs::msg::LocalPlanEvaluation> & results);

protected:
  /**
   * @brief Helper method for two common operations for the operating on the global_plan
   *
   * Transforms the global plan (stored in global_plan_) relative to the pose and saves it in
   * transformed_plan and possibly publishes it. Then it takes the last pose and transforms it
   * to match the local costmap's frame
   */
  void prepareGlobalPlan(
    const nav_2d_msgs::msg::Pose2DStamped & pose, nav_2d_msgs::msg::Path2D & transformed_plan,
    nav_2d_msgs::msg::Pose2DStamped & goal_pose, bool publish_plan = true);

  /**
   * @brief Iterate through all the twists and find the best one
   */
  dwb_msgs::msg::TrajectoryScore coreScoringAlgorithm(
    const geometry_msgs::msg::Pose2D & pose,
    const nav_2d_msgs::msg::Twist2D velocity,
    std::shared_ptr<dwb_msgs::msg::LocalPlanEvaluation> & results);

  /**
   * @brief Transforms global plan into same frame as pose, clips far away poses and possibly prunes passed poses
   *
   * Three key operations
   * 1) Transforms global plan into frame of the given pose
   * 2) Only returns poses that are near the robot, i.e. whether they are likely on the local costmap
   * 3) If prune_plan_ is true, it will remove all points that we've already passed from both the transformed plan
   *     and the saved global_plan_. Technically, it iterates to a pose on the path that is within prune_distance_
   *     of the robot and erases all poses before that.
   */
  nav_2d_msgs::msg::Path2D transformGlobalPlan(const nav_2d_msgs::msg::Pose2DStamped & pose);
  nav_2d_msgs::msg::Path2D global_plan_;  ///< Saved Global Plan
  bool prune_plan_;
  double prune_distance_;
  bool debug_trajectory_details_;
  rclcpp::Duration transform_tolerance_{0, 0};

  /**
   * @brief try to resolve a possibly shortened critic name with the default namespaces and the suffix "Critic"
   *
   * @param base_name The name of the critic as read in from the parameter server
   * @return Our attempted resolution of the name, with namespace prepended and/or the suffix Critic appended
   */
  std::string resolveCriticClassName(std::string base_name);

  /**
   * @brief Load the critic parameters from the namespace
   * @param name The namespace of this planner.
   */
  void loadCritics();

  void loadBackwardsCompatibleParameters();

  nav2_util::LifecycleNode::SharedPtr node_;
  TFBufferPtr tf_;
  CostmapROSPtr costmap_ros_;

  std::unique_ptr<DWBPublisher> pub_;
  std::vector<std::string> default_critic_namespaces_;

  // Plugin handling
  pluginlib::ClassLoader<TrajectoryGenerator> traj_gen_loader_;
  TrajectoryGenerator::Ptr traj_generator_;

  pluginlib::ClassLoader<GoalChecker> goal_checker_loader_;
  GoalChecker::Ptr goal_checker_;

  pluginlib::ClassLoader<TrajectoryCritic> critic_loader_;
  std::vector<TrajectoryCritic::Ptr> critics_;
};

}  // namespace dwb_core

#endif  // DWB_CORE__DWB_LOCAL_PLANNER_HPP_
