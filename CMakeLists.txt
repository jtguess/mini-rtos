cmake_minimum_required(VERSION 3.20)
project(mini_rtos CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mini_rtos
  src/main.cpp
  src/scheduler.cpp
)

target_include_directories(mini_rtos PRIVATE src)
