# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.19

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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.19.6/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.19.6/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp

# Include any dependencies generated for this target.
include CMakeFiles/native-lib.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/native-lib.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/native-lib.dir/flags.make

CMakeFiles/native-lib.dir/native-lib.cpp.o: CMakeFiles/native-lib.dir/flags.make
CMakeFiles/native-lib.dir/native-lib.cpp.o: native-lib.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/native-lib.dir/native-lib.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/native-lib.dir/native-lib.cpp.o -c /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/native-lib.cpp

CMakeFiles/native-lib.dir/native-lib.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/native-lib.dir/native-lib.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/native-lib.cpp > CMakeFiles/native-lib.dir/native-lib.cpp.i

CMakeFiles/native-lib.dir/native-lib.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/native-lib.dir/native-lib.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/native-lib.cpp -o CMakeFiles/native-lib.dir/native-lib.cpp.s

CMakeFiles/native-lib.dir/FFMPEG.cpp.o: CMakeFiles/native-lib.dir/flags.make
CMakeFiles/native-lib.dir/FFMPEG.cpp.o: FFMPEG.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/native-lib.dir/FFMPEG.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/native-lib.dir/FFMPEG.cpp.o -c /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/FFMPEG.cpp

CMakeFiles/native-lib.dir/FFMPEG.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/native-lib.dir/FFMPEG.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/FFMPEG.cpp > CMakeFiles/native-lib.dir/FFMPEG.cpp.i

CMakeFiles/native-lib.dir/FFMPEG.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/native-lib.dir/FFMPEG.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/FFMPEG.cpp -o CMakeFiles/native-lib.dir/FFMPEG.cpp.s

CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.o: CMakeFiles/native-lib.dir/flags.make
CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.o: JavaPlayerCaller.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.o -c /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/JavaPlayerCaller.cpp

CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/JavaPlayerCaller.cpp > CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.i

CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/JavaPlayerCaller.cpp -o CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.s

CMakeFiles/native-lib.dir/AudioChannel.cpp.o: CMakeFiles/native-lib.dir/flags.make
CMakeFiles/native-lib.dir/AudioChannel.cpp.o: AudioChannel.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/native-lib.dir/AudioChannel.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/native-lib.dir/AudioChannel.cpp.o -c /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/AudioChannel.cpp

CMakeFiles/native-lib.dir/AudioChannel.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/native-lib.dir/AudioChannel.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/AudioChannel.cpp > CMakeFiles/native-lib.dir/AudioChannel.cpp.i

CMakeFiles/native-lib.dir/AudioChannel.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/native-lib.dir/AudioChannel.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/AudioChannel.cpp -o CMakeFiles/native-lib.dir/AudioChannel.cpp.s

CMakeFiles/native-lib.dir/VideoChannel.cpp.o: CMakeFiles/native-lib.dir/flags.make
CMakeFiles/native-lib.dir/VideoChannel.cpp.o: VideoChannel.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/native-lib.dir/VideoChannel.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/native-lib.dir/VideoChannel.cpp.o -c /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/VideoChannel.cpp

CMakeFiles/native-lib.dir/VideoChannel.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/native-lib.dir/VideoChannel.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/VideoChannel.cpp > CMakeFiles/native-lib.dir/VideoChannel.cpp.i

CMakeFiles/native-lib.dir/VideoChannel.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/native-lib.dir/VideoChannel.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/VideoChannel.cpp -o CMakeFiles/native-lib.dir/VideoChannel.cpp.s

# Object files for target native-lib
native__lib_OBJECTS = \
"CMakeFiles/native-lib.dir/native-lib.cpp.o" \
"CMakeFiles/native-lib.dir/FFMPEG.cpp.o" \
"CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.o" \
"CMakeFiles/native-lib.dir/AudioChannel.cpp.o" \
"CMakeFiles/native-lib.dir/VideoChannel.cpp.o"

# External object files for target native-lib
native__lib_EXTERNAL_OBJECTS =

libnative-lib.dylib: CMakeFiles/native-lib.dir/native-lib.cpp.o
libnative-lib.dylib: CMakeFiles/native-lib.dir/FFMPEG.cpp.o
libnative-lib.dylib: CMakeFiles/native-lib.dir/JavaPlayerCaller.cpp.o
libnative-lib.dylib: CMakeFiles/native-lib.dir/AudioChannel.cpp.o
libnative-lib.dylib: CMakeFiles/native-lib.dir/VideoChannel.cpp.o
libnative-lib.dylib: CMakeFiles/native-lib.dir/build.make
libnative-lib.dylib: CMakeFiles/native-lib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Linking CXX shared library libnative-lib.dylib"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/native-lib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/native-lib.dir/build: libnative-lib.dylib

.PHONY : CMakeFiles/native-lib.dir/build

CMakeFiles/native-lib.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/native-lib.dir/cmake_clean.cmake
.PHONY : CMakeFiles/native-lib.dir/clean

CMakeFiles/native-lib.dir/depend:
	cd /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp /Users/bingy/Material/My_Documents/011_FFMPEG-master/app/src/main/cpp/CMakeFiles/native-lib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/native-lib.dir/depend

