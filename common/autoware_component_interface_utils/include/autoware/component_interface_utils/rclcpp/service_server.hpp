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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_SERVER_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_SERVER_HPP_

#include <autoware/component_interface_utils/rclcpp/exceptions.hpp>
#include <autoware/component_interface_utils/rclcpp/interface.hpp>
#include <rclcpp/node.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace autoware::component_interface_utils
{

/// Create the underlying service handle from a wrapped rclcpp-style callback
/// (void(Request::SharedPtr, Response::SharedPtr)). For the wrapper node the callback is adapted to
/// the agnocast-native message_ptr service callback; for a plain rclcpp::Node it is used directly
/// with the Humble create_service signature. The returned handle type is node-native.
template <class SpecT, class NodeT, class WrappedT>
auto create_service_handle(NodeT * node, WrappedT wrapped, rclcpp::CallbackGroup::SharedPtr group)
{
  using Svc = typename SpecT::Service;
  if constexpr (is_wrapper_node_v<NodeT>) {
    return node->template create_service<Svc>(
      SpecT::name,
      [wrapped](
        AUTOWARE_SERVER_REQUEST_PTR(Svc) && req_msg, AUTOWARE_SERVER_RESPONSE_PTR(Svc) && res_msg) {
        auto request = std::make_shared<typename Svc::Request>(*req_msg);
        auto response = std::make_shared<typename Svc::Response>();
        wrapped(request, response);
        *res_msg = *response;
      },
      rclcpp::ServicesQoS(), group);
  } else {
    return node->template create_service<Svc>(
      SpecT::name, wrapped, rmw_qos_profile_services_default, group);
  }
}

/// The wrapper class of a service server. Converts a service exception thrown by the user callback
/// into the response status (when the response has one).
template <class SpecT, class NodeT = rclcpp::Node>
class Service
{
private:
  // Detect if the service response has status.
  template <class, template <class> class, class = std::void_t<>>
  struct detect : std::false_type
  {
  };
  template <class T, template <class> class Check>
  struct detect<T, Check, std::void_t<Check<T>>> : std::true_type
  {
  };
  template <class T>
  using has_status_impl = decltype(std::declval<T>().status);
  template <class T>
  using has_status_type = detect<T, has_status_impl>;

  using Request = typename SpecT::Service::Request;
  using Response = typename SpecT::Service::Response;
  using WrappedFn = std::function<void(std::shared_ptr<Request>, std::shared_ptr<Response>)>;

public:
  RCLCPP_SMART_PTR_DEFINITIONS(Service)
  using SpecType = SpecT;
  using WrapSharedPtr = decltype(create_service_handle<SpecT>(
    std::declval<NodeT *>(), std::declval<WrappedFn>(),
    std::declval<rclcpp::CallbackGroup::SharedPtr>()));

  /// Constructor.
  template <class CallbackT>
  Service(NodeT * node, CallbackT && callback, rclcpp::CallbackGroup::SharedPtr group)
  {
    service_ = create_service_handle<SpecT>(node, wrap(std::forward<CallbackT>(callback)), group);
  }

  /// Create a service callback that converts exceptions to the response status.
  template <class CallbackT>
  WrappedFn wrap(CallbackT && callback)
  {
    return [callback](std::shared_ptr<Request> request, std::shared_ptr<Response> response) {
      // If the response has status, convert it from the exception.
      if constexpr (!has_status_type<Response>::value) {
        callback(request, response);
      } else {
        try {
          callback(request, response);
        } catch (const ServiceException & error) {
          error.set(response->status);
        }
      }
    };
  }

private:
  RCLCPP_DISABLE_COPY(Service)
  WrapSharedPtr service_;
};

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_SERVER_HPP_
