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

# Find the Gazebo Transport library
find_package(gz-transport13 REQUIRED)

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

##### CPP BINARY #####

add_executable(requester_async
    src/requester_async.cpp)

target_link_libraries(requester_async
    PRIVATE
    gz-transport13::core
    )

install(TARGETS
    requester_async
    DESTINATION lib/${PROJECT_NAME})

##END## CPP BINARY #####

ament_package()    
