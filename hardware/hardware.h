#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

typedef struct {
    uint16_t value; // Current value of the potentiometer
    uint16_t min;   // Minimum value
    uint16_t max;   // Maximum value
} potentiometer16u_t;

typedef struct {
    uint16_t value; // Current value of the jack
    uint16_t zero;  // Reading at zero position
    uint16_t hardware_bits;
} jack16u_t;

typedef struct {
    uint16_t value; // Current value of the switch
    uint16_t min;   // Minimum value
    uint16_t max;   // Maximum value
} switch16u_t;

static inline void zero_potentiometer(potentiometer16u_t *pot) {
    pot->value = 0x8000; // Set to halfway point
    pot->min = 0xFFFF; // Set to maximum value
    pot->max = 0x0000; // Set to minimum value
}

static inline void zero_jack(jack16u_t *jack, uint16_t hardware_bits) {
    jack->value = 0x8000; // Set to halfway point
    jack->zero = 0x8000; // Set to halfway point
    jack->hardware_bits = hardware_bits;
}

static inline void zero_switch(switch16u_t *sw) {
    sw->value = 0x8000; // Set to halfway point
    sw->min = 0xFFFF; // Set to maximum value
    sw->max = 0x0000; // Set to minimum value
}

/**
 * @brief Scale a jack value to the given range
 */
static inline float jack_to_float(
    uint16_t jack_value,
    jack16u_t *jack,
    float min_value,
    float max_value
) {
    // Jacks use the zero value to add an offset, but assume the full 0 to 0xFFFF range
    uint16_t range = 0xFFFF >> (16 - jack->hardware_bits);
    float normalized_value;
    if (range == 0) {
        return 0.0f; // Avoid division by zero
    }
    if (jack_value >= jack->zero) {
        normalized_value = (float)(jack_value - jack->zero) / (float)(range) + 0.5;
    } else {
        normalized_value = 0.5f - (float)(jack->zero - jack_value) / (float)(range);
    }
    if (normalized_value < 0.0f) {
        normalized_value = 0.0f; // Clamp to zero
    } else if (normalized_value > 1.0f) {
        normalized_value = 1.0f; // Clamp to one
    }
    return min_value + normalized_value * (max_value - min_value);
}

static inline float pot_to_float(
    uint16_t pot_value,
    potentiometer16u_t *pot,
    float min_value,
    float max_value
) {
    float pot_range = (float)(pot->max - pot->min);
    float pot_normalized = pot_range > 0 ? (float)(pot_value - pot->min) / pot_range : 0.0f;
    return min_value + pot_normalized * (max_value - min_value);
}

static inline float pot_and_jack_to_float(
    uint16_t pot_value,
    uint16_t jack_value,
    potentiometer16u_t *pot,
    jack16u_t *jack,
    float min_value,
    float max_value
) {
    float pot_range = (float)(pot->max - pot->min);
    float jack_normalized = jack_to_float(jack_value, jack, 1.0f, -1.0f);
    float pot_normalized = pot_range > 0 ? (float)(pot_value - pot->min) / pot_range : 0.0f;
    float normalized_value = (pot_normalized + jack_normalized);
    if (normalized_value < 0.0f) {
        normalized_value = 0.0f; // Clamp to zero
    } else if (normalized_value > 1.0f) {
        normalized_value = 1.0f; // Clamp to one
    }
    return min_value + (normalized_value) * (max_value - min_value);
}

static inline uint8_t switch_to_uint8(
    uint16_t switch_value,
    switch16u_t *sw
) {
    float sw_range = (float)(sw->max - sw->min);
    float sw_normalized = sw_range > 0 ? (float)(switch_value - sw->min) / sw_range : 0.0f;
    return (uint8_t)(sw_normalized > 0.7f ? 1 : 0); // Threshold at 70% of the range
}

#endif  // HARDWARE_H