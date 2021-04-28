cmake_minimum_required(VERSION 3.5)
project(my_srv_class)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(phoenix_msgs REQUIRED)





add_executable(srv_test
  src/udp.cpp
  src/my_srv_main.cpp
  src/udp.hpp
)
ament_target_dependencies(srv_test
  rclcpp
  phoenix_msgs
)

install(TARGETS
  srv_test
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()