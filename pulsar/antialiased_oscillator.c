#include "antialiased_oscillator.h"

#define FIXED_BANDWIDTH 11
#define SQUARE_BANDWIDTH 5

void osc_zero(ps_osc_t *self)
{
  self->output = 0.0f;
  self->count = -1;
  self->square_minblepper._pos = 0;
  minblep_zero(&self->square_minblepper);
  biquad_clear(&self->biquad);
}

void osc_initialize(ps_osc_t *self)
{
  osc_zero(self);
  self->square_minblepper._minblepTable = minblep_alt_shift_lut;

    // Set the special order for waveforms in each voice
    self->ordered_use[0] = &(self->use_sinc);
    self->ordered_use[1] = &(self->use_special_1);
    self->ordered_use[2] = &(self->use_tri);
    self->ordered_use[3] = &(self->use_rect);
    self->ordered_use[4] = &(self->use_saw);
    self->ordered_use[5] = &(self->use_special_3);
    self->ordered_use[6] = &(self->use_special_2);
    self->ordered_use[7] = &(self->use_filthy_1);
    self->ordered_use[8] = &(self->use_filthy_2);

    self->ordered_scale[0] = &(self->sinc_scale);
    self->ordered_scale[1] = &(self->special_1_scale);
    self->ordered_scale[2] = &(self->tri_scale);
    self->ordered_scale[3] = &(self->rect_scale);
    self->ordered_scale[4] = &(self->saw_scale);
    self->ordered_scale[5] = &(self->special_3_scale);
    self->ordered_scale[6] = &(self->special_2_scale);
    self->ordered_scale[7] = &(self->filthy_1_scale);
    self->ordered_scale[8] = &(self->filthy_2_scale);
}

void osc_configure(ps_osc_t *self, float waveform)
{
    for (int j = 0; j < 9; j++) {
        uint8_t *use = self->ordered_use[j];
        float *scale = self->ordered_scale[j];
        *use = (waveform >= j - 1.0 && waveform < j + 1.0);
        *scale = *use ? (1.0 - fabs(j - waveform)) : 0.0f;
    }
}

void osc_process(ps_osc_t *self, int32_t phase, int32_t phase_increment)
{
  static const int32_t square_width = (2147483648) / SQUARE_BANDWIDTH;
  static const float inv_square_width = 1.0f / square_width;
  static const float inv_saw_period = 1.0f / (square_width * 2);
  static const int half_square_period = square_width * (SQUARE_BANDWIDTH - 1) / 2;
  static const int half_saw_period = square_width * (SQUARE_BANDWIDTH - 1) / 2;
  static const int32_t filthy_square_width = (2147483648) / FIXED_BANDWIDTH;
  static const float filthy_inv_square_width = 1.0f / filthy_square_width;
  static const float filthy_inv_saw_period = 1.0f / (filthy_square_width * 2);

  float square_output = 0.0f;
  float saw_output = 0.0f;
  float tri_output = 0.0f;
  float special_1_output = 0.0f;
  float special_2_output = 0.0f;
  float special_3_output = 0.0f;
  float filthy_1_output = 0.0f;
  float filthy_2_output = 0.0f;

  int32_t count = phase * inv_square_width;
  square_output = (count & 1) ? 0.5 : -0.5;
  if (self->use_special_2) {
    special_2_output = phase > half_square_period ? square_output * 0.5 + 0.5 : square_output * 0.5 - 0.5;
  }
  float local_saw_phase = phase * inv_saw_period;
  float local_phase_floor = floorf(local_saw_phase);
  saw_output = (local_saw_phase - local_phase_floor) - 0.5f;
  if (self->use_special_3) {
    special_3_output = phase > half_saw_period ? saw_output * 0.5 + 0.5 : saw_output * 0.5 - 0.5;
  }

  tri_output = saw_output > 0.0f ? fabs(0.25 - saw_output) * -2.0 + 0.5 : fabs(-0.25 - saw_output) * 2.0 - 0.5;

  if (self->use_special_1) {
    special_1_output = phase < half_saw_period ? 0.5 - fabs(saw_output) : tri_output;
  }

  if (self->use_filthy_1) {
    int32_t count = phase * filthy_inv_square_width;
    filthy_1_output = (count & 1) ? 0.5 : -0.5;
  }

  if (self->use_filthy_2) {
    float local_saw_phase = phase * filthy_inv_saw_period;
    float local_phase_floor = floorf(local_saw_phase);
    filthy_2_output = (local_saw_phase - local_phase_floor) - 0.5f;
  }

  if (count >= SQUARE_BANDWIDTH - 1) {
    tri_output = saw_output = square_output = special_1_output = special_2_output = special_3_output = 0.0f;
    filthy_1_output = filthy_2_output = 0.0f;
  }

  float output = 0.0f;

  if (self->use_tri) {
    output += tri_output * self->tri_scale;
  }

  if (self->use_rect) {
    output += square_output * self->rect_scale;
  }

  if (self->use_saw) {
    output += saw_output * self->saw_scale;
  }

  if (self->use_special_1) {
    output += special_1_output * self->special_1_scale;
  }

  if (self->use_special_2) {
    output += special_2_output * self->special_2_scale;
  }

  if (self->use_special_3) {
    output += special_3_output * self->special_3_scale;
  }

  if (self->use_filthy_1) {
    output += filthy_1_output * self->filthy_1_scale;
  }

  if (self->use_filthy_2) {
    output += filthy_2_output * self->filthy_2_scale;
  }

  uint8_t needs_minblep = self->use_rect || self->use_saw || self->use_special_2 || self->use_special_3;
  if (needs_minblep && count != self->count) {
    float local_phase = phase - (count * square_width);
    local_phase /= (float) phase_increment;
    local_phase = 1.0f - local_phase;
    if (count == 0) local_phase = 1.0f;
    float discontinuity = output - self->output;
    minblep_insertDiscontinuity(&self->square_minblepper, local_phase - 1.0f, discontinuity);
  }

  self->count = count;
  self->output = minblep_process(&self->square_minblepper) + output;
}