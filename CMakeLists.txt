cmake_minimum_required(VERSION 3.5.1)

# project name
project(vulkan)

# creates the variable EXEC and sets it to hello
set(EXEC vulkan)

# set the C++20 standard
set(CMAKE_CXX_STANDARD 20)

# set any compiler flags
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# I../include
# L../lib
include_directories(include)
include_directories(src)

# execute command at build time to run the bash script to compile shaders
add_custom_target(comp_shaders
  COMMAND bash ../shaders/compile.sh "__cmake"
  COMMENT "Compiling shaders..."
)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(lib/googletest)
link_directories(lib)