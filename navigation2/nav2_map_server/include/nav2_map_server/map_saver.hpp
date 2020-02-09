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

#ifndef NAV2_MAP_SERVER__MAP_SAVER_HPP_
#define NAV2_MAP_SERVER__MAP_SAVER_HPP_

#include <string>
#include "map_mode.hpp"
#include "nav_msgs/srv/get_map.hpp"
#include "rclcpp/rclcpp.hpp"

namespace nav2_map_server
{
class MapSaver : public rclcpp::Node
{
public:
  explicit MapSaver(const rclcpp::NodeOptions & options);

  void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr map);

  std::shared_future<void> map_saved_future() {return save_next_map_promise.get_future().share();}

protected:
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::ConstSharedPtr map_sub_;

  void try_write_map_to_file(const nav_msgs::msg::OccupancyGrid & map);

  std::promise<void> save_next_map_promise;

  std::string image_format;
  std::string mapname_;
  int threshold_occupied_;
  int threshold_free_;
  nav2_map_server::MapMode map_mode;
};

}  // namespace nav2_map_server

#endif  // NAV2_MAP_SERVER__MAP_SAVER_HPP_
