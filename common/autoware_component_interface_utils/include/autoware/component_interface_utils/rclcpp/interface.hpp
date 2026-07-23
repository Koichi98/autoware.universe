// Copyright 2022 TIER IV, Inc.
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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__INTERFACE_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__INTERFACE_HPP_

// Provides autoware::agnocast_wrapper::Node and the AUTOWARE_* macros. Under ENABLE_AGNOCAST=0 the
// macros resolve to plain rclcpp/std types, so consumers that pass a plain rclcpp::Node are
// unaffected; under =1 they resolve to the agnocast-backed wrapper types.
#include <autoware/agnocast_wrapper/node.hpp>
#include <rclcpp/rclcpp.hpp>

#include <memory>
#include <type_traits>

namespace autoware::component_interface_utils
{

/// Trait that is true when NodeT exposes get_rclcpp_node(), i.e. it is an
/// autoware::agnocast_wrapper::Node rather than a plain rclcpp::Node. Used to pick the
/// agnocast-native create path (rclcpp::QoS + wrapper handle types) vs the plain rclcpp path
/// (rmw_qos_profile_t + rclcpp handle types), and to keep working for both node types.
template <class NodeT, class = void>
struct is_wrapper_node : std::false_type
{
};
template <class NodeT>
struct is_wrapper_node<NodeT, std::void_t<decltype(std::declval<NodeT>().get_rclcpp_node())>>
: std::true_type
{
};
template <class NodeT>
inline constexpr bool is_wrapper_node_v = is_wrapper_node<NodeT>::value;

/// Minimal holder for the node pointer used to create publishers, subscriptions, services and
/// clients. Templated on the node type so both rclcpp::Node and autoware::agnocast_wrapper::Node
/// can be adapted; the created handle types are deduced from the node's create_* methods.
template <class NodeT = rclcpp::Node>
struct NodeInterface
{
  using SharedPtr = std::shared_ptr<NodeInterface>;

  explicit NodeInterface(NodeT * node) { this->node = node; }

  NodeT * node;
};

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__INTERFACE_HPP_
