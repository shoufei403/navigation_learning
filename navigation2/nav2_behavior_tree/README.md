# nav2_behavior_tree

The nav2_behavior_tree module provides:
* A C++ template class for integrating ROS2 actions into Behavior Trees,
* Navigation-specific behavior tree nodes, and 
* a generic BehaviorTreeEngine class that simplifies the integration of BT processing into ROS2 nodes. 

This module is used by the nav2_bt_navigator to implement a ROS2 node that executes navigation Behavior Trees. The nav2_behavior_tree module uses the [Behavior-Tree.CPP library](https://github.com/BehaviorTree/BehaviorTree.CPP) for the core Behavior Tree processing. 

## The bt_action_node Template and the Behavior Tree Engine

The [bt_action_node template](include/nav2_behavior_tree/bt_action_node.hpp) allows one to easily integrate a ROS2 action into a BehaviorTree. To do so, one derives from the BTActionNode template, providing the action message type. For example,

```C++
#include "nav2_msgs/action/follow_path.hpp"
#include "nav2_behavior_tree/bt_action_node.hpp"

class FollowPathAction : public BtActionNode<nav2_msgs::action::FollowPath>
{
    ...
};
```

The resulting node must be registered with the factory in the Behavior Tree engine in order to be available for use in Behavior Trees executed by this engine.

```C++
BehaviorTreeEngine::BehaviorTreeEngine()
{
    ...

  factory_.registerNodeType<nav2_behavior_tree::FollowPathAction>("FollowPath");

    ...
}
```

Once a new node is registered with the factory, it is now available to the BehaviorTreeEngine and can be used in Behavior Trees. For example, the following simple XML description of a BT shows the FollowPath node in use:

```XML
<root main_tree_to_execute="MainTree">
  <BehaviorTree ID="MainTree">
    <Sequence name="root">
      <ComputePathToPose goal="${goal}"/>
      <FollowPath path="${path}"/>
    </Sequence>
  </BehaviorTree>
</root>
```
The BehaviorTree engine has a run method that accepts an XML description of a BT for execution:

```C++
  BtStatus run(
    BT::Blackboard::Ptr & blackboard,
    const std::string & behavior_tree_xml,
    std::function<void()> onLoop,
    std::function<bool()> cancelRequested,
    std::chrono::milliseconds loopTimeout = std::chrono::milliseconds(10));
```

See the code in the [BT Navigator](../nav2_bt_navigator/src/bt_navigator.cpp) for an example usage of the BehaviorTreeEngine.
    
## Navigation-Specific Behavior Tree Nodes

The nav2_behavior_tree package provides several navigation-specific nodes that are pre-registered and can be included in Behavior Trees.

| BT Node   |      Type      |  Description |
|----------|:-------------|------|
| Backup |  Action | Invokes the BackUp ROS2 action server, which causes the robot to back up to a specific pose. This is used in nav2 Behavior Trees as a recovery behavior. The nav2_recoveries module implements the BackUp action server. | 
| ComputePathToPose |    Action   | Invokes the ComputePathToPose ROS2 action server, which is implemented by the nav2_navfn_planner module. |
| FollowPath | Action |Invokes the FollowPath ROS2 action server, which is implemented by the nav2_dwb_controller module. |
| GoalReached | Condition | Checks the distance to the goal, if the distance to goal is less than the pre-defined threshold, the tree returns SUCCESS, otherwise it returns FAILURE. |
| IsStuck | Condition | Determines if the robot is not progressing towards the goal. If the robot is stuck and not progressing, the condition returns SUCCESS, otherwise it returns FAILURE. |
| NavigateToPose | Action | Invokes the NavigateToPose ROS2 action server, which is implemented by the bt_navigator module. |
| RateController | Decorator | A node that throttles the tick rate for its child. The tick rate can be supplied to the node as a parameter. The node returns RUNNING when it is not ticking its child. Currently, in the navigation stack, the `RateController` is used to adjust the rate at which the `ComputePathToPose` and `GoalReached` nodes are ticked. |
| RandomCrawl | Action | This BT action invokes the RandomCrawl ROS2 action server, which is implemented by the nav2_experimental/nav2_rl/nav2_turtlebot3_rl experimental module. The RandomCrawl action server will direct the robot to randomly navigate its environment without hitting any obstacles. |
| RecoveryNode | Control | The RecoveryNode is a control flow node with two children.  It returns SUCCESS if and only if the first child returns SUCCESS. The second child will be executed only if the first child returns FAILURE. If the second child SUCCEEDS, then the first child will be executed again. The user can specify how many times the recovery actions should be taken before returning FAILURE. In nav2, the RecoveryNode is included in Behavior Trees to implement recovery actions upon failures.
| Spin | Action | Invokes the Spin ROS2 action server, which is implemented by the nav2_recoveries module. This action is using in nav2 Behavior Trees as a recovery behavior. |

For more information about the behavior tree nodes that are available in the default BehaviorTreeCPP library, see documentation here: https://www.behaviortree.dev/bt_basics/
