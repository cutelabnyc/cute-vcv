cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME mom-jeans-stm32h723vet6)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/pulsar/biquad.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/pulsar/pulsar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/pulsar/pulsar_calibration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/pulsar/pulsar_lut.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/pulsar/pulsar_oscillator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/pulsar/minblep.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/pulsar/minblep_lut.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/oscillator/oscillator.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/phasor/phasor.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/oscillator/lut/lut_sin_uint16.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/oscillator/lut/lut_saw_uint16.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/oscillator/lut/lut_square_uint16.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/oscillator/lut/lut_tri_uint16.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cores/pulsar
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
