cmake_minimum_required(VERSION 3.16)
project(scoped_set_pose_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(gz-sim8 REQUIRED)
find_package(gz-msgs10 REQUIRED)
find_package(gz-transport13 REQUIRED)

add_library(ScopedSetPose SHARED
  ScopedSetPose.cc
)

target_link_libraries(ScopedSetPose PRIVATE
  gz-sim8::gz-sim8
  gz-msgs10::gz-msgs10
  gz-transport13::gz-transport13
)


