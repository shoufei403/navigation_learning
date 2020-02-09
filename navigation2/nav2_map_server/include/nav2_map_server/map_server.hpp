// Copyright (c) 2018 Intel Corporation
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

#ifndef NAV2_MAP_SERVER__MAP_SERVER_HPP_
#define NAV2_MAP_SERVER__MAP_SERVER_HPP_

#include <memory>
#include "nav2_util/lifecycle_node.hpp"
#include "nav2_util/lifecycle_helper_interface.hpp"

namespace nav2_map_server
{

class MapServer : public nav2_util::LifecycleNode
{
public:
  MapServer();
  ~MapServer();

protected:
  // Implement the lifecycle interface
  nav2_util::CallbackReturn on_configure(const rclcpp_lifecycle::State & state) override;
  nav2_util::CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override;
  nav2_util::CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override;
  nav2_util::CallbackReturn on_cleanup(const rclcpp_lifecycle::State & state) override;
  nav2_util::CallbackReturn on_shutdown(const rclcpp_lifecycle::State & state) override;
  nav2_util::CallbackReturn on_error(const rclcpp_lifecycle::State & state) override;

  // The map loader that will actually do the work
  std::unique_ptr<nav2_util::LifecycleHelperInterface> map_loader_;
};

}  // namespace nav2_map_server

#endif  // NAV2_MAP_SERVER__MAP_SERVER_HPP_
