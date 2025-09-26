#include "biquad.h"
#include "math.h"

void biquad_clear(biquad_t *self) {
    self->x1 = 0.0f;
    self->x2 = 0.0f;
    self->y1 = 0.0f;
    self->y2 = 0.0f;
}

void biquad_set_coefficients(biquad_t *self, float fc, float fs, float Q) {
        // Normalize frequency
    float K = tanf(M_PI * fc / fs);  // K = tan(omega/2) where omega = 2 * pi * fc
    float norm = 1 / (1 + K / Q + K * K);  // Normalization factor for filter coefficients

    // Calculate coefficients
    self->b0 = K * K * norm;
    self->b1 = 2 * self->b0;
    self->b2 = self->b0;
    self->a1 = 2 * (K * K - 1) * norm;
    self->a2 = (1 - K / Q + K * K) * norm;
}

float biquad_process(biquad_t *self, float x) {
    float y = self->b0 * x + self->b1 * self->x1 + self->b2 * self->x2 - self->a1 * self->y1 - self->a2 * self->y2;
    self->x2 = self->x1;
    self->x1 = x;
    self->y2 = self->y1;
    self->y1 = y;
    return y;
}
