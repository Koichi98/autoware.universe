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

#ifndef UTILS__TYPES_HPP_
#define UTILS__TYPES_HPP_

#include <autoware/agnocast_wrapper/node.hpp>
#include <autoware/agnocast_wrapper/polling_subscriber.hpp>
#include <autoware/component_interface_utils/rclcpp.hpp>

namespace autoware::default_adapi
{

// All AD API nodes run on autoware::agnocast_wrapper::Node.
using Node = autoware::agnocast_wrapper::Node;

template <class T>
using Pub = typename autoware::component_interface_utils::Publisher<T, Node>::SharedPtr;
template <class T>
using Sub = typename autoware::component_interface_utils::Subscription<T, Node>::SharedPtr;
// Polling subscriber (agnocast-native; take_data()). Spec T must provide ::Message.
template <class T>
using Poll =
  typename autoware::agnocast_wrapper::polling::PollingSubscriber<typename T::Message>::SharedPtr;
template <class T>
using Cli = typename autoware::component_interface_utils::Client<T, Node>::SharedPtr;
template <class T>
using Srv = typename autoware::component_interface_utils::Service<T, Node>::SharedPtr;

}  // namespace autoware::default_adapi

#endif  // UTILS__TYPES_HPP_
