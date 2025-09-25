#ifndef PULSAR_HARDWARE_H
#define PULSAR_HARDWARE_H

#include "../hardware/hardware.h"

#define MOM_JEANS_POT_COUNT 5
#define MOM_JEANS_JACK_COUNT 8
#define MOM_JEANS_SWITCH_COUNT 2

typedef struct mom_jeans_calibration {
    uint32_t total_readings; // Total number of readings taken for calibration
    uint32_t reading_at_plausible_complete_calibration; // Reading at which calibration is considered plausible

    potentiometer16u_t pitch_pot;
    potentiometer16u_t density_pot; 
    potentiometer16u_t cadence_pot;
    potentiometer16u_t torque_pot;
    potentiometer16u_t waveform_pot;

    jack16u_t density_jack;
    jack16u_t cadence_jack;
    jack16u_t torque_jack;
    jack16u_t waveform_jack;
    jack16u_t voct_jack;
    jack16u_t linear_fm_jack;
    jack16u_t fm_index_jack;
    jack16u_t sync_jack;

    switch16u_t coupling_switch;
    switch16u_t quant_switch;
} mom_jeans_calibration_t;

typedef struct mom_jeans_reading {
    uint16_t pitch_pot;
    uint16_t density_pot;
    uint16_t cadence_pot;
    uint16_t torque_pot;
    uint16_t waveform_pot;

    uint16_t density_jack;
    uint16_t cadence_jack;
    uint16_t torque_jack;
    uint16_t waveform_jack;
    uint16_t voct_jack;
    uint16_t linear_fm_jack;
    uint16_t fm_index_jack;
    uint16_t sync_jack;

    uint16_t coupling_switch;
    uint16_t quant_switch;
} mom_jeans_reading_t;

void mom_jeans_zero_calibration(mom_jeans_calibration_t *calibration);

void mom_jeans_append_calibration_reading(
    mom_jeans_reading_t *reading,
    mom_jeans_calibration_t *calibration
);

uint8_t mom_jeans_is_calibrated(
    mom_jeans_calibration_t *calibration
);

#endif  // PULSAR_HARDWARE_H