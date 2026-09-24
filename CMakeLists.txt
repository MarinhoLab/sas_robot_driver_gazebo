cmake_minimum_required(VERSION 3.8)
project(sas_robot_driver_gazebo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sas_core REQUIRED)
find_package(sas_common REQUIRED)
find_package(sas_robot_driver REQUIRED)
# sdformat / gz-math for the SDF serial-manipulator loader.
find_package(sdformat14 REQUIRED)
find_package(gz-math7 REQUIRED)
# ROS-free SmartArmStack core (MarinhoLab/sas_cpp); provides the
# marinholab::sas::core::modeling::SerialManipulatorSimulatorFriendly kinematic
# model used by the SDF serial-manipulator loader.
find_package(marinholab_sas_core REQUIRED)

# Find the Gazebo Transport library
find_package(gz-transport13 REQUIRED)

add_subdirectory(src/plugins/absolute_pose_publisher)
install(TARGETS AbsolutePosePublisher
    LIBRARY DESTINATION share/${PROJECT_NAME}/plugins)

add_subdirectory(src/plugins/scoped_set_pose_plugin)
install(TARGETS ScopedSetPose
    LIBRARY DESTINATION share/${PROJECT_NAME}/plugins)

############################
# CPP Library Block [BEGIN]#
# vvvvvvvvvvvvvvvvvvvvvvvv #
# SDF serial-manipulator loader: builds an
# SerialManipulatorSimulatorFriendly (marinholab::sas::core::modeling,
# provided by the marinholab_sas_core package) from a Gazebo-SDF model.
set(RCLCPP_LOCAL_LIBRARY_NAME serial_manipulator_sdf)

add_library(${RCLCPP_LOCAL_LIBRARY_NAME}
    src/SdfSerialManipulatorLoader.cc
)

# The public headers expose dqrobotics, sdf and the sas-core modeling types,
# so link PUBLIC. marinholab::sas::core pulls in the kinematic-model library
# and its dqrobotics dependency; Eigen is still linked directly because the
# loader headers use Eigen types.
target_link_libraries(${RCLCPP_LOCAL_LIBRARY_NAME}
    PUBLIC
    Eigen3::Eigen
    marinholab::sas::core
    sdformat14::sdformat14
    gz-math7::gz-math7
)

target_include_directories(${RCLCPP_LOCAL_LIBRARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(${RCLCPP_LOCAL_LIBRARY_NAME} PUBLIC cxx_std_17)

install(TARGETS ${RCLCPP_LOCAL_LIBRARY_NAME}
    DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY include/${PROJECT_NAME}
    DESTINATION include)

unset(RCLCPP_LOCAL_LIBRARY_NAME)
# ^^^^^^^^^^^^^^^^^^^^^^ #
# CPP Library Block [END] #
##########################

############################
# CPP Binary Block [BEGIN] #
# vvvvvvvvvvvvvvvvvvvvvvvv #
# Command-line tool: print an SDF model's kinematics as YAML.
set(RCLCPP_LOCAL_BINARY_NAME sdf2manipulator)

add_executable(${RCLCPP_LOCAL_BINARY_NAME}
    src/${RCLCPP_LOCAL_BINARY_NAME}.cc
)

target_link_libraries(${RCLCPP_LOCAL_BINARY_NAME}
    serial_manipulator_sdf
)

install(TARGETS ${RCLCPP_LOCAL_BINARY_NAME}
    DESTINATION lib/${PROJECT_NAME})

unset(RCLCPP_LOCAL_BINARY_NAME)
# ^^^^^^^^^^^^^^^^^^^^^^ #
# CPP Binary Block [END] #
##########################

############################
# Test Block [BEGIN]       #
# vvvvvvvvvvvvvvvvvvvvvvvv #
# Standalone loader test (run with a SDF model as argument,
# e.g. test_sdf_serial_manipulator_loader sdf/r820.sdf).
add_executable(test_sdf_serial_manipulator_loader
    src/test_sdf_serial_manipulator_loader.cc
)

target_link_libraries(test_sdf_serial_manipulator_loader
    serial_manipulator_sdf
)
# ^^^^^^^^^^^^^^^^^^ #
# Test Block [END]     #
##########################

ament_python_install_package(${PROJECT_NAME})

#########################
# Scripts Block [BEGIN] #
# vvvvvvvvvvvvvvvvvvvvv #
# https://ros2-tutorial.readthedocs.io/en/latest/
install(DIRECTORY
        scripts/
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
        DESTINATION lib/${PROJECT_NAME}
)
# ^^^^^^^^^^^^^^^^^^^ #
# Scripts Block [END] #
#######################

########################
# Share Block [BEGIN]  #
# vvvvvvvvvvvvvvvvvvvv #
# According to https://github.com/SmartArmStack/sas_robot_driver/blob/ros2/CMakeLists.txt
install(DIRECTORY
        launch
        config
        sdf
        vendor
        DESTINATION share/${PROJECT_NAME}/
)
# ^^^^^^^^^^^^^^^^^^ #
# Share Block [END]  #
######################

############################
# CPP Binary Block [BEGIN] #
# vvvvvvvvvvvvvvvvvvvvvvvv #
# https://ros2-tutorial.readthedocs.io/en/latest/
# While we cant use blocks https://cmake.org/cmake/help/latest/command/block.html#command:block
# we use set--unset
set(RCLCPP_LOCAL_BINARY_NAME gazebo_service_frequency_checker)

add_executable(${RCLCPP_LOCAL_BINARY_NAME}
    src/gazebo_service_frequency_checker.cpp

    )

ament_target_dependencies(${RCLCPP_LOCAL_BINARY_NAME}
    sas_core

    )

target_link_libraries(${RCLCPP_LOCAL_BINARY_NAME}
    gz-transport13::core

    )

target_include_directories(${RCLCPP_LOCAL_BINARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(${RCLCPP_LOCAL_BINARY_NAME} PUBLIC c_std_99 cxx_std_17)

install(TARGETS ${RCLCPP_LOCAL_BINARY_NAME}
    DESTINATION lib/${PROJECT_NAME})

unset(RCLCPP_LOCAL_BINARY_NAME)
# ^^^^^^^^^^^^^^^^^^^^^^ #
# CPP Binary Block [END] #
##########################

############################
# CPP Binary Block [BEGIN] #
# vvvvvvvvvvvvvvvvvvvvvvvv #
# https://ros2-tutorial.readthedocs.io/en/latest/
# While we cant use blocks https://cmake.org/cmake/help/latest/command/block.html#command:block
# we use set--unset
set(RCLCPP_LOCAL_BINARY_NAME sas_object_server_gazebo_node)

add_executable(${RCLCPP_LOCAL_BINARY_NAME}
    src/${RCLCPP_LOCAL_BINARY_NAME}.cpp
    src/sas_object_server_gazebo.cpp

    )

ament_target_dependencies(${RCLCPP_LOCAL_BINARY_NAME}
    sas_core
    sas_common

    )

target_link_libraries(${RCLCPP_LOCAL_BINARY_NAME}
    gz-transport13::core

    )

target_include_directories(${RCLCPP_LOCAL_BINARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(${RCLCPP_LOCAL_BINARY_NAME} PUBLIC c_std_99 cxx_std_17)

install(TARGETS ${RCLCPP_LOCAL_BINARY_NAME}
    DESTINATION lib/${PROJECT_NAME})

unset(RCLCPP_LOCAL_BINARY_NAME)
# ^^^^^^^^^^^^^^^^^^^^^^ #
# CPP Binary Block [END] #
##########################

############################
# CPP Binary Block [BEGIN] #
# vvvvvvvvvvvvvvvvvvvvvvvv #
# https://ros2-tutorial.readthedocs.io/en/latest/
# While we cant use blocks https://cmake.org/cmake/help/latest/command/block.html#command:block
# we use set--unset
set(RCLCPP_LOCAL_BINARY_NAME gazebo_subscription_frequency_checker)

add_executable(${RCLCPP_LOCAL_BINARY_NAME}
    src/${RCLCPP_LOCAL_BINARY_NAME}.cpp

    )

ament_target_dependencies(${RCLCPP_LOCAL_BINARY_NAME}
    sas_core

    )

target_link_libraries(${RCLCPP_LOCAL_BINARY_NAME}
    gz-transport13::core

    )

target_include_directories(${RCLCPP_LOCAL_BINARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(${RCLCPP_LOCAL_BINARY_NAME} PUBLIC c_std_99 cxx_std_17)

install(TARGETS ${RCLCPP_LOCAL_BINARY_NAME}
    DESTINATION lib/${PROJECT_NAME})

unset(RCLCPP_LOCAL_BINARY_NAME)
# ^^^^^^^^^^^^^^^^^^^^^^ #
# CPP Binary Block [END] #
##########################

############################
# CPP Binary Block [BEGIN] #
# vvvvvvvvvvvvvvvvvvvvvvvv #
# https://ros2-tutorial.readthedocs.io/en/latest/
# While we cant use blocks https://cmake.org/cmake/help/latest/command/block.html#command:block
# we use set--unset
set(RCLCPP_LOCAL_BINARY_NAME sas_simulator_server_gazebo_node)

add_executable(${RCLCPP_LOCAL_BINARY_NAME}
    src/${RCLCPP_LOCAL_BINARY_NAME}.cpp

    )

ament_target_dependencies(${RCLCPP_LOCAL_BINARY_NAME}
    sas_core
    sas_common

    )

target_link_libraries(${RCLCPP_LOCAL_BINARY_NAME}
    gz-transport13::core

    )

target_include_directories(${RCLCPP_LOCAL_BINARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(${RCLCPP_LOCAL_BINARY_NAME} PUBLIC c_std_99 cxx_std_17)

install(TARGETS ${RCLCPP_LOCAL_BINARY_NAME}
    DESTINATION lib/${PROJECT_NAME})

unset(RCLCPP_LOCAL_BINARY_NAME)
# ^^^^^^^^^^^^^^^^^^^^^^ #
# CPP Binary Block [END] #
##########################

# Environment hooks
ament_environment_hooks(
    "${CMAKE_CURRENT_SOURCE_DIR}/env-hooks/sas_robot_driver_gazebo.dsv.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/env-hooks/gz_sim_resource_path.sh.in"
)

ament_package()
