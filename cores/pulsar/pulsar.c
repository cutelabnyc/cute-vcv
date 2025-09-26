#include "pulsar.h"
#include "../utils.h"
#include <stdlib.h>
#include <math.h>

#include "pulsar_lut.h"

#define FIXED_BANDWIDTH 11
#define SQUARE_BANDWIDTH 5
#define LOOKUP_SINC 0
#define TWO_OVER_PI 0.63661977236758134308f // 2 / PI

#ifdef ARM_MATH_CM7
extern uint32_t getCycleCount();
#endif

static float linear_scale(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Waveform functions (normalized)
static float sinc(int32_t phase) {
  static const float scale = 1.0f / 2147483648.0f;
  float sinx = 0.0;
  q31_t sinx_q = arm_lut_q31(phase, sinc_lut_q31);
  sinx = (float)sinx_q * scale;
  return sinx;
}

int _pulsar_add_ramp(ps_t *self, float start) {
  for (int i = 0; i < RAMPS_MAX; i++) {
    ps_ramp_t *ramp = &self->ramps[i];
    if (!ramp->active) {
      float ramp_increment = 20.0f / ((float) self->sample_rate);
      ramp->start = start;
      ramp->increment = (start > 0) ? (-1.0f * ramp_increment) : ramp_increment;
      ramp->steps_remaining = ceilf(fabsf(start) / ramp_increment);
      ramp->active = 1;

      return i;
    }
  }

  return -1;
}

int _pulsar_add_blep(ps_t *self, float discontinuity_amplitude, float discontinuity_phase) {
  for (int i = 0; i < BLEP_MAX; i++) {
    minblep_t *blep = &self->bleps[i];
    if (!blep->_active) {
      minblep_insertDiscontinuity(
        blep,
        discontinuity_phase,
        discontinuity_amplitude
      );
      blep->_active = 1;

      return i;
    }
  }

  return -1;
}

float _pulsar_process_ramp(ps_ramp_t *ramp) {
  if (ramp->active) {
    float tmp = ramp->start;
    ramp->start += ramp->increment;
    ramp->steps_remaining--;
    ramp->active = ramp->steps_remaining > 0;
    return tmp;
  }
  return 0.0f;
}

float _pulsar_process_blep(minblep_t *blep) {
  if (blep->_active) {
    float output = minblep_process(blep);
    return output;
  }
  return 0.0f;
}

int16_t _pulsar_add_voice(ps_t *self, float grain_length, float bandwidth, float waveform) {

  uint16_t highest_age = 0;
  uint16_t highest_age_index = VOICE_MAX + 1;
  int next_voice_index = -1;

  for (int i = 0; i < VOICE_MAX; i++) {
    ps_voice_t *voice = &self->voices[i];
    if (!voice->active) {
      next_voice_index = i;
      break;
    } else {
      if (highest_age < voice->age) {
        highest_age = voice->age;
        highest_age_index = i;
      }
    }
  }

  // Add a fade out and steal the voice
  if (next_voice_index < 0) {
    ps_voice_t *oldest_voice = &self->voices[highest_age_index];
    int ramp_index = _pulsar_add_ramp(self, oldest_voice->last_output);
    if (ramp_index >= 0) {
      next_voice_index = highest_age_index;
    }
  }

  if (next_voice_index >= 0) {
    ps_voice_t *voice = &self->voices[next_voice_index];
    float rate = 2.0 / (grain_length - 1);
    float phase_increment_float = 1.0 / grain_length;
    voice->active = 1;
    voice->grain_length = grain_length;
    voice->rate = rate;
    voice->waveform = waveform;
    voice->bandwidth = bandwidth;
    voice->step = 0;
    voice->phase = 0;
    voice->age = 0;
    voice->last_discontinuous_output = 0.0f;
    voice->phase_increment = ((uint32_t) 0xFFFFFFFF) * phase_increment_float;
    voice->use_extendend_osc = 0;
  }

  return -1;
}

float _pulsar_process_voice(ps_t *self, ps_voice_t *voice) {

  if (voice->step >= voice->grain_length) {
    voice->active = 0;
    if (fabsf(voice->last_output) > 0.00001f) {
      // If the voice is not active, we need to add a blep
      _pulsar_add_blep(
        self,
        -voice->last_output,
        0.0f
      );
    }
  }

  if (voice->active) {
    float samp = 0;
    int32_t phase = voice->phase;
    float bipolarphase = ((float)voice->step / (float)voice->grain_length) * 2.0f - 1.0f;

    self->context.produced_discontinuity = 0;
    self->context.discontinuity_amplitude = 0.0f;
    self->context.discontinuity_phase = 0.0f;
    self->context.debug_value = 0.0f;
    float total_discontinuity_amplitude = 0.0f;

    for (int i = 0; i < OSCILLATOR_COUNT; i++) {

      // calculate a linear scale value for each oscillator based on i and voice->waveform
      float scale = 1.0f - fabsf((float)i - voice->waveform);
      if (scale < 0.0f) {
        scale = 0.0f;
      }

      if (scale <= 0.00001f) {
        continue; // Skip this oscillator if the scale is too small
      }

      float next_samp = self->oscillator_functions[i](
        phase,
        voice->phase_increment,
        bipolarphase,
        voice->bandwidth,
        &self->context
      );

      samp += next_samp * scale;

      if (self->context.produced_discontinuity) {
        // If a discontinuity was produced, we need to accumulate the amplitude
        total_discontinuity_amplitude += self->context.discontinuity_amplitude * scale;
        self->context.discontinuity_amplitude = 0.0f;
      }
    }

    if (self->context.produced_discontinuity) {

      // If a discontinuity was produced, we need to add a minblep
      _pulsar_add_blep(
        self,
        total_discontinuity_amplitude,
        self->context.discontinuity_phase
      );
    } else {
      voice->last_discontinuous_output = 0.0f;
    }

    voice->phase += voice->phase_increment;
    voice->step++;
    voice->age++;

    voice->last_output = samp;
    return samp;
  }

  voice->last_output = 0;
  return 0;
}

float _calc_mod_ratio(float f0, float mod_ratio, uint8_t frequency_couple, uint8_t ratio_lock) {
  static const float lowCutoff = 0.25;
  static const float highCutoff = 0.5;
  static const float vibratoMax = 0.5f;
  static const float ratioMax = 1;
  static float tf = 1;
  static float fac1 = 0;

  float out = 0;

  if (frequency_couple) {
    if (mod_ratio < lowCutoff) {
      out = (mod_ratio / lowCutoff) * vibratoMax;
    } else if (mod_ratio < highCutoff) {
      tf = (mod_ratio - lowCutoff) / (highCutoff - lowCutoff);
      tf = powf(tf, 2.0f); // ease
      out = (1 - tf) * vibratoMax + (tf) * f0 * 0.1;
    } else {
      fac1 = (ratioMax - 0.1) * (mod_ratio - highCutoff) / (1 - highCutoff);
      out = (fac1 + 0.1) * f0;
    }
  } else {
    out = powf(mod_ratio, 3.5) * 1000.0;
  }

  // if (frequency_couple) {
  //   if (ratio_lock) {
  //     out = (1.0 - mod_ratio) * 15.0f;
  //     out = floorf(out);
  //     out += 1.0;
  //     out = fmaxf(out, 1.0f);
  //   } else {
  //     if (mod_ratio < lowCutoff) {
  //       out = (mod_ratio / lowCutoff) * vibratoMax;
  //     } else if (mod_ratio < highCutoff) {
  //       tf = (mod_ratio - lowCutoff) / (highCutoff - lowCutoff);
  //       out = (1 - tf) * vibratoMax + (tf) * f0 * 0.1;
  //     } else {
  //       float fac1 = (ratioMax - 0.1) * (mod_ratio - highCutoff) / (1 - highCutoff);
  //       out = (fac1 + 0.1) * f0;
  //     }
  //   }
  // } else {
  //   out = powf(mod_ratio, 2.2) * 1000.0;
  //   if (ratio_lock) {
  //     float ratio = f0 / (out + 1);
  //     ratio = ceilf(ratio);
  //     out = ratio;
  //   }
  // }

  return out;
}

float _calc_ratio_lock(float fm, float f0, uint8_t ratio_lock) {
  if (ratio_lock) {
    float ratio = f0 / fm;
    ratio = ceilf(ratio);
    return f0 / ratio;
  } else {
    return fm;
  }
}

void _calc_mod_depth(float f0, float mod_depth, uint8_t frequency_couple, float *low, float *high) {
  static const float breakpoint = 0.85f;
  static const float inv_breakpoint = 1.0f / breakpoint;
  static const float breakpoint_top_scale = 9.0f / (1.0f - breakpoint);
  float depth_mult = 1.0f;

  if (frequency_couple) {
    float lf = f0 * 0.001;
    depth_mult = lf * powf(mod_depth, 2.7) * 10.0f;
  } else {
    if (mod_depth < breakpoint) {
      depth_mult = mod_depth * inv_breakpoint;
    } else {
      depth_mult = (mod_depth - 0.75) * breakpoint_top_scale + 1.0f;
    }
  }

  *low = -1.0f * depth_mult;
  *high = depth_mult;
}

float _calc_density(float f0, float density_ratio, float mod_depth) {
  mod_depth = mod_depth * mod_depth;
  float mod_depth_scale_factor = linear_scale(mod_depth, 0.0f, 1.0f, 4.0f, 40.0f);
  float frequency_low_scale_factor = linear_scale(
    f0,
    65.0f,
    3136.0f,
    1.2f,
    mod_depth_scale_factor
  );

  float frequency_high_scale_factor = linear_scale(
    f0,
    65.0f,
    3136.0f,
    0.2f,
    3.5f
  );

  float density = linear_scale(
    density_ratio,
    -1.0f,
    1.0f,
    0.0f,
    1.0f
  );
  density = powf(density, 1.5f);
  density = linear_scale(density, 0.0f, 1.0f, frequency_low_scale_factor, frequency_high_scale_factor);

  return density;
}

float _calc_grain_length(float f0, float density_ratio, float mod_depth, float modulated_density, float sample_rate)
{
  float density = _calc_density(f0, density_ratio, mod_depth);
  float grain_length_base = powf(1.15f, density_ratio * 10.0f);
  grain_length_base = (f0 < 1) ? 1 : sample_rate / (grain_length_base * f0);
  float grain_length_scale = density * (modulated_density + 1.0f);
  return grain_length_base * grain_length_scale;
}

float _calc_bandwidth(float density_ratio) {
  float bandwidth = density_ratio * 0.5 + 0.5;
  bandwidth = bandwidth * bandwidth;
  bandwidth = linear_scale(bandwidth, 0.0f, 1.0f, 1.0, 0.05);
  return linear_scale(bandwidth, 0.0f, 1.0f, 3.0f, 20.0f);
}

void _trigger_pulsaret(ps_t *self, float pulse_frequency, float oscPhase)
{
  float depth_min, depth_max;
  _calc_mod_depth(pulse_frequency, self->mod_depth, self->frequency_couple, &depth_min, &depth_max);

  float minRatioCutoff = 0.05f;
  if (self->mod_ratio < minRatioCutoff) {
    depth_min *= self->mod_ratio / minRatioCutoff;
    depth_max *= self->mod_ratio / minRatioCutoff;
  }

  #ifdef ARM_MATH_CM7
  float lfo = arm_sin_f32(oscPhase * PI * 2);
  #else
  float lfo = sinf(oscPhase * PI * 2);
  #endif

  float modulated_density = linear_scale(
    lfo,
    -1.0f,
    1.0f,
    depth_min,
    depth_max
  );

  float grain_length = _calc_grain_length(
    pulse_frequency,
    self->density_ratio,
    self->mod_depth,
    modulated_density,
    self->sample_rate
  );

  float bandwidth = _calc_bandwidth(self->density_ratio);

  self->context.debug_value = grain_length;

  if (grain_length >= 1) {
    int16_t voice = _pulsar_add_voice(self, grain_length, bandwidth, self->waveform);
  }
}

void _reset_phase(ps_t *self) {
  phasor_set_phase(&self->phasor, 0);
  phasor_set_phase(&self->osc_phasor, 0.25);
}

// Initialize momjeans struct with default values
void pulsar_init(ps_t *self, float sample_rate) {
  phasor_init(&self->phasor);
  phasor_init(&self->osc_phasor);

  self->sample_rate = sample_rate;
  self->inv_sample_rate = 1.0 / sample_rate;
  self->last_input = 0;
  self->last_output = 0;
  self->last_ratio_lock = 0;
  self->last_ratio = 0;
  for (int i = 0; i < VOICE_MAX; i++) {
    self->voices[i].active = 0;
  }

  for (int i = 0; i < RAMPS_MAX; i++) {
    self->ramps[i].active = 0;
  }

  for (int i = 0; i < BLEP_MAX; i++) {
    self->bleps[i]._active = 0;
    minblep_zero(&self->bleps[i]);
    self->bleps[i]._minblepTable = minblep_alt_shift_lut;
  }

  self->oscillator_functions[0] = slow_sinc; // Sinc
  self->oscillator_functions[1] = blep_square;
  self->oscillator_functions[2] = blep_saw;
  self->oscillator_functions[3] = triangle;
  self->oscillator_functions[4] = contained_square;
  self->oscillator_functions[5] = raw_dirty_saw;
}

void pulsar_configure(
  ps_t *self,
  float pulse_frequency,
  float density_ratio,
  float mod_ratio,
  float mod_depth,
  float waveform,
  uint8_t ratio_lock,
  uint8_t frequency_couple
) {
  // Process FM
  float fm = _calc_mod_ratio(pulse_frequency, mod_ratio, frequency_couple, ratio_lock);

  fm = _calc_ratio_lock(fm, pulse_frequency, ratio_lock);

  if (self->last_ratio_lock != ratio_lock) {
    _reset_phase(self);
  }

  self->oscPhaseDelta = fm * self->inv_sample_rate;
  self->density_ratio = density_ratio;
  self->mod_ratio = mod_ratio;
  self->mod_depth = mod_depth;
  self->waveform = waveform;
  self->frequency_couple = frequency_couple;  

  self->last_ratio_lock = ratio_lock;
}

// Process function that mimics the operator() function
float pulsar_process(ps_t *self, float pulse_frequency, uint8_t resync, float *debug_value)
{
  // If resync is true, reset the phase of the phasor and osc_phasor
  uint8_t zerox = 0;
  if (resync) {
    _reset_phase(self);
    zerox = 1;
  }

  // Compute the phase of the LFO oscillator
  float oscPhase = phasor_step(&self->osc_phasor, self->oscPhaseDelta);
  self->osc_phase = oscPhase;
  
  // Detect zero crossing to trigger blip
  float lastPhase = self->phasor.id;
  float phaseDelta = (pulse_frequency / self->sample_rate);
  phasor_step(&self->phasor, phaseDelta);
  zerox |= phasor_getZeroCrossing(&self->phasor);
  self->sync_output = self->phasor.id < 0.5f ? 1.0f : 0.0f;

  float phasorValue = self->phasor.id;
  float nextOutput = 0.0f;
  if (phasorValue < 0.5) {
    nextOutput = 1.0f;
  } else {
    nextOutput = -1.0f;
  }

  float phaseScale = phaseDelta < 0.00001 ? 1.0f : (1.0f / phaseDelta);
  
  if (zerox) {
    _trigger_pulsaret(self, pulse_frequency, oscPhase);
  }

  float samp = 0;
  for (int i = 0; i < VOICE_MAX; i++) {
    samp += _pulsar_process_voice(self, &self->voices[i]);
  }

  for (int i = 0; i < BLEP_MAX; i++) {
    samp += _pulsar_process_blep(&self->bleps[i]);
  }
  
  for (int i = 0; i < RAMPS_MAX; i++) {
    samp += _pulsar_process_ramp(&self->ramps[i]);
  }

  float output = samp;
  output *= 0.5;

  output = tanhf(output); // Apply tanh to the output for saturation

  // tanh scaling
  // float knee = 0.75f;
  // float peak = fabsf(output);
  // float clip_amp = peak - knee;
  // if (clip_amp > 0) {
  //   float fade = clamp(clip_amp, 0.0, 0.25) * 4.0;
  //   float clipped_output = tanhf(clip_amp * 4.0f) * 0.25;
  //   output = knee + (1.0 - fade) * clip_amp + fade * clipped_output;
  // }

  return output;
}

float pulsar_get_debug_value(ps_t *self)
{
  return self->debug_value;
}

float pulsar_get_internal_lfo_phase(ps_t *self)
{
  // Return the current phase of the internal LFO oscillator
  return self->osc_phase;
}

float pulsar_get_sync_output(ps_t *self)
{
  // Return the current sync output value
  return self->sync_output;
}
