/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "../../cores/pizza-gate/pizza-gate.h"
#include "../../cores/phasor/phasor.h"
#include <stdexcept>
#include <stdio.h>

int main() {

    stdio_init_all();  // Initialize serial over USB

    pzg_t pzg;
    phasor_t phasor;
    float pizza_output, phasor_output, pulse_output;

    pizza_gate_init(&pzg, 1000);
    phasor_init(&phasor);

    while (true) {
        phasor_output = phasor_step(&phasor, (float)0.001);
        pulse_output = (phasor_output > 0.5 ? 1.0 : 0.0);

        pizza_output = pizza_gate_process(&pzg, 1, 0.25, 0.25, 0.25, 0.25, pulse_output, 1, 1, 1);
        system("clear");


        printf("Pulse out: %f          ", pulse_output);
        printf("Pizza out: %f\n", pizza_output);
        sleep_ms(1);
    }
}
