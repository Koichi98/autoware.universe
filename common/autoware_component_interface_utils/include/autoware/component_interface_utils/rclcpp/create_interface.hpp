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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__CREATE_INTERFACE_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__CREATE_INTERFACE_HPP_

#include <autoware/component_interface_utils/rclcpp/interface.hpp>
#include <autoware/component_interface_utils/rclcpp/service_client.hpp>
#include <autoware/component_interface_utils/rclcpp/service_server.hpp>
#include <autoware/component_interface_utils/rclcpp/topic_publisher.hpp>
#include <autoware/component_interface_utils/rclcpp/topic_subscription.hpp>
#include <autoware/component_interface_utils/specs.hpp>
#include <rclcpp/rclcpp.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace autoware::component_interface_utils
{

/// Create a client wrapper. This is a private implementation.
template <class SpecT, class NodeT>
typename Client<SpecT, NodeT>::SharedPtr create_client_impl(
  std::shared_ptr<NodeInterface<NodeT>> interface, rclcpp::CallbackGroup::SharedPtr group = nullptr)
{
  return Client<SpecT, NodeT>::make_shared(interface->node, group);
}

/// Create a service wrapper. This is a private implementation.
template <class SpecT, class NodeT, class CallbackT>
typename Service<SpecT, NodeT>::SharedPtr create_service_impl(
  std::shared_ptr<NodeInterface<NodeT>> interface, CallbackT && callback,
  rclcpp::CallbackGroup::SharedPtr group = nullptr)
{
  return Service<SpecT, NodeT>::make_shared(
    interface->node, std::forward<CallbackT>(callback), group);
}

/// Create a publisher using traits like services. This is a private implementation.
template <class SpecT, class NodeT>
typename Publisher<SpecT, NodeT>::SharedPtr create_publisher_impl(NodeT * node)
{
  auto publisher =
    node->template create_publisher<typename SpecT::Message>(SpecT::name, get_qos<SpecT>());
  return Publisher<SpecT, NodeT>::make_shared(publisher);
}

/// Create a subscription using traits like services. This is a private implementation.
template <class SpecT, class NodeT, class CallbackT>
typename Subscription<SpecT, NodeT>::SharedPtr create_subscription_impl(
  NodeT * node, CallbackT && callback)
{
  if constexpr (std::is_null_pointer_v<std::decay_t<CallbackT>>) {
    // If the callback is nullptr, create a subscription for polling (rclcpp take path).
    auto group = node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
    auto options = rclcpp::SubscriptionOptions();
    options.callback_group = group;

    auto subscription = node->template create_subscription<typename SpecT::Message>(
      SpecT::name, get_qos<SpecT>(), [](const typename SpecT::Message &) {}, options);
    return Subscription<SpecT, NodeT>::make_shared(subscription);
  } else if constexpr (std::is_invocable_v<CallbackT, const typename SpecT::Message &>) {
    // Callback takes const-ref (or by value): agnocast-compatible, pass through directly.
    auto subscription = node->template create_subscription<typename SpecT::Message>(
      SpecT::name, get_qos<SpecT>(), std::forward<CallbackT>(callback));
    return Subscription<SpecT, NodeT>::make_shared(subscription);
  } else {
    // Callback takes Message::ConstSharedPtr: adapt to a const-ref callback (one copy) so it is
    // valid for both rclcpp and the agnocast-backed subscription.
    auto cb = std::forward<CallbackT>(callback);
    auto adapted = [cb](const typename SpecT::Message & msg) {
      cb(std::make_shared<typename SpecT::Message>(msg));
    };
    auto subscription = node->template create_subscription<typename SpecT::Message>(
      SpecT::name, get_qos<SpecT>(), adapted);
    return Subscription<SpecT, NodeT>::make_shared(subscription);
  }
}

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__CREATE_INTERFACE_HPP_
