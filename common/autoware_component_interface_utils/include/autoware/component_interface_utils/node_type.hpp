// Copyright 2025 TIER IV, Inc.
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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__NODE_TYPE_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__NODE_TYPE_HPP_

// Single place that decides which node backend the interface proxies are built against.
//
// A consumer that links autoware_agnocast_wrapper inherits the PUBLIC compile definition
// USE_AGNOCAST_ENABLED, which flips this whole package from the plain rclcpp::Node backend to the
// agnocast_wrapper::Node backend. Everything else in this package is written against the aliases
// declared here, so no other header needs an #ifdef.
//
// The chosen node type must expose: create_publisher / create_subscription / create_service /
// create_client / create_callback_group / now / get_logger / get_name / get_namespace — both
// rclcpp::Node and autoware::agnocast_wrapper::Node satisfy this.

#ifdef USE_AGNOCAST_ENABLED

#include <autoware/agnocast_wrapper/node.hpp>

#include <tier4_system_msgs/msg/service_log.hpp>

namespace autoware::component_interface_utils
{
using NodeType = autoware::agnocast_wrapper::Node;
using ServiceLogPublisher =
  autoware::agnocast_wrapper::Publisher<tier4_system_msgs::msg::ServiceLog>::SharedPtr;

template <class MessageT>
using PublisherPtr = typename autoware::agnocast_wrapper::Publisher<MessageT>::SharedPtr;
template <class MessageT>
using SubscriptionPtr = typename autoware::agnocast_wrapper::Subscription<MessageT>::SharedPtr;
template <class MessageT>
using PollingSubscriberPtr =
  typename autoware::agnocast_wrapper::PollingSubscriber<MessageT>::SharedPtr;
}  // namespace autoware::component_interface_utils

#else

#include <rclcpp/rclcpp.hpp>

#include <tier4_system_msgs/msg/service_log.hpp>

namespace autoware::component_interface_utils
{
using NodeType = rclcpp::Node;
using ServiceLogPublisher = rclcpp::Publisher<tier4_system_msgs::msg::ServiceLog>::SharedPtr;

template <class MessageT>
using PublisherPtr = typename rclcpp::Publisher<MessageT>::SharedPtr;
template <class MessageT>
using SubscriptionPtr = typename rclcpp::Subscription<MessageT>::SharedPtr;
template <class MessageT>
using PollingSubscriberPtr = typename rclcpp::Subscription<MessageT>::SharedPtr;
}  // namespace autoware::component_interface_utils

#endif

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__NODE_TYPE_HPP_
