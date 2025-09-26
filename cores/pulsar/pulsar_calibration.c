#include "pulsar_calibration.h"
#include <math.h>

static void _add_potentiometer_calibration_reading(
    potentiometer16u_t *potentiometer,
    uint16_t pot_value
) {
    potentiometer->value = pot_value;
    if (potentiometer->min > pot_value) {
        potentiometer->min = pot_value;
    }
    if (potentiometer->max < pot_value) {
        potentiometer->max = pot_value;
    }
}

static void _add_jack_calibration_reading(
    jack16u_t *jack,
    uint16_t jack_value
) {
    jack->value = jack_value;

    // For the zero, we take a weighted average of the current value and the previous zero value
    jack->zero = (jack->zero * 0.9) + (jack_value * 0.1);
}

static void _add_switch_calibration_reading(
    switch16u_t *sw,
    uint16_t sw_value
) {
    sw->value = sw_value;
    if (sw->min > sw_value) {
        sw->min = sw_value;
    }
    if (sw->max < sw_value) {
        sw->max = sw_value;
    }
}

uint8_t _is_calibration_plausible(mom_jeans_calibration_t *calibration) {
    // A reading is plausible if all of the pots have a reasonable range,
    // and all the switches have a reasonable range
    
    for (int i = 0; i < MOM_JEANS_POT_COUNT; i++) {
        potentiometer16u_t *pot = (potentiometer16u_t *)((uint8_t *)&calibration->pitch_pot + i * sizeof(potentiometer16u_t));
        if ((pot->max < pot->min) || (pot->max - pot->min < 500)) {
            return 0;
        }
    }

    for (int i = 0; i < MOM_JEANS_SWITCH_COUNT; i++) {
        switch16u_t *sw = (switch16u_t *)((uint8_t *)&calibration->coupling_switch + i * sizeof(switch16u_t));
        if ((sw->max < sw->min) || (sw->max - sw->min < 500)) {
            return 0;
        }
    }

    return 1;
}

void mom_jeans_zero_calibration(mom_jeans_calibration_t *calibration) {
    calibration->total_readings = 0;
    calibration->reading_at_plausible_complete_calibration = 0;

    zero_potentiometer(&calibration->pitch_pot);
    zero_potentiometer(&calibration->density_pot);
    zero_potentiometer(&calibration->cadence_pot);
    zero_potentiometer(&calibration->torque_pot);
    zero_potentiometer(&calibration->waveform_pot);

    zero_jack(&calibration->density_jack, 16);
    zero_jack(&calibration->cadence_jack, 16);
    zero_jack(&calibration->torque_jack, 16);
    zero_jack(&calibration->waveform_jack, 16);
    zero_jack(&calibration->voct_jack, 16);
    zero_jack(&calibration->linear_fm_jack, 12);
    zero_jack(&calibration->fm_index_jack, 16);
    zero_jack(&calibration->sync_jack, 12);

    zero_switch(&calibration->coupling_switch);
    zero_switch(&calibration->quant_switch);
}

void mom_jeans_append_calibration_reading(
    mom_jeans_reading_t *reading,
    mom_jeans_calibration_t *calibration
) {
    // First handle the potentiometers
    for (int i = 0; i < MOM_JEANS_POT_COUNT; i++) {
        uint16_t pot_value = 0;
        switch (i) {
            case 0: pot_value = reading->pitch_pot; break;
            case 1: pot_value = reading->density_pot; break;
            case 2: pot_value = reading->cadence_pot; break;
            case 3: pot_value = reading->torque_pot; break;
            case 4: pot_value = reading->waveform_pot; break;
        }
        _add_potentiometer_calibration_reading(
            &calibration->pitch_pot + i,
            pot_value
        );
    }

    // Now handle the jacks
    for (int i = 0; i < MOM_JEANS_JACK_COUNT; i++) {
        uint16_t jack_value = 0;
        switch (i) {
            case 0: jack_value = reading->density_jack; break;
            case 1: jack_value = reading->cadence_jack; break;
            case 2: jack_value = reading->torque_jack; break;
            case 3: jack_value = reading->waveform_jack; break;
            case 4: jack_value = reading->voct_jack; break;
            case 5: jack_value = reading->linear_fm_jack; break;
            case 6: jack_value = reading->fm_index_jack; break;
            case 7: jack_value = reading->sync_jack; break;
        }
        _add_jack_calibration_reading(
            &calibration->density_jack + i,
            jack_value
        );
    }

    // Now handle the switches
    for (int i = 0; i < MOM_JEANS_SWITCH_COUNT; i++) {
        uint16_t sw_value = 0;
        switch (i) {
            case 0: sw_value = reading->coupling_switch; break;
            case 1: sw_value = reading->quant_switch; break;
        }
        _add_switch_calibration_reading(
            &calibration->coupling_switch + i,
            sw_value
        );
    }

    calibration->total_readings++;
    if (calibration->reading_at_plausible_complete_calibration == 0) {
        if (_is_calibration_plausible(calibration)) {
            calibration->reading_at_plausible_complete_calibration = calibration->total_readings;
        }
    }
}

uint8_t mom_jeans_is_calibrated(mom_jeans_calibration_t *calibration)
{
    uint8_t hasPlausibleCalibration = calibration->reading_at_plausible_complete_calibration > 0;
    uint8_t hasEnoughReadings = calibration->total_readings - calibration->reading_at_plausible_complete_calibration > 10;
    return hasPlausibleCalibration && hasEnoughReadings;
}
