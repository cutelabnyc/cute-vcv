set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(blink
    blink.cpp
    ../../cores/pizza-gate/pizza-gate.c # Add the actual source file
    ../../cores/phasor/phasor.c # Add the actual source file
)

# Add include directories
target_include_directories(blink PRIVATE
    ../../cores/pizza-gate
    ../../cores/phasor
)

# Link to standard Pico libraries
target_link_libraries(blink pico_stdlib)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(blink pico_cyw43_arch_none)
endif()

# Enable USB Serial
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)

# Generate output files
pico_add_extra_outputs(blink)

# Set URL
example_auto_set_url(blink)

