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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SUBSCRIPTION_CALLBACK_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SUBSCRIPTION_CALLBACK_HPP_

// Backend-specific glue for subscriptions. The spec callbacks throughout Autoware are written
// against Message::ConstSharedPtr; this header keeps that calling convention identical in both
// builds while bridging to whatever the underlying node's create_subscription expects.

#include <autoware/component_interface_utils/node_type.hpp>
#include <autoware/component_interface_utils/specs.hpp>

#include <memory>
#include <type_traits>
#include <utility>

#ifdef USE_AGNOCAST_ENABLED

#include <autoware/agnocast_wrapper/autoware_agnocast_wrapper.hpp>

#include <rclcpp/rclcpp.hpp>

namespace autoware::component_interface_utils
{

/// Adapt a Message::ConstSharedPtr callback to the agnocast message_ptr callback that
/// agnocast_wrapper::Node::create_subscription requires.
///
/// The agnocast subscription delivers an AUTOWARE_MESSAGE_CONST_SHARED_PTR (a zero-copy view into
/// shared memory). The spec callbacks take a std::shared_ptr<const Message>; bridging the two
/// requires one copy out of shared memory. These ADAPI topics are low rate, so the copy is
/// acceptable and keeps every node callback signature unchanged.
template <class MessageT, class CallbackT>
auto wrap_subscription_callback(CallbackT && callback)
{
  return [callback = std::forward<CallbackT>(callback)](
           AUTOWARE_MESSAGE_CONST_SHARED_PTR(MessageT) && msg) {
    callback(std::make_shared<const MessageT>(*msg));
  };
}

/// Create a polling subscription (callback == nullptr case). Uses the wrapper's PollingSubscriber
/// so it keeps working under an AgnocastOnly executor, which would not spin a plain rclcpp
/// subscription.
template <class SpecT, class NodeT>
auto make_polling_subscription(NodeT * node)
{
  return node->template create_polling_subscriber<typename SpecT::Message>(
    SpecT::name, get_qos<SpecT>());
}

// Detects the polling subscriber's take_data() so the take() proxy compiles even when the held
// subscription is a callback subscription (which has no take_data()); in that case it is never
// called and just returns nullptr.
template <class T, class = void>
struct has_take_data : std::false_type
{
};
template <class T>
struct has_take_data<T, std::void_t<decltype(std::declval<T>()->take_data())>> : std::true_type
{
};

/// Drain the polling subscriber to its newest message, mirroring the rclcpp Subscription::take()
/// proxy. Returns nullptr when no message is available (or for a callback subscription).
template <class SpecT, class SubPtrT>
typename SpecT::Message::ConstSharedPtr take_polling_message(const SubPtrT & subscription)
{
  if constexpr (has_take_data<SubPtrT>::value) {
    auto data = subscription->take_data();
    if (!data) {
      return nullptr;
    }
    return std::make_shared<const typename SpecT::Message>(*data);
  } else {
    (void)subscription;
    return nullptr;
  }
}

}  // namespace autoware::component_interface_utils

#else

#include <rclcpp/rclcpp.hpp>

namespace autoware::component_interface_utils
{

/// rclcpp build: the spec callback already has the right signature, forward it untouched.
template <class MessageT, class CallbackT>
auto wrap_subscription_callback(CallbackT && callback)
{
  return std::forward<CallbackT>(callback);
}

/// rclcpp build: create a plain subscription with a no-op callback on its own callback group, as
/// the original implementation did, so it is only drained via take().
template <class SpecT, class NodeT>
auto make_polling_subscription(NodeT * node)
{
  auto group = node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
  auto options = rclcpp::SubscriptionOptions();
  options.callback_group = group;
  return node->template create_subscription<typename SpecT::Message>(
    SpecT::name, get_qos<SpecT>(), [](const typename SpecT::Message) {}, options);
}

/// Drain a plain rclcpp subscription to its newest message via take(), mirroring the original
/// Subscription::take() loop. Returns nullptr when no message is available.
template <class SpecT, class SubPtrT>
typename SpecT::Message::ConstSharedPtr take_polling_message(const SubPtrT & subscription)
{
  rclcpp::MessageInfo info;
  auto data = std::make_shared<typename SpecT::Message>();
  bool flag = false;
  for (size_t i = 0; i < subscription->get_actual_qos().depth(); ++i) {
    if (!subscription->take(*data, info)) {
      break;
    }
    flag = true;  // Whether there is at least one data.
  }
  return flag ? data : nullptr;
}

}  // namespace autoware::component_interface_utils

#endif

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SUBSCRIPTION_CALLBACK_HPP_
