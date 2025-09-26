#include "math.h"

#pragma once

#ifdef __cplusplus
extern "C"
{   
#endif

typedef struct biquad {
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
} biquad_t;

void biquad_clear(biquad_t *self);
void biquad_set_coefficients(biquad_t *self, float fc, float fs, float Q);
float biquad_process(biquad_t *self, float x);

#ifdef __cplusplus
}
#endif
