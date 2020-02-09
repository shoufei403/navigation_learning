#ifndef NAV2_COSTMAP_2D__TESTING_HELPER_HPP_
#define NAV2_COSTMAP_2D__TESTING_HELPER_HPP_

#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"
#include "nav2_costmap_2d/cost_values.hpp"
#include "nav2_costmap_2d/costmap_2d.hpp"
#include "nav2_costmap_2d/static_layer.hpp"
#include "nav2_costmap_2d/obstacle_layer.hpp"
#include "nav2_costmap_2d/inflation_layer.hpp"
#include "nav2_util/lifecycle_node.hpp"

const double MAX_Z(1.0);

void setValues(nav2_costmap_2d::Costmap2D & costmap, const unsigned char * map)
{
  int index = 0;
  for (unsigned int i = 0; i < costmap.getSizeInCellsY(); i++) {
    for (unsigned int j = 0; j < costmap.getSizeInCellsX(); j++) {
      costmap.setCost(j, i, map[index]);
    }
  }
}

char printableCost(unsigned char cost)
{
  switch (cost) {
    case nav2_costmap_2d::NO_INFORMATION: return '?';
    case nav2_costmap_2d::LETHAL_OBSTACLE: return 'L';
    case nav2_costmap_2d::INSCRIBED_INFLATED_OBSTACLE: return 'I';
    case nav2_costmap_2d::FREE_SPACE: return '.';
    default: return '0' + (unsigned char) (10 * cost / 255);
  }
}

void printMap(nav2_costmap_2d::Costmap2D & costmap)
{
  printf("map:\n");
  for (unsigned int i = 0; i < costmap.getSizeInCellsY(); i++) {
    for (unsigned int j = 0; j < costmap.getSizeInCellsX(); j++) {
      printf("%4d", static_cast<int>(costmap.getCost(j, i)));
    }
    printf("\n\n");
  }
}

unsigned int countValues(
  nav2_costmap_2d::Costmap2D & costmap,
  unsigned char value, bool equal = true)
{
  unsigned int count = 0;
  for (unsigned int i = 0; i < costmap.getSizeInCellsY(); i++) {
    for (unsigned int j = 0; j < costmap.getSizeInCellsX(); j++) {
      unsigned char c = costmap.getCost(j, i);
      if ((equal && c == value) || (!equal && c != value)) {
        count += 1;
      }
    }
  }
  return count;
}

nav2_costmap_2d::StaticLayer * addStaticLayer(
  nav2_costmap_2d::LayeredCostmap & layers,
  tf2_ros::Buffer & tf, nav2_util::LifecycleNode::SharedPtr node)
{
  nav2_costmap_2d::StaticLayer * slayer = new nav2_costmap_2d::StaticLayer();
  layers.addPlugin(std::shared_ptr<nav2_costmap_2d::Layer>(slayer));
  slayer->initialize(&layers, "static", &tf, node, nullptr, nullptr /*TODO*/);
  return slayer;
}

nav2_costmap_2d::ObstacleLayer * addObstacleLayer(
  nav2_costmap_2d::LayeredCostmap & layers,
  tf2_ros::Buffer & tf, nav2_util::LifecycleNode::SharedPtr node)
{
  nav2_costmap_2d::ObstacleLayer * olayer = new nav2_costmap_2d::ObstacleLayer();
  olayer->initialize(&layers, "obstacles", &tf, node, nullptr, nullptr /*TODO*/);
  layers.addPlugin(std::shared_ptr<nav2_costmap_2d::Layer>(olayer));
  return olayer;
}

void addObservation(
  nav2_costmap_2d::ObstacleLayer * olayer, double x, double y, double z = 0.0,
  double ox = 0.0, double oy = 0.0, double oz = MAX_Z)
{
  sensor_msgs::msg::PointCloud2 cloud;
  sensor_msgs::PointCloud2Modifier modifier(cloud);
  modifier.setPointCloud2FieldsByString(1, "xyz");
  modifier.resize(1);
  sensor_msgs::PointCloud2Iterator<float> iter_x(cloud, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(cloud, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(cloud, "z");
  *iter_x = x;
  *iter_y = y;
  *iter_z = z;

  geometry_msgs::msg::Point p;
  p.x = ox;
  p.y = oy;
  p.z = oz;

  // obstacle range = raytrace range = 100.0
  nav2_costmap_2d::Observation obs(p, cloud, 100.0, 100.0);
  olayer->addStaticObservation(obs, true, true);
}

nav2_costmap_2d::InflationLayer * addInflationLayer(
  nav2_costmap_2d::LayeredCostmap & layers,
  tf2_ros::Buffer & tf, nav2_util::LifecycleNode::SharedPtr node)
{
  nav2_costmap_2d::InflationLayer * ilayer = new nav2_costmap_2d::InflationLayer();
  ilayer->initialize(&layers, "inflation", &tf, node, nullptr, nullptr /*TODO*/);
  std::shared_ptr<nav2_costmap_2d::Layer> ipointer(ilayer);
  layers.addPlugin(ipointer);
  return ilayer;
}


#endif  // NAV2_COSTMAP_2D__TESTING_HELPER_HPP_
