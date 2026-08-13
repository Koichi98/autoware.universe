cmake_minimum_required(VERSION 3.14)
project(autoware_default_adapi_universe)

find_package(autoware_cmake REQUIRED)
autoware_package()
find_package(autoware_agnocast_wrapper REQUIRED)

ament_auto_add_library(${PROJECT_NAME} SHARED
  src/diagnostics.cpp
  src/fail_safe.cpp
  src/heartbeat.cpp
  src/manual_control.cpp
  src/motion.cpp
  src/mrm_request.cpp
  src/operation_mode.cpp
  src/perception.cpp
  src/planning.cpp
  src/vehicle_status.cpp
  src/vehicle_command.cpp
  src/vehicle_metrics.cpp
  src/vehicle_info.cpp
  src/vehicle_door.cpp
  src/utils/command_conversion.cpp
  src/compatibility/autoware_state.cpp
)
ament_target_dependencies(${PROJECT_NAME} autoware_agnocast_wrapper)
autoware_agnocast_wrapper_setup(${PROJECT_NAME})

# These nodes derive from autoware::agnocast_wrapper::Node, which requires an AgnocastOnly
# executor, so they run as standalone executables rather than inside a component container.
# The container was component_container_mt, so the multi-threaded executors are kept: some of
# these nodes call a client from inside a service callback and deadlock on a single thread.
foreach(node
  "VehicleStatusNode;vehicle_status_node"
  "VehicleInfoNode;vehicle_info_node"
  "VehicleCommandNode;vehicle_command_node"
  "VehicleMetricsNode;vehicle_metrics_node"
  "PerceptionNode;perception_node"
  "FailSafeNode;fail_safe_node"
  "HeartbeatNode;heartbeat_node"
  "DiagnosticsNode;diagnostics_node"
  "OperationModeNode;operation_mode_node"
  "AutowareStateNode;autoware_state_node"
  "MrmRequestNode;mrm_request_node"
  "VehicleDoorNode;vehicle_door_node"
  "MotionNode;motion_node"
  "PlanningNode;planning_node"
  "ManualControlNode;manual_control_node"
)
  list(GET node 0 class_name)
  list(GET node 1 executable_name)
  autoware_agnocast_wrapper_register_node(${PROJECT_NAME}
    PLUGIN "autoware::default_adapi::${class_name}"
    EXECUTABLE ${executable_name}
    ROS2_EXECUTOR MultiThreadedExecutor
    AGNOCAST_EXECUTOR AgnocastOnlyMultiThreadedExecutor
  )
endforeach()

if(BUILD_TESTING)
endif()

install(
  PROGRAMS script/web_server.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_auto_package(INSTALL_TO_SHARE config launch)
