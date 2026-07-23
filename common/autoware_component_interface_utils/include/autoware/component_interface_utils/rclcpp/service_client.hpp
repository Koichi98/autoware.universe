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
#include <rclcpp/node.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <utility>

namespace autoware::component_interface_utils
{

/// Create the underlying client handle. Uses rclcpp::QoS for the wrapper node (agnocast-native
/// create_client) and rmw_qos_profile_t for a plain rclcpp::Node (Humble create_client signature).
/// The returned handle type is node-native (rclcpp::Client or the agnocast-backed client).
template <class SpecT, class NodeT>
auto create_client_handle(NodeT * node, rclcpp::CallbackGroup::SharedPtr group)
{
  if constexpr (is_wrapper_node_v<NodeT>) {
    return node->template create_client<typename SpecT::Service>(
      SpecT::name, rclcpp::ServicesQoS(), group);
  } else {
    return node->template create_client<typename SpecT::Service>(
      SpecT::name, rmw_qos_profile_services_default, group);
  }
}

/// The wrapper class of a service client, providing a synchronous call() and a callback-based
/// async_send_request() over both the rclcpp client and the agnocast-backed client.
template <class SpecT, class NodeT = rclcpp::Node>
class Client
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(Client)
  using SpecType = SpecT;
  using Request = typename SpecT::Service::Request;
  using Response = typename SpecT::Service::Response;
  using SharedRequest = std::shared_ptr<Request>;
  using SharedResponse = std::shared_ptr<const Response>;
  using WrapSharedPtr = decltype(create_client_handle<SpecT>(
    std::declval<NodeT *>(), std::declval<rclcpp::CallbackGroup::SharedPtr>()));
  using ClientType = typename WrapSharedPtr::element_type;

  /// Constructor.
  Client(NodeT * node, rclcpp::CallbackGroup::SharedPtr group)
  {
    client_ = create_client_handle<SpecT>(node, group);
  }

  /// Send request and block until the response is ready (or the timeout elapses).
  SharedResponse call(const SharedRequest request, std::optional<double> timeout = std::nullopt)
  {
    if (!client_->service_is_ready()) {
      throw ServiceUnready(SpecT::name);
    }
    if constexpr (is_wrapper_node_v<NodeT>) {
      // Agnocast-native path: allocate the request from the client, copy in the fields, send.
      auto req = client_->allocate_output_service_request();
      *req = *request;
      auto future = client_->async_send_request(std::move(req)).future;
      if (timeout) {
        const auto duration = std::chrono::duration<double, std::ratio<1>>(timeout.value());
        if (future.wait_for(duration) != std::future_status::ready) {
          throw ServiceTimeout(SpecT::name);
        }
      }
      return std::make_shared<Response>(*future.get());
    } else {
      auto future = client_->async_send_request(request).future;
      if (timeout) {
        const auto duration = std::chrono::duration<double, std::ratio<1>>(timeout.value());
        if (future.wait_for(duration) != std::future_status::ready) {
          throw ServiceTimeout(SpecT::name);
        }
      }
      return future.get();
    }
  }

  /// Send request without blocking. The callback receives the backend-native shared future.
  template <class CallbackT>
  void async_send_request(const SharedRequest request, CallbackT && callback)
  {
    if constexpr (is_wrapper_node_v<NodeT>) {
      auto req = client_->allocate_output_service_request();
      *req = *request;
      client_->async_send_request(
        std::move(req), [callback](typename ClientType::SharedFuture future) { callback(future); });
    } else {
      client_->async_send_request(
        request, [callback](typename ClientType::SharedFuture future) { callback(future); });
    }
  }

  /// Check if the service is ready.
  bool service_is_ready() const { return client_->service_is_ready(); }

private:
  RCLCPP_DISABLE_COPY(Client)
  WrapSharedPtr client_;
};

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_CLIENT_HPP_
