#ifndef PS_OSCILLATOR_H
#define PS_OSCILLATOR_H

#include <stdint.h>

typedef struct osc_process_context {
  float discontinuity_amplitude;
  float discontinuity_phase;
  float debug_value;
  uint8_t produced_discontinuity; // Flag to indicate if a discontinuity was produced
} osc_process_context_t;

// function pointer typedef for oscillator functions
typedef float (*oscillator_function_t)(
    uint32_t raw_phase,
    uint32_t phase_increment,
    float bipolar_phase,
    float bandwidth,
    osc_process_context_t *context);

// oscillator functions
float slow_sinc(
    uint32_t raw_phase,
    uint32_t phase_increment,
    float bipolar_phase,
    float bandwidth,
    osc_process_context_t *context);

float triangle(
    uint32_t raw_phase,
    uint32_t phase_increment,
    float bipolar_phase,
    float bandwidth,
    osc_process_context_t *context);

float blep_square(
    uint32_t raw_phase,
    uint32_t phase_increment,
    float bipolar_phase,
    float bandwidth,
    osc_process_context_t *context);

float blep_saw(
    uint32_t raw_phase,
    uint32_t phase_increment,
    float bipolar_phase,
    float bandwidth,
    osc_process_context_t *context);

float contained_square(
    uint32_t raw_phase,
    uint32_t phase_increment,
    float bipolar_phase,
    float bandwidth,
    osc_process_context_t *context);

float raw_dirty_square(
    uint32_t raw_phase,
    uint32_t phase_increment,
    float bipolar_phase,
    float bandwidth,
    osc_process_context_t *context);

float raw_dirty_saw(
    uint32_t raw_phase,
    uint32_t phase_increment,
    float bipolar_phase,
    float bandwidth,
    osc_process_context_t *context);

#endif // PS_OSCILLATOR_H