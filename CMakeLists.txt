cmake_minimum_required(VERSION 3.5.1)

# project name
project(vulkan)

# creates the variable EXEC and sets it to hello
set(EXEC vulkan)

# set the C++20 standard
set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lglfw -lvulkan -lglad -ldl -lm -lGL -lm -lcglm")

# I../include
# L../lib
include_directories(include)
include_directories(src)
link_directories(lib)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(lib/googletest)
