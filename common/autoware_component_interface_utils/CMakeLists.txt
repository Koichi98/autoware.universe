cmake_minimum_required(VERSION 3.14)
project(autoware_component_interface_utils)

find_package(autoware_cmake REQUIRED)
autoware_package()

# This package is header-only. When Agnocast is enabled, the interface proxies are compiled against
# autoware::agnocast_wrapper::Node in consumers; export the dependency so its headers and the
# USE_AGNOCAST_ENABLED compile definition reach them. When disabled, the plain rclcpp::Node path is
# used and nothing extra is needed (full backward compatibility).
if(DEFINED ENV{ENABLE_AGNOCAST} AND "$ENV{ENABLE_AGNOCAST}" STREQUAL "1")
  find_package(autoware_agnocast_wrapper REQUIRED)
  ament_export_dependencies(autoware_agnocast_wrapper)
endif()

include_directories(
  include
  SYSTEM
  ${rclcpp_INCLUDE_DIRS}
  ${rosidl_runtime_cpp_INCLUDE_DIRS}
  ${rcl_INCLUDE_DIRS}
  ${autoware_adapi_v1_msgs_INCLUDE_DIRS}
)

if(BUILD_TESTING)
  ament_add_ros_isolated_gtest(test_component_interface_utils
    test/test_component_interface_utils.cpp
  )

  target_include_directories(test_component_interface_utils
    PRIVATE include
  )
endif()

ament_auto_package()
