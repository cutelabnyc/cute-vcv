#include "pulsar_oscillator.h"
#include <math.h>

#define PI 3.14159265358979323846

float slow_sinc(uint32_t raw_phase, uint32_t phase_increment, float bipolar_phase, float bandwidth, osc_process_context_t *context)
{
  float x = bipolar_phase * PI * bandwidth;
  if (fabsf(x) < 0.0001f) {
    return 1.0f; // Handle the zero case
  }
  
  #ifdef ARM_MATH_CM7
  float sinx = arm_sin_f32(x);
  #else
  float sinx = sinf(x);
  #endif

  return sinx / x; // Sinc function: sin(x) / x
}

float triangle(uint32_t raw_phase, uint32_t phase_increment, float bipolar_phase, float bandwidth, osc_process_context_t *context)
{
  // Triangle wave generation
  float x = bipolar_phase * bandwidth;
  // if (fabsf(x) < 0.0001f) {
  //   return 1.0f; // Handle the zero case
  // }

  // periodic triangle wave function, centered around 0
  float t = fabsf(fmodf(fabsf(x + 0.5f), 2.0f) - 1.0f) * -2.0f + 1.0f;

  if (raw_phase < phase_increment) {
    // We are crossing from the second half to the first half
    context->discontinuity_amplitude += t; // Full amplitude change

    // If we haven't produced a discontinuity yet, we need to set it up
    if (!context->produced_discontinuity) {
      float discontinuity_phase = (float)(raw_phase) / phase_increment;
      context->discontinuity_phase = -discontinuity_phase;
    }
    context->produced_discontinuity = 1;
  }

  return t;
}

float blep_square(uint32_t raw_phase, uint32_t phase_increment, float bipolar_phase, float bandwidth, osc_process_context_t *context)
{
  uint32_t state = raw_phase & 0x80000000; // MSB bit
  float output;
  if (state) {
    // If the MSB is set, we are in the second half of the square wave
    output = -1.0f;
    // Check if we need to produce a discontinuity
    if ((raw_phase - phase_increment) < 0x80000000) {
      // We are crossing from the first half to the second half
      context->discontinuity_amplitude -= 1.0f; // Full amplitude change

      // If we haven't produced a discontinuity yet, we need to set it up
      if (!context->produced_discontinuity) {
        float discontinuity_phase = (float)(raw_phase - 0x80000000) / phase_increment;
        context->discontinuity_phase = -discontinuity_phase;
      }
      context->produced_discontinuity = 1;
    }
  } else {
    // If the MSB is not set, we are in the first half of the square wave
    output = 1.0f;
    if (raw_phase < phase_increment) {
      // We are crossing from the second half to the first half
      context->discontinuity_amplitude += 0.5f; // Full amplitude change

      // If we haven't produced a discontinuity yet, we need to set it up
      if (!context->produced_discontinuity) {
        float discontinuity_phase = (float)(raw_phase) / phase_increment;
        context->discontinuity_phase = -discontinuity_phase;
      }
      context->produced_discontinuity = 1;
    }
  }
  return output;
}

float blep_saw(uint32_t raw_phase, uint32_t phase_increment, float bipolar_phase, float bandwidth, osc_process_context_t *context)
{
  float output = (float) raw_phase / (float) 0xFFFFFFFF; // Normalize to [0, 1]
  output = output * -2.0f + 1.0f; // Scale

  if (raw_phase < phase_increment) {
    // We are crossing from the second half to the first half
      context->discontinuity_amplitude += 1.0f; // Full amplitude change

      // If we haven't produced a discontinuity yet, we need to set it up
      if (!context->produced_discontinuity) {
        float discontinuity_phase = (float)(raw_phase) / phase_increment;
        context->discontinuity_phase = -discontinuity_phase;
      }
      context->produced_discontinuity = 1;
  }

  return output;
}

float contained_square(uint32_t raw_phase, uint32_t phase_increment, float bipolar_phase, float bandwidth, osc_process_context_t *context)
{
  float x = bipolar_phase * PI;
  float window = cosf(x) * 0.5f + 0.5f; // Convert to [0, 1]
  // x *= bandwidth;
  x = bipolar_phase * bandwidth / 2.0f;
  float t = fabsf(fmodf(fabsf(x + 0.5f), 2.0f) - 1.0f) < 0.5f ? 1.0f : -1.0f;
  return t * window;
}

float raw_dirty_square(uint32_t raw_phase, uint32_t phase_increment, float bipolar_phase, float bandwidth, osc_process_context_t *context)
{
  float x = bipolar_phase * bandwidth;
  float rect = fmodf(x, 1.0f) < 0.0f ? -1.0f : 0.0f;
  return rect;
}

float raw_dirty_saw(uint32_t raw_phase, uint32_t phase_increment, float bipolar_phase, float bandwidth, osc_process_context_t *context) {
	float x = bipolar_phase * bandwidth;
	float saw = fmodf(x, 1.0f);
	return saw;
}
