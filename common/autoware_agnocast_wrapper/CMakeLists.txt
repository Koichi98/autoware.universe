cmake_minimum_required(VERSION 3.14)
project(autoware_agnocast_wrapper)

find_package(autoware_cmake REQUIRED)
autoware_package()

find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(class_loader REQUIRED)
find_package(agnocastlib REQUIRED)
find_package(autoware_utils REQUIRED)

# Define the setup function locally for self-build
# Always enable USE_AGNOCAST_ENABLED when agnocastlib is available.
# Runtime switching is controlled by ENABLE_AGNOCAST environment variable.
function(autoware_agnocast_wrapper_setup target)
  target_compile_definitions(${target} PUBLIC USE_AGNOCAST_ENABLED)
endfunction()

# Build the node wrapper library
ament_auto_add_library(${PROJECT_NAME} SHARED
  src/node.cpp
)

ament_target_dependencies(${PROJECT_NAME}
  rclcpp
  rclcpp_components
  class_loader
  agnocastlib
  autoware_utils
)

# Apply agnocast wrapper setup
autoware_agnocast_wrapper_setup(${PROJECT_NAME})

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_dependencies(
  rclcpp
  rclcpp_components
  class_loader
  agnocastlib
  autoware_utils
)

# Install cmake files
install(
  FILES
    cmake/agnocast_wrapper_config_extras.cmake
    cmake/autoware_agnocast_wrapper_register_node.cmake
  DESTINATION share/${PROJECT_NAME}/cmake
)

# Install templates
install(
  DIRECTORY templates/
  DESTINATION share/${PROJECT_NAME}/templates
)

ament_auto_package(
  CONFIG_EXTRAS "cmake/agnocast_wrapper_config_extras.cmake"
)
