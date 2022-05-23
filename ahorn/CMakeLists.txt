cmake_minimum_required(VERSION 3.16)
project(ahorn
        VERSION 1.0
        DESCRIPTION "Code associated with the thesis."
        LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# ANTLR4 requires CXX 11 or 14.
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

option(UNIT_TESTS "Build the unit tests" ON)
option(SPDLOG_TRACE_ON "Enable trace log" OFF)

if (UNIT_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(test)
endif ()

if (SPDLOG_TRACE_ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPDLOG_TRACE_ON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")
endif ()

# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)
# Set ANTLR4_TAG to a desired commit hash value to avoid rebuilding after
# every clean and keep the build stable, at the cost of not automatically
# update to latest commit.
# Choose a commit later or equal c278c1df5d22f597d932f25669ffc32e2aece8e5 to support llvm 10
# https://github.com/antlr/antlr4/pull/2983
set(ANTLR4_TAG e4c1a74c66bd5290364ea2b36c97cd724b247357)
# add external build for antlrcpp
include(ExternalAntlr4Cpp)
# add macros to generate ANTLR Cpp code from grammar
find_package(ANTLR REQUIRED)

find_package(Boost COMPONENTS program_options filesystem REQUIRED)

add_subdirectory(extern/spdlog)
if (NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif ()

add_subdirectory(extern/crab)

add_subdirectory(src)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} lib_compiler lib_ir lib_cfg lib_pass lib_se lib_sa)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include ${Boost_INCLUDE_DIR} ${CRAB_INCLUDE_DIRS})
