#ifndef PHASOR_H
#define PHASOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct phasor {
  float id;
  uint8_t zero_x;
} phasor_t;

void phasor_init(phasor_t *self);
void phasor_reset(phasor_t *self);
uint8_t phasor_getZeroCrossing(phasor_t *self);
void phasor_set_phase(phasor_t *self, float id);

float phasor_step(phasor_t *self, float rate);

#ifdef __cplusplus
}
#endif

#endif // PHASOR_H
