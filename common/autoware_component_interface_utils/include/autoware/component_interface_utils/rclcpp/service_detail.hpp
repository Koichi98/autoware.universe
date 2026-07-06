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

#ifndef AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_DETAIL_HPP_
#define AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_DETAIL_HPP_

// Backend glue for service clients/servers. Isolates every difference between the rclcpp and the
// agnocast_wrapper service APIs so that service_client.hpp / service_server.hpp stay backend
// agnostic. The node-facing request/response types are always std::shared_ptr<Request|Response>.

#include <autoware/component_interface_utils/node_type.hpp>
#include <autoware/component_interface_utils/specs.hpp>

#include <rclcpp/rclcpp.hpp>

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>

namespace autoware::component_interface_utils::service_detail
{

template <class SpecT>
using SharedRequest = std::shared_ptr<typename SpecT::Service::Request>;
template <class SpecT>
using SharedResponse = std::shared_ptr<typename SpecT::Service::Response>;
template <class SpecT>
using SharedFuture = std::shared_future<SharedResponse<SpecT>>;
template <class SpecT>
using ServerCallback =
  std::function<void(SharedRequest<SpecT> request, SharedResponse<SpecT> response)>;

}  // namespace autoware::component_interface_utils::service_detail

#ifdef USE_AGNOCAST_ENABLED

#include <autoware/agnocast_wrapper/autoware_agnocast_wrapper.hpp>

namespace autoware::component_interface_utils::service_detail
{

/// Client backend over agnocast_wrapper::Node. Bridges the std::shared_ptr request/response that
/// node code uses to the agnocast message_ptr API. One copy in (request) and one copy out
/// (response) per call; acceptable for the low-rate ADAPI services and keeps node code unchanged.
template <class SpecT>
class ClientBackend
{
  // Alias so the AUTOWARE_*_PTR macros (which prepend `typename ServiceT::`) receive a bare type;
  // passing `typename SpecT::Service` directly would yield an ill-formed double `typename`.
  using ServiceT = typename SpecT::Service;
  using ClientRequestPtr = AUTOWARE_CLIENT_REQUEST_PTR(ServiceT);
  using ClientSharedFuture = AUTOWARE_CLIENT_SHARED_FUTURE(ServiceT);

public:
  ClientBackend(NodeType * node, const std::string & name, rclcpp::CallbackGroup::SharedPtr group)
  : client_(node->template create_client<ServiceT>(name, rclcpp::ServicesQoS(), group))
  {
  }

  bool service_is_ready() const { return client_->service_is_ready(); }

  template <class WrappedCallbackT>
  SharedFuture<SpecT> async_send_request(SharedRequest<SpecT> request, WrappedCallbackT && wrapped)
  {
    // The wrapper client takes ownership of a message_ptr request; copy the node's shared_ptr into
    // one. On the way back, re-wrap the message_ptr<const Response> as a std::shared_ptr so the
    // logging wrapper and node callback see the familiar SharedResponse.
    ClientRequestPtr loaned{std::make_shared<typename ServiceT::Request>(*request)};

    auto promise = std::make_shared<std::promise<SharedResponse<SpecT>>>();
    SharedFuture<SpecT> future = promise->get_future().share();
    client_->async_send_request(
      std::move(loaned),
      [wrapped = std::forward<WrappedCallbackT>(wrapped), promise](ClientSharedFuture agnocast_future) {
        promise->set_value(std::make_shared<typename ServiceT::Response>(*agnocast_future.get()));
        wrapped(promise->get_future().share());
      });
    return future;
  }

private:
  typename autoware::agnocast_wrapper::Client<ServiceT>::SharedPtr client_;
};

/// Service backend over agnocast_wrapper::Node. The wrapper's create_service accepts the
/// std::shared_ptr request/response callback form directly, so the node callback is passed through
/// unchanged (the wrapper does the message_ptr bridging internally).
template <class SpecT>
class ServiceBackend
{
  using ServiceT = typename SpecT::Service;

public:
  ServiceBackend(
    NodeType * node, const std::string & name, ServerCallback<SpecT> callback,
    rclcpp::CallbackGroup::SharedPtr group)
  {
    service_ = node->template create_service<ServiceT>(
      name, std::move(callback), rclcpp::ServicesQoS(), group);
  }

private:
  typename autoware::agnocast_wrapper::Service<ServiceT>::SharedPtr service_;
};

}  // namespace autoware::component_interface_utils::service_detail

#else

namespace autoware::component_interface_utils::service_detail
{

/// Client backend over rclcpp::Node. Thin pass-through to rclcpp::Client.
template <class SpecT>
class ClientBackend
{
public:
  ClientBackend(NodeType * node, const std::string & name, rclcpp::CallbackGroup::SharedPtr group)
  : client_(node->template create_client<typename SpecT::Service>(
      name, rmw_qos_profile_services_default, group))
  {
  }

  bool service_is_ready() const { return client_->service_is_ready(); }

  template <class WrappedCallbackT>
  SharedFuture<SpecT> async_send_request(SharedRequest<SpecT> request, WrappedCallbackT && wrapped)
  {
    return client_->async_send_request(request, std::forward<WrappedCallbackT>(wrapped)).future;
  }

private:
  typename rclcpp::Client<typename SpecT::Service>::SharedPtr client_;
};

/// Service backend over rclcpp::Node. Thin pass-through to rclcpp::Service.
template <class SpecT>
class ServiceBackend
{
public:
  ServiceBackend(
    NodeType * node, const std::string & name, ServerCallback<SpecT> callback,
    rclcpp::CallbackGroup::SharedPtr group)
  {
    service_ = node->template create_service<typename SpecT::Service>(
      name, std::move(callback), rmw_qos_profile_services_default, group);
  }

private:
  typename rclcpp::Service<typename SpecT::Service>::SharedPtr service_;
};

}  // namespace autoware::component_interface_utils::service_detail

#endif

#endif  // AUTOWARE__COMPONENT_INTERFACE_UTILS__RCLCPP__SERVICE_DETAIL_HPP_
