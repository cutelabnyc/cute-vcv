# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I../cores/pulsar
FLAGS += -I../cores/phasor
FLAGS += -I../cores/oscillator

# Comment these out if you don't want the gen export involved
FLAGS += -I../cores/pulsar/gen_dsp

CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)
SOURCES += ../cores/pulsar/biquad.c
SOURCES += ../cores/pulsar/pulsar.c
SOURCES += ../cores/pulsar/pulsar_lut.c 
SOURCES += ../cores/pulsar/pulsar_oscillator.c
SOURCES += ../cores/pulsar/minblep.c
SOURCES += ../cores/pulsar/minblep_lut.c 
SOURCES += ../cores/oscillator/oscillator.c 
SOURCES += ../cores/phasor/phasor.c 
SOURCES += ../cores/oscillator/lut/lut_sin_uint16.c
SOURCES += ../cores/oscillator/lut/lut_saw_uint16.c
SOURCES += ../cores/oscillator/lut/lut_square_uint16.c
SOURCES += ../cores/oscillator/lut/lut_tri_uint16.c

# Comment these out if you don't want the gen export involved
# SOURCES += ../cores/pulsar/gen_dsp/genlib.cpp
# SOURCES += ../cores/pulsar/gen_dsp/json_builder.c
# SOURCES += ../cores/pulsar/gen_dsp/json.c
# SOURCES += ../cores/pulsar/mom_jeans_gen.cpp

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
