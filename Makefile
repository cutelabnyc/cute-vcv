# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# If CORES_DIR is not defined when calling the Makefile, default to one directory above
CORES_DIR ?= ./cores

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I$(CORES_DIR)/pulsar
FLAGS += -I$(CORES_DIR)/phasor
FLAGS += -I$(CORES_DIR)/oscillator

# Comment these out if you don't want the gen export involved
# FLAGS += -I$(CORES_DIR)/pulsar/gen_dsp

CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(CORES_DIR)/pulsar/biquad.c
SOURCES += $(CORES_DIR)/pulsar/pulsar.c
SOURCES += $(CORES_DIR)/pulsar/pulsar_lut.c
SOURCES += $(CORES_DIR)/pulsar/pulsar_oscillator.c
SOURCES += $(CORES_DIR)/pulsar/minblep.c
SOURCES += $(CORES_DIR)/pulsar/minblep_lut.c
SOURCES += $(CORES_DIR)/oscillator/oscillator.c 
SOURCES += $(CORES_DIR)/phasor/phasor.c 
SOURCES += $(CORES_DIR)/oscillator/lut/lut_sin_uint16.c
SOURCES += $(CORES_DIR)/oscillator/lut/lut_saw_uint16.c
SOURCES += $(CORES_DIR)/oscillator/lut/lut_square_uint16.c
SOURCES += $(CORES_DIR)/oscillator/lut/lut_tri_uint16.c

# Comment these out if you don't want the gen export involved
# SOURCES += $(CORES_DIR)/pulsar/gen_dsp/genlib.cpp
# SOURCES += $(CORES_DIR)/pulsar/gen_dsp/json_builder.c
# SOURCES += $(CORES_DIR)/pulsar/gen_dsp/json.c
# SOURCES += $(CORES_DIR)/pulsar/mom_jeans_gen.cpp

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
