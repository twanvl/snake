cmake_minimum_required(VERSION 3.13)

project(snake VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories("${PROJECT_BINARY_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/src")

find_package (Threads)

# snake executable

add_executable(snake)

file(GLOB_RECURSE sources src/*.cpp)
target_sources(snake PRIVATE ${sources})
target_link_libraries(${PROJECT_NAME} Threads::Threads)

# warnings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wstrict-null-sentinel -Wno-comment -Wno-unused-parameter")
endif()

