cmake_minimum_required(VERSION 3.12)
project(StrecsFEM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(VTK REQUIRED
  COMPONENTS
    CommonCore
    CommonDataModel
    IOXML
)
include(${VTK_USE_FILE})

# Find nlohmann-json
find_package(nlohmann_json 3.2.0 REQUIRED)

# Find Gmsh
find_path(GMSH_INCLUDE_DIR 
    NAMES gmsh.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        $ENV{GMSH_DIR}/include
        $ENV{GMSH_ROOT}/include
    PATH_SUFFIXES gmsh
)

find_library(GMSH_LIBRARY 
    NAMES gmsh
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        $ENV{GMSH_DIR}/lib
        $ENV{GMSH_ROOT}/lib
)

# Try to use pkg-config as fallback
if(NOT GMSH_INCLUDE_DIR OR NOT GMSH_LIBRARY)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_GMSH QUIET gmsh)
        if(PC_GMSH_FOUND)
            set(GMSH_INCLUDE_DIR ${PC_GMSH_INCLUDE_DIRS})
            set(GMSH_LIBRARY ${PC_GMSH_LIBRARIES})
        endif()
    endif()
endif()

if(NOT GMSH_INCLUDE_DIR OR NOT GMSH_LIBRARY)
    message(FATAL_ERROR "Could not find Gmsh. Please install Gmsh or set GMSH_ROOT environment variable.")
endif()

message(STATUS "Found Gmsh: ${GMSH_LIBRARY}")
message(STATUS "Gmsh include dir: ${GMSH_INCLUDE_DIR}")

# Create frd2vtu library
add_library(frd2vtu_lib frd2vtu.cpp)
target_link_libraries(frd2vtu_lib PRIVATE ${VTK_LIBRARIES})

# Create step2inp library with modular components
add_library(step2inp_lib
    step2inp.cpp
    step2inp/MeshGenerator.cpp
    step2inp/ConstraintSetter.cpp
    step2inp/MaterialSetter.cpp
    step2inp/LoadConditionSetter.cpp
    step2inp/InpWriter.cpp
)
target_include_directories(step2inp_lib
    PRIVATE ${GMSH_INCLUDE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(step2inp_lib PRIVATE ${GMSH_LIBRARY})

# Create simulation config library
add_library(simulation_config_lib simulation_config.cpp)
target_link_libraries(simulation_config_lib PRIVATE nlohmann_json::nlohmann_json)

# Create main executable
add_executable(strecsfem main.cpp)
target_link_libraries(strecsfem PRIVATE 
    frd2vtu_lib 
    step2inp_lib
    simulation_config_lib
    nlohmann_json::nlohmann_json
)