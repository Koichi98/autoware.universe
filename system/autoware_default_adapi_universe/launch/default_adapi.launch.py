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
    # Each AD API node is now a standalone executable (registered via
    # autoware_agnocast_wrapper_register_node) so it can switch between rclcpp and agnocast
    # backends at runtime based on the ENABLE_AGNOCAST environment variable.
    fullname = pathlib.Path("adapi/node") / node_name
    return Node(
        namespace=str(fullname.parent),
        name=str(fullname.name),
        package=package_name,
        executable=executable,
        parameters=[ParameterFile(LaunchConfiguration("config"))],
        ros_arguments=["--log-level", "adapi.node:=WARN"],
    )


def get_default_config():
    path = FindPackageShare("autoware_default_adapi_universe")
    path = PathJoinSubstitution([path, "config/default_adapi.param.yaml"])
    return path


def generate_launch_description():
    nodes = [
        create_api_node("autoware_default_adapi", "interface", "interface_node"),
        create_api_node("autoware_default_adapi", "localization", "localization_node"),
        create_api_node("autoware_default_adapi", "routing", "routing_node"),
        create_api_node("autoware_default_adapi_universe", "autoware_state", "autoware_state_node"),
        create_api_node("autoware_default_adapi_universe", "diagnostics", "diagnostics_node"),
        create_api_node("autoware_default_adapi_universe", "fail_safe", "fail_safe_node"),
        create_api_node("autoware_default_adapi_universe", "heartbeat", "heartbeat_node"),
        create_api_node("autoware_default_adapi_universe", "manual/local", "manual_control_node"),
        create_api_node("autoware_default_adapi_universe", "manual/remote", "manual_control_node"),
        create_api_node("autoware_default_adapi_universe", "motion", "motion_node"),
        create_api_node("autoware_default_adapi_universe", "mrm_request", "mrm_request_node"),
        create_api_node(
            "autoware_default_adapi_universe", "operation_mode", "operation_mode_node"
        ),
        create_api_node("autoware_default_adapi_universe", "perception", "perception_node"),
        create_api_node("autoware_default_adapi_universe", "planning", "planning_node"),
        create_api_node(
            "autoware_default_adapi_universe", "vehicle_status", "vehicle_status_node"
        ),
        create_api_node(
            "autoware_default_adapi_universe", "vehicle_command", "vehicle_command_node"
        ),
        create_api_node(
            "autoware_default_adapi_universe", "vehicle_metrics", "vehicle_metrics_node"
        ),
        create_api_node("autoware_default_adapi_universe", "vehicle_info", "vehicle_info_node"),
        create_api_node("autoware_default_adapi_universe", "vehicle_door", "vehicle_door_node"),
    ]
    argument = DeclareLaunchArgument("config", default_value=get_default_config())
    return launch.LaunchDescription([argument, *nodes])
