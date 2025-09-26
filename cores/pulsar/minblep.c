#include "minblep.h"
#include "math.h"
#include "stdint.h"

static float minblep_index_table[MINBLEP_ZERO_CROSSINGS * 2];
static float oversampling_table[MINBLEP_ZERO_CROSSINGS * 2];
static const int MINBLEP_SAMPLE_LEN = MINBLEP_ZERO_CROSSINGS * 2;

#define interpolateLinear(mbTable, x) (mbTable[(int)x] + (mbTable[(int)x + 1] - mbTable[(int)x]) * (x - (int)x))

static float fclampf(float x, float a, float b) {
    return fmaxf(fminf(x, b), a);
}

void minblep_zero(minblep_t *self)
{
    for (int i = 0; i < 2 * MINBLEP_ZERO_CROSSINGS; i++) {
        self->_buf[i] = 0.0f;
    }
}

void minblep_insertDiscontinuity(minblep_t *self, float phase, float amp)
{
    static const int mask = (2 * MINBLEP_ZERO_CROSSINGS) - 1;

    if (!(-1 < phase && phase <= 0)) {
        return;
    }

    float minBlepIndex = -phase * MINBLEP_OVERSAMPLING;
    float minBlepIncrement = MINBLEP_OVERSAMPLING;

    for (int j = 0; j < MINBLEP_SAMPLE_LEN; j++) {
        int index = (self->_pos + j) & mask;
        float blep = interpolateLinear(self->_minblepTable, minBlepIndex);
        self->_buf[index] += amp * blep;
        minBlepIndex += minBlepIncrement;
    }
}

float minblep_process(minblep_t *self)
{
    static const int mask = (2 * MINBLEP_ZERO_CROSSINGS) - 1;

    float v = self->_buf[self->_pos];
    self->_buf[self->_pos] = 0.0f;
    self->_pos = (self->_pos + 1) & mask;
    if (self->_pos == 0) {
        self->_active = 0; // Reset active state when we reach the end of the buffer
    }
    return v;
}
