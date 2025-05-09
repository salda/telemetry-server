cmake_minimum_required(VERSION 3.14)
project(telemetry-server VERSION 0.1.0)

# Set C++20 as required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow deprecated FetchContent_Populate for Pistache's RapidJSON dependency
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

# Include CMake modules
include(cmake/Dependencies.cmake)

# Build the component libraries first
add_subdirectory(lib)

# Build the main application
add_subdirectory(src)

# Build tests
option(BUILD_TESTING "Build test targets" ON)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

# Build information
message(STATUS "Building telemetry-server v${PROJECT_VERSION} with C++20")
message(STATUS "Using Pistache v0.4.26 for HTTP server")
message(STATUS "Using nlohmann/json v3.11.3 for JSON parsing")
