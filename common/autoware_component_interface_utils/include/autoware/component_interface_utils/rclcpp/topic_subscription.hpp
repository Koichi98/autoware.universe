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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__TOPIC_SUBSCRIPTION_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__TOPIC_SUBSCRIPTION_HPP_

#include <rclcpp/qos.hpp>
#include <rclcpp/subscription.hpp>

#include <functional>
#include <memory>
#include <string>

namespace autoware::component_interface_utils
{

/// The wrapper class of a subscription. The underlying handle type is deduced from the node's
/// create_subscription(), so it is rclcpp::Subscription for a plain rclcpp::Node and the
/// agnocast-backed subscription for autoware::agnocast_wrapper::Node under ENABLE_AGNOCAST=1.
///
/// NOTE: take()/take_and_update() are only valid for the rclcpp handle. They are member templates
/// instantiated on use, so agnocast-backed nodes that need polling should use
/// autoware_utils_rclcpp::InterProcessPollingSubscriber instead (the wrapper subscription has no
/// take()).
template <class SpecT, class NodeT = rclcpp::Node>
class Subscription
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(Subscription)
  using SpecType = SpecT;
  using WrapType = std::remove_reference_t<
    decltype(*std::declval<NodeT>().template create_subscription<typename SpecT::Message>(
      std::declval<const std::string &>(), std::declval<const rclcpp::QoS &>(),
      std::declval<std::function<void(const typename SpecT::Message &)>>()))>;
  using WrapSharedPtr = typename WrapType::SharedPtr;

  /// Constructor.
  explicit Subscription(WrapSharedPtr subscription)
  {
    subscription_ = subscription;  // to keep the reference count
  }

  typename SpecType::Message::ConstSharedPtr take()
  {
    rclcpp::MessageInfo info;
    auto data = std::make_shared<typename SpecType::Message>();
    bool flag = false;
    for (size_t i = 0; i < subscription_->get_actual_qos().depth(); ++i) {
      if (!subscription_->take(*data, info)) {
        break;
      }
      flag = true;  // Whether there is at least one data.
    }
    return flag ? data : nullptr;
  }

  bool take_and_update(typename SpecType::Message::ConstSharedPtr & ptr)
  {
    const auto msg = take();
    if (!msg) {
      return false;
    }
    ptr = msg;
    return true;
  }

  bool take_and_update(typename SpecType::Message & ref)
  {
    const auto msg = take();
    if (!msg) {
      return false;
    }
    ref = *msg;
    return true;
  }

private:
  RCLCPP_DISABLE_COPY(Subscription)
  WrapSharedPtr subscription_;
};

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__TOPIC_SUBSCRIPTION_HPP_
