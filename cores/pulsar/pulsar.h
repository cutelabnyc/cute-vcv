#ifndef PULSAR_H
#define PULSAR_H

#include "../oscillator/oscillator.h"
#include "../phasor/phasor.h"
#include "biquad.h"
#include "minblep.h"
#include "pulsar_oscillator.h"
#include <math.h>
#include <stdlib.h>

#define VOICE_MAX 6
#define RAMPS_MAX 8
#define BLEP_MAX 6
#define OSCILLATOR_COUNT 6
#define PI 3.14159265358979323846

#ifdef __cplusplus
extern "C" {
#endif

// If we run out of voices, a nice, gradual ramp back to zero.
typedef struct ps_ramp {
  float start;
  float increment;
  uint16_t steps_remaining;
  uint8_t active;
} ps_ramp_t;

typedef struct ps_voice {
  uint8_t active;
  float grain_length;
  float rate;
  float waveform;
  float bandwidth;
  float last_output;
  float last_discontinuous_output;
  uint16_t step;
  uint16_t age;
  uint32_t phase;
  uint32_t phase_increment;
  uint8_t use_extendend_osc;
} ps_voice_t;

typedef struct ps {
  phasor_t phasor;
  phasor_t osc_phasor;
  ps_voice_t voices[VOICE_MAX];
  ps_ramp_t ramps[RAMPS_MAX];
  minblep_t bleps[BLEP_MAX];
  oscillator_function_t oscillator_functions[OSCILLATOR_COUNT];
  osc_process_context_t context;
  float sample_rate;
  float inv_sample_rate;
  float last_input;
  float last_output;
  float last_ratio_lock;
  float last_ratio;
  float debug_value;
  float sync_output;
  float osc_phase;
  float internal_mod_rate;

  // computed intermediate parameter values, used once per block
  float oscPhaseDelta;
  float density_ratio;
  float mod_ratio;
  float mod_depth;
  float waveform;
  uint8_t frequency_couple;
} ps_t;

    // uint16_t pitch_pot;
    // uint16_t density_pot;
    // uint16_t cadence_pot;
    // uint16_t torque_pot;
    // uint16_t waveform_pot;

    // uint16_t density_jack;
    // uint16_t cadence_jack;
    // uint16_t torque_jack;
    // uint16_t waveform_jack;
    // uint16_t voct_jack;
    // uint16_t linear_fm_jack;
    // uint16_t fm_index_jack;
    // uint16_t sync_jack;

    // uint16_t coupling_switch;
    // uint16_t quant_switch;
typedef struct pulsar_state {
  float pitch;
  float density;
  float cadence;
  float torque;
  float waveform;

  float v_oct;
  float linear_fm;
  float fm_index;

  uint8_t sync;
  uint8_t coupling;
  uint8_t quantization;
} pulsar_state_t;

// Pulsar functions
void pulsar_init(ps_t *self, float sample_rate);

// Per-block configuration. Reduce the amount of computation per-sample by
// calling this function once per block
void pulsar_configure(
  ps_t *self,
  float pulse_frequency,
  float density_ratio,
  float mod_ratio,
  float mod_depth,
  float waveform,
  uint8_t ratio_lock,
  uint8_t frequency_couple
);

// pulse_frequency and resync must be provided per-sample, since these 
// are sample-rate inputs
float pulsar_process(ps_t *self, float pulse_frequency, uint8_t resync, float *debug_value);
float pulsar_get_debug_value(ps_t *self);
float pulsar_get_internal_lfo_phase(ps_t *self);
float pulsar_get_sync_output(ps_t *self);
float pulsar_get_internal_mod_rate(ps_t *self);

#ifdef __cplusplus
}
#endif

#endif // PULSAR_H
