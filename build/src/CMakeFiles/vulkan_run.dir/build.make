# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/cmake-3.25.1-linux-x86_64/bin/cmake

# The command to remove a file.
RM = /opt/cmake-3.25.1-linux-x86_64/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/walker/Workspace/cpp-workspace/vulkan

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/walker/Workspace/cpp-workspace/vulkan/build

# Include any dependencies generated for this target.
include src/CMakeFiles/vulkan_run.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/CMakeFiles/vulkan_run.dir/compiler_depend.make

# Include the progress variables for this target.
include src/CMakeFiles/vulkan_run.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/vulkan_run.dir/flags.make

src/CMakeFiles/vulkan_run.dir/main.cpp.o: src/CMakeFiles/vulkan_run.dir/flags.make
src/CMakeFiles/vulkan_run.dir/main.cpp.o: /home/walker/Workspace/cpp-workspace/vulkan/src/main.cpp
src/CMakeFiles/vulkan_run.dir/main.cpp.o: src/CMakeFiles/vulkan_run.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/walker/Workspace/cpp-workspace/vulkan/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/vulkan_run.dir/main.cpp.o"
	cd /home/walker/Workspace/cpp-workspace/vulkan/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/vulkan_run.dir/main.cpp.o -MF CMakeFiles/vulkan_run.dir/main.cpp.o.d -o CMakeFiles/vulkan_run.dir/main.cpp.o -c /home/walker/Workspace/cpp-workspace/vulkan/src/main.cpp

src/CMakeFiles/vulkan_run.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/vulkan_run.dir/main.cpp.i"
	cd /home/walker/Workspace/cpp-workspace/vulkan/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/walker/Workspace/cpp-workspace/vulkan/src/main.cpp > CMakeFiles/vulkan_run.dir/main.cpp.i

src/CMakeFiles/vulkan_run.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/vulkan_run.dir/main.cpp.s"
	cd /home/walker/Workspace/cpp-workspace/vulkan/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/walker/Workspace/cpp-workspace/vulkan/src/main.cpp -o CMakeFiles/vulkan_run.dir/main.cpp.s

# Object files for target vulkan_run
vulkan_run_OBJECTS = \
"CMakeFiles/vulkan_run.dir/main.cpp.o"

# External object files for target vulkan_run
vulkan_run_EXTERNAL_OBJECTS =

src/vulkan_run: src/CMakeFiles/vulkan_run.dir/main.cpp.o
src/vulkan_run: src/CMakeFiles/vulkan_run.dir/build.make
src/vulkan_run: src/CMakeFiles/vulkan_run.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/walker/Workspace/cpp-workspace/vulkan/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable vulkan_run"
	cd /home/walker/Workspace/cpp-workspace/vulkan/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/vulkan_run.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/vulkan_run.dir/build: src/vulkan_run
.PHONY : src/CMakeFiles/vulkan_run.dir/build

src/CMakeFiles/vulkan_run.dir/clean:
	cd /home/walker/Workspace/cpp-workspace/vulkan/build/src && $(CMAKE_COMMAND) -P CMakeFiles/vulkan_run.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/vulkan_run.dir/clean

src/CMakeFiles/vulkan_run.dir/depend:
	cd /home/walker/Workspace/cpp-workspace/vulkan/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/walker/Workspace/cpp-workspace/vulkan /home/walker/Workspace/cpp-workspace/vulkan/src /home/walker/Workspace/cpp-workspace/vulkan/build /home/walker/Workspace/cpp-workspace/vulkan/build/src /home/walker/Workspace/cpp-workspace/vulkan/build/src/CMakeFiles/vulkan_run.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/vulkan_run.dir/depend

