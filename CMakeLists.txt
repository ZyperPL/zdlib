CMAKE_MINIMUM_REQUIRED(VERSION 3.16)

SET(NAME "zd")
project(${NAME} CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

find_program(CCACHE ccache)
if(CCACHE)
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
else()
  message("ccache cannot be found")
endif()

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Debug")
ENDIF()
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")

INCLUDE_DIRECTORIES("src/ZD/" "src/")

ADD_DEFINITIONS(-DNAME="${NAME}")

option(ENABLE_TESTING "Enable Test Builds" ON)

set(SANITIZERS "")
option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" FALSE)
if(ENABLE_SANITIZER_ADDRESS)
  list(APPEND SANITIZERS "address")
endif()

option(ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" FALSE)
if(ENABLE_SANITIZER_MEMORY)
  list(APPEND SANITIZERS "memory")
endif()

option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
        "Enable undefined behavior sanitizer" FALSE)
if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
  list(APPEND SANITIZERS "undefined")
endif()

option(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" FALSE)
if(ENABLE_SANITIZER_THREAD)
  list(APPEND SANITIZERS "thread")
endif()
list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)
set(SANITIZERS_STR "")
if(NOT "${LIST_OF_SANITIZERS}" STREQUAL "")
  set(SANITIZERS_STR "-fsanitize=${LIST_OF_SANITIZERS}")
endif()

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb ${SANITIZERS_STR} -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -DDEBUG")
SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -O3 -s -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
add_compile_options(${CMAKE_CXX_FLAGS} -fPIC -std=c++20 -Wall)

FILE(GLOB SOURCES "src/*.cpp" "src/ZD/*.cpp")
ADD_LIBRARY(${NAME} SHARED "${SOURCES}")
TARGET_LINK_LIBRARIES(${NAME} PRIVATE stdc++ GL GLEW glfw pthread)

if(ENABLE_TESTING)
  message("Building Tests")
  add_subdirectory(test)
endif()
