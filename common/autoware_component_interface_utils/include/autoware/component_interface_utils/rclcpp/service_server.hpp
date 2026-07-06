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
#include <autoware/component_interface_utils/rclcpp/service_detail.hpp>

#include <rclcpp/node.hpp>

#include <tier4_system_msgs/msg/service_log.hpp>

#include <memory>
#include <utility>

namespace autoware::component_interface_utils
{

/// The wrapper class of the underlying service server for logging.
///
/// The node-facing callback takes std::shared_ptr<Request|Response>, identical in both builds.
/// service_detail::ServiceBackend handles the actual service creation and, under
/// USE_AGNOCAST_ENABLED, the message_ptr bridging.
template <class SpecT>
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

public:
  RCLCPP_SMART_PTR_DEFINITIONS(Service)
  using SpecType = SpecT;
  using ServiceLog = tier4_system_msgs::msg::ServiceLog;

  using SharedRequest = service_detail::SharedRequest<SpecT>;
  using SharedResponse = service_detail::SharedResponse<SpecT>;

  /// Constructor.
  template <class CallbackT>
  Service(
    NodeInterface::SharedPtr interface, CallbackT && callback,
    rclcpp::CallbackGroup::SharedPtr group)
  : interface_(interface),
    backend_(interface->node, SpecT::name, wrap(std::forward<CallbackT>(callback)), group)
  {
  }

  /// Create a service callback with logging added.
  template <class CallbackT>
  service_detail::ServerCallback<SpecT> wrap(CallbackT && callback)
  {
    auto wrapped = [this, callback](SharedRequest request, SharedResponse response) {
      // If the response has status, convert it from the exception.
      interface_->log(ServiceLog::SERVER_REQUEST, SpecType::name, to_yaml(*request));
      if constexpr (!has_status_type<typename SpecT::Service::Response>::value) {
        callback(request, response);
      } else {
        try {
          callback(request, response);
        } catch (const ServiceException & error) {
          error.set(response->status);
        }
      }
      interface_->log(ServiceLog::SERVER_RESPONSE, SpecType::name, to_yaml(*response));
    };
    return wrapped;
  }

private:
  RCLCPP_DISABLE_COPY(Service)
  NodeInterface::SharedPtr interface_;
  service_detail::ServiceBackend<SpecT> backend_;
};

}  // namespace autoware::component_interface_utils

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_SERVER_HPP_
