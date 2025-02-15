cmake_minimum_required(VERSION 3.10)

project(
    coil
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add the json library to the build
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory("./extern/json")

# Find the DBus library
find_package(sdbus-c++ REQUIRED)

# Build the interface library
add_subdirectory("./coil-lib")

# Build the daemon and the configuration tool
add_subdirectory("./coil-daemon")
add_subdirectory("./coil")
