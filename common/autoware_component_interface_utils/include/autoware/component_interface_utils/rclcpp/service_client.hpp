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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_CLIENT_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_CLIENT_HPP_

#include <autoware/component_interface_utils/rclcpp/exceptions.hpp>
#include <autoware/component_interface_utils/rclcpp/interface.hpp>
#include <autoware/component_interface_utils/rclcpp/service_detail.hpp>

#include <rclcpp/node.hpp>

#include <tier4_system_msgs/msg/service_log.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace autoware::component_interface_utils
{

/// The wrapper class of the underlying service client for logging.
///
/// The node-facing API (call / async_send_request / service_is_ready) is expressed in plain
/// std::shared_ptr<Request|Response>, identical in both builds, so node code is unchanged. Under
/// USE_AGNOCAST_ENABLED the request/response are bridged to/from the agnocast message_ptr API by
/// service_detail::ClientBackend.
template <class SpecT>
class Client
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(Client)
  using SpecType = SpecT;
  using ServiceLog = tier4_system_msgs::msg::ServiceLog;

  using SharedRequest = std::shared_ptr<typename SpecT::Service::Request>;
  using SharedResponse = std::shared_ptr<typename SpecT::Service::Response>;
  using SharedFuture = std::shared_future<SharedResponse>;

  /// Constructor.
  Client(NodeInterface::SharedPtr interface, rclcpp::CallbackGroup::SharedPtr group)
  : interface_(interface), backend_(interface->node, SpecT::name, group)
  {
  }

  /// Send request.
  SharedResponse call(const SharedRequest request, std::optional<double> timeout = std::nullopt)
  {
    if (!backend_.service_is_ready()) {
      interface_->log(ServiceLog::ERROR_UNREADY, SpecType::name);
      throw ServiceUnready(SpecT::name);
    }

    const auto future = this->async_send_request(request);
    if (timeout) {
      const auto duration = std::chrono::duration<double, std::ratio<1>>(timeout.value());
      if (future.wait_for(duration) != std::future_status::ready) {
        interface_->log(ServiceLog::ERROR_TIMEOUT, SpecType::name);
        throw ServiceTimeout(SpecT::name);
      }
    }
    return future.get();
  }

  /// Send request.
  SharedFuture async_send_request(SharedRequest request)
  {
    return this->async_send_request(request, [](SharedFuture) {});
  }

  /// Send request.
  template <class CallbackT>
  SharedFuture async_send_request(SharedRequest request, CallbackT && callback)
  {
    const auto wrapped = [this, callback](SharedFuture future) {
      interface_->log(ServiceLog::CLIENT_RESPONSE, SpecType::name, to_yaml(*future.get()));
      callback(future);
    };

    interface_->log(ServiceLog::CLIENT_REQUEST, SpecType::name, to_yaml(*request));
    return backend_.async_send_request(request, wrapped);
  }

  /// Check if the service is ready.
  bool service_is_ready() const { return backend_.service_is_ready(); }

private:
  RCLCPP_DISABLE_COPY(Client)
  NodeInterface::SharedPtr interface_;
  service_detail::ClientBackend<SpecT> backend_;
};

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_CLIENT_HPP_
