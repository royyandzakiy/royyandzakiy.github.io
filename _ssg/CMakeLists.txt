cmake_minimum_required(VERSION 3.20)
project(ssg LANGUAGES CXX)

# Unity build: each executable is a single translation unit that includes
# the portable core (engine/ssg.cpp) and implements core/os.cpp, with
# platform/main.cpp for ssg and test/main_test.cpp for the unit tests.

include(CheckCXXCompilerFlag)

# Includes are rooted here: "core/base.cpp", "parser/markdown.cpp", ...
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(
  -Wall -Wextra -Wconversion -Wno-sign-conversion
  -Wno-unused-parameter -Wno-unused-function
)

# Debug builds trap on undefined behavior, which includes failed asserts.
set(SSG_TRAP "")
check_cxx_compiler_flag(-fsanitize-trap SSG_HAS_TRAP)
if(SSG_HAS_TRAP)
  set(SSG_TRAP -fsanitize=undefined -fsanitize-trap)
else()
  check_cxx_compiler_flag(-fsanitize-undefined-trap-on-error SSG_HAS_TRAP2)
  if(SSG_HAS_TRAP2)
    set(SSG_TRAP -fsanitize=undefined -fsanitize-undefined-trap-on-error)
  endif()
endif()
add_compile_options("$<$<CONFIG:Debug>:-g3;${SSG_TRAP}>")
add_link_options("$<$<CONFIG:Debug>:${SSG_TRAP}>")

# Windows: link the C++ runtime statically, so the .exe files run on any
# machine without runtime DLLs beside them.
if(CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(WIN32)
  add_link_options(-static)
endif()

if(WIN32)
  # UTF-8 process code page: argv and narrow paths are UTF-8, as on POSIX
  add_executable(ssg platform/main.cpp platform/utf8.manifest)
else()
  add_executable(ssg platform/main.cpp)
endif()

if(NOT CMAKE_CROSSCOMPILING)
  enable_testing()
  add_executable(ssg_test test/main_test.cpp)
  add_test(NAME unit COMMAND ssg_test)

  # Development tool: highlight standard input (see test/hlgallery.py)
  add_executable(hlcat test/hlcat.cpp)
endif()
