#ifndef ANTIALIASED_OSCILLATOR_H
#define ANTIALIASED_OSCILLATOR_H

#include "biquad.h"
#include "minblep.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ps_osc {
    float output;
    uint8_t count;
    minblep_t square_minblepper;
    biquad_t biquad;

    // For crossfading between different waveforms
    uint8_t use_sinc;
    float sinc_scale;
    uint8_t use_tri;
    float tri_scale;
    uint8_t use_rect;
    float rect_scale;
    uint8_t use_saw;
    float saw_scale;
    uint8_t use_special_1;
    float special_1_scale;
    uint8_t use_special_2;
    float special_2_scale;
    uint8_t use_special_3;
    float special_3_scale;
    uint8_t use_filthy_1;
    float filthy_1_scale;
    uint8_t use_filthy_2;
    float filthy_2_scale;

    // These let us order the waveforms in a specific way
    uint8_t *ordered_use[9];
    float *ordered_scale[9];
} ps_osc_t;

// Clear the oscillator state
void osc_zero(ps_osc_t *self);

// Initialize the oscillator, setting up the minblep table and biquad filter,
// and ordering the waveforms
void osc_initialize(ps_osc_t *self);

// Configure the oscillator with a waveform crossfade value
void osc_configure(ps_osc_t *self, float waveform);

// Process the oscillator for a given phase and phase increment.
void osc_process(ps_osc_t *self, int32_t phase, int32_t phase_increment);

#ifdef __cplusplus
}
#endif // extern "C"

#endif // ANTIALIASED_OSCILLATOR_H