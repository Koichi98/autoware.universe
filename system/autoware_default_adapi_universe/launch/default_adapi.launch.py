# Copyright 2022 TIER IV, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import pathlib

import launch
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterFile
from launch_ros.substitutions import FindPackageShare


def create_api_node(package_name, node_name, executable):
    """Launch a node that derives from autoware::agnocast_wrapper::Node.

    Those nodes need an AgnocastOnly executor when ENABLE_AGNOCAST=1, which a shared component
    container cannot provide, so each runs as its own process instead of being composed. Every
    executable keeps a multi-threaded executor, matching the component_container_mt they used to
    share: some of them call a client from inside a service callback and would deadlock on a
    single thread.
    """
    fullname = pathlib.Path("adapi/node") / node_name
    return Node(
        namespace=str(fullname.parent),
        name=str(fullname.name),
        package=package_name,
        executable=executable,
        parameters=[ParameterFile(LaunchConfiguration("config"))],
    )


def get_default_config():
    path = FindPackageShare("autoware_default_adapi_universe")
    path = PathJoinSubstitution([path, "config/default_adapi.param.yaml"])
    return path


def generate_launch_description():
    core = "autoware_default_adapi"
    universe = "autoware_default_adapi_universe"
    nodes = [
        create_api_node(core, "interface", "interface_node"),
        create_api_node(core, "localization", "localization_node"),
        create_api_node(core, "routing", "routing_node"),
        create_api_node(universe, "autoware_state", "autoware_state_node"),
        create_api_node(universe, "diagnostics", "diagnostics_node"),
        create_api_node(universe, "fail_safe", "fail_safe_node"),
        create_api_node(universe, "heartbeat", "heartbeat_node"),
        create_api_node(universe, "manual/local", "manual_control_node"),
        create_api_node(universe, "manual/remote", "manual_control_node"),
        create_api_node(universe, "motion", "motion_node"),
        create_api_node(universe, "mrm_request", "mrm_request_node"),
        create_api_node(universe, "operation_mode", "operation_mode_node"),
        create_api_node(universe, "perception", "perception_node"),
        create_api_node(universe, "planning", "planning_node"),
        create_api_node(universe, "vehicle_status", "vehicle_status_node"),
        create_api_node(universe, "vehicle_command", "vehicle_command_node"),
        create_api_node(universe, "vehicle_metrics", "vehicle_metrics_node"),
        create_api_node(universe, "vehicle_info", "vehicle_info_node"),
        create_api_node(universe, "vehicle_door", "vehicle_door_node"),
    ]
    argument = DeclareLaunchArgument("config", default_value=get_default_config())
    return launch.LaunchDescription([argument, *nodes])
