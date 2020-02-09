// Copyright (c) 2019 Intel Corporation
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
#include <memory>
#include <thread>

#include "gtest/gtest.h"
#include "nav2_util/node_utils.hpp"
#include "nav2_util/simple_action_server.hpp"
#include "test_msgs/action/fibonacci.hpp"

using Fibonacci = test_msgs::action::Fibonacci;
using GoalHandle = rclcpp_action::ServerGoalHandle<Fibonacci>;

using std::placeholders::_1;
using namespace std::chrono_literals;

class FibonacciServerNode : public rclcpp::Node
{
public:
  FibonacciServerNode()
  : rclcpp::Node("fibonacci_server_node")
  {
  }

  ~FibonacciServerNode()
  {
  }

  void on_init()
  {
    action_server_ = std::make_unique<nav2_util::SimpleActionServer<Fibonacci>>(
      shared_from_this(),
      "fibonacci",
      std::bind(&FibonacciServerNode::execute, this));
  }

  void on_term()
  {
    action_server_.reset();
  }

  void execute()
  {
    rclcpp::Rate loop_rate(10);

preempted:
    // Initialize the goal, feedback, and result
    auto goal = action_server_->get_current_goal();
    auto feedback = std::make_shared<Fibonacci::Feedback>();
    auto result = std::make_shared<Fibonacci::Result>();

    // Fibonacci-specific initialization
    auto & sequence = feedback->sequence;
    sequence.push_back(0);
    sequence.push_back(1);

    for (int i = 1; (i < goal->order) && rclcpp::ok(); ++i) {
      // Check if this action has been canceled
      if (action_server_->is_cancel_requested()) {
        result->sequence = sequence;
        action_server_->terminate_goals(result);
        return;
      }

      // Check if we've gotten an new goal, pre-empting the current one
      if (action_server_->is_preempt_requested()) {
        action_server_->accept_pending_goal();
        goto preempted;
      }

      // Update the sequence
      sequence.push_back(sequence[i] + sequence[i - 1]);

      // Publish feedback
      action_server_->publish_feedback(feedback);
      loop_rate.sleep();
    }

    // Check if goal is done
    if (rclcpp::ok()) {
      result->sequence = sequence;
      action_server_->succeeded_current(result);
    }
  }

private:
  std::unique_ptr<nav2_util::SimpleActionServer<Fibonacci>> action_server_;
};

class RclCppFixture
{
public:
  RclCppFixture()
  {
    rclcpp::init(0, nullptr);
    server_thread_ =
      std::make_unique<std::thread>(std::bind(&RclCppFixture::server_thread_func, this));
  }

  ~RclCppFixture()
  {
    rclcpp::shutdown();
    server_thread_->join();
  }

  void server_thread_func()
  {
    auto node = std::make_shared<FibonacciServerNode>();
    node->on_init();
    rclcpp::spin(node->get_node_base_interface());
    node->on_term();
    node.reset();
  }

  std::unique_ptr<std::thread> server_thread_;
};

RclCppFixture g_rclcppfixture;

class ActionTestNode : public rclcpp::Node
{
public:
  ActionTestNode()
  : rclcpp::Node(nav2_util::generate_internal_node_name("action_test_node"))
  {
  }

  void on_init()
  {
    action_client_ = rclcpp_action::create_client<Fibonacci>(shared_from_this(), "fibonacci");
    action_client_->wait_for_action_server();
  }

  void on_term()
  {
    action_client_.reset();
  }

  rclcpp_action::Client<Fibonacci>::SharedPtr action_client_;
};

class ActionTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    node_ = std::make_shared<ActionTestNode>();
    node_->on_init();
  }

  void TearDown() override
  {
    node_->on_term();
    node_.reset();
  }

  std::shared_ptr<ActionTestNode> node_;
};

TEST_F(ActionTest, test_simple_action)
{
  // The goal for this invocation
  auto goal = Fibonacci::Goal();
  goal.order = 12;

  // Send the goal
  auto future_goal_handle = node_->action_client_->async_send_goal(goal);
  ASSERT_EQ(rclcpp::spin_until_future_complete(node_,
    future_goal_handle), rclcpp::executor::FutureReturnCode::SUCCESS);

  auto goal_handle = future_goal_handle.get();

  // Wait for the result
  auto future_result = node_->action_client_->async_get_result(goal_handle);
  ASSERT_EQ(rclcpp::spin_until_future_complete(node_, future_result),
    rclcpp::executor::FutureReturnCode::SUCCESS);

  // The final result
  rclcpp_action::ClientGoalHandle<Fibonacci>::WrappedResult result = future_result.get();
  ASSERT_EQ(result.code, rclcpp_action::ResultCode::SUCCEEDED);

  // Sum all of the values in the requested fibonacci series
  int sum = 0;
  for (auto number : result.result->sequence) {
    sum += number;
  }

  ASSERT_EQ(sum, 376);
}

TEST_F(ActionTest, test_simple_action_with_feedback)
{
  int feedback_sum = 0;

  // A callback to accumulate the intermediate values
  auto feedback_callback = [&feedback_sum](
    rclcpp_action::ClientGoalHandle<Fibonacci>::SharedPtr /*goal_handle*/,
    const std::shared_ptr<const Fibonacci::Feedback> feedback)
    {
      feedback_sum += feedback->sequence.back();
    };

  // The goal for this invocation
  auto goal = Fibonacci::Goal();
  goal.order = 10;

  auto send_goal_options = rclcpp_action::Client<Fibonacci>::SendGoalOptions();
  send_goal_options.feedback_callback = feedback_callback;

  // Send the goal
  auto future_goal_handle = node_->action_client_->async_send_goal(goal, send_goal_options);
  ASSERT_EQ(rclcpp::spin_until_future_complete(node_,
    future_goal_handle), rclcpp::executor::FutureReturnCode::SUCCESS);

  auto goal_handle = future_goal_handle.get();

  // Wait for the result
  auto future_result = node_->action_client_->async_get_result(goal_handle);
  ASSERT_EQ(rclcpp::spin_until_future_complete(node_,
    future_result), rclcpp::executor::FutureReturnCode::SUCCESS);

  // The final result
  rclcpp_action::ClientGoalHandle<Fibonacci>::WrappedResult result = future_result.get();
  ASSERT_EQ(result.code, rclcpp_action::ResultCode::SUCCEEDED);

  // Sum all of the values in the requested fibonacci series
  int sum = 0;
  for (auto number : result.result->sequence) {
    sum += number;
  }

  ASSERT_EQ(sum, 143);
  ASSERT_GE(feedback_sum, 0);  // We should have received *some* feedback
}
