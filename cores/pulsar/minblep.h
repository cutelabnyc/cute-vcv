#include "minblep_lut.h"
#include "pulsar_lut.h"

#ifdef   __cplusplus
extern "C"
{
#endif

#pragma once

#define MINBLEP_OVERSAMPLING 32
#define MINBLEP_ZERO_CROSSINGS 32

// Maintains the minBLEP buffer. The included 2049 sample minBLEP 
// is 32 zerocrossings, 32 oversampling + 1 padding.
// Probably much bigger than necessary.
typedef struct minblep_t {
    const float *_minblepTable;
    int _pos;
    float _buf[MINBLEP_ZERO_CROSSINGS * 2];
    uint8_t _active;  // Is this minBLEP currently running?
} minblep_t;

void minblep_zero(minblep_t *self);
void minblep_insertDiscontinuity(minblep_t *self, float phase, float amp);
float minblep_process(minblep_t *self);

typedef struct cute_minblep_t {
    const float *blep_table;  // Pre-computed MinBLEP table
    uint32_t table_size;          // Size of BLEP table
    float phase;              // Current position in BLEP table
    float phase_increment;    // How fast we step through table
    float amplitude;          // Height of the discontinuity
    uint8_t active;                  // Is this BLEP currently running?
} cute_minblep_t;

void beginBlepCompensation(
    cute_minblep_t *self,
    const float *blep_table,
    uint32_t table_size,
    float oversampling,
    float amplitude,
    float subsample_offset
);

float stepBlep(
    cute_minblep_t *self
);

#ifdef   __cplusplus
}
#endif
