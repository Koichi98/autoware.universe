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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__TOPIC_PUBLISHER_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__TOPIC_PUBLISHER_HPP_

#include <rclcpp/publisher.hpp>
#include <rclcpp/qos.hpp>

#include <string>
#include <utility>

namespace autoware::component_interface_utils
{

/// The wrapper class of a publisher. The underlying handle type is deduced from the node's
/// create_publisher(), so it is rclcpp::Publisher for a plain rclcpp::Node and the agnocast-backed
/// publisher for autoware::agnocast_wrapper::Node under ENABLE_AGNOCAST=1.
template <class SpecT, class NodeT = rclcpp::Node>
class Publisher
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(Publisher)
  using SpecType = SpecT;
  using WrapType = std::remove_reference_t<
    decltype(*std::declval<NodeT>().template create_publisher<typename SpecT::Message>(
      std::declval<const std::string &>(), std::declval<const rclcpp::QoS &>()))>;
  using WrapSharedPtr = typename WrapType::SharedPtr;

  /// Constructor.
  explicit Publisher(WrapSharedPtr publisher)
  {
    publisher_ = publisher;  // to keep the reference count
  }

  /// Publish a message (copy overload; available on both rclcpp and agnocast-backed publishers).
  void publish(const typename SpecT::Message & msg) { publisher_->publish(msg); }

private:
  RCLCPP_DISABLE_COPY(Publisher)
  WrapSharedPtr publisher_;
};

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__TOPIC_PUBLISHER_HPP_
