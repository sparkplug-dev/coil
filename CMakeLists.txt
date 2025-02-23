cmake_minimum_required(VERSION 3.10)

project(
    coil
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Update git submodules
option(GIT_SUBMODULE "Update submodules during build" ON)

if(GIT_SUBMODULE)
    find_package(Git REQUIRED)
    message(STATUS "Submodule update")

    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)

    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(
            FATAL_ERROR 
            "git submodule update failed (${GIT_SUBMOD_RESULT})"
        )
    endif()
endif()

# Check if submodules are updated
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/json/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded")
endif()
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/spdlog/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded")
endif()

# Add the json library to the build
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory("./extern/json")

# Find the D-Bus library
find_package(sdbus-c++ REQUIRED)

# Build spdlog
add_subdirectory("./extern/spdlog")

# Build the interface library
add_subdirectory("./coil-lib")

# Build the daemon and the configuration tool
add_subdirectory("./coil-daemon")
add_subdirectory("./coil")
