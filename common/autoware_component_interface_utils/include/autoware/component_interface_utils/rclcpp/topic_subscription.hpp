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

#include <autoware/component_interface_utils/rclcpp/subscription_callback.hpp>

#include <rclcpp/subscription.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace autoware::component_interface_utils
{

/// The wrapper class of the underlying subscription. This is for future use and no functionality
/// now.
///
/// Holds whatever create_subscription_impl produced: a callback subscription or, for the polling
/// (nullptr-callback) case, a polling subscriber. take() / take_and_update() are only meaningful
/// for the polling case and are routed through take_polling_message(), which is specialized per
/// build (rclcpp take()-loop vs agnocast PollingSubscriber::take_data()).
template <class SpecT>
class Subscription
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(Subscription)
  using SpecType = SpecT;

  /// Constructor. Accepts any underlying subscription pointer (callback or polling).
  template <class SubPtrT>
  explicit Subscription(SubPtrT subscription)
  : take_impl_([subscription]() { return take_polling_message<SpecT>(subscription); }),
    keep_alive_(std::move(subscription))  // to keep the reference count
  {
  }

  typename SpecType::Message::ConstSharedPtr take() { return take_impl_(); }

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
  std::function<typename SpecType::Message::ConstSharedPtr()> take_impl_;
  std::shared_ptr<void> keep_alive_;
};

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__TOPIC_SUBSCRIPTION_HPP_
