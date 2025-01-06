/*
 *
 * Copyright (c) 2021-2024 LAAS-CNRS
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 2.1 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

/**
 * @brief  This file is a power control class for single phase inverters
 *
 * @author Regis Ruelland <rruelland@laas.fr>
 * @author Luiz Villa <luiz.villa@laas.fr>
 */

#ifndef POWER_AC1PHASE_H
#define POWER_AC1PHASE_H

#include "sogi_pll.h"
#include "transform.h"

// Define a structure to hold the power output
struct PowerAC1PhaseOutput {
    float32_t p; // Active power
    float32_t q; // Reactive power
};

// Define a structure to hold the parameters for initialization
struct PowerAC1PhaseParams {
    float32_t grid_voltage;
    float32_t w0;
    float32_t Ts;
};

class PowerAC1Phase {
public:
    // Constructor
    PowerAC1Phase();

    // Initialization function
    int8_t init(PowerAC1PhaseParams params);

    // Reset function
    void reset();

    // Calculate function
    PowerAC1PhaseOutput calculate(float32_t v, float32_t i);

    dqo_t getVdq();

    clarke_t getIab();

    clarke_t getVab();

    dqo_t getIdq();

    float32_t getw();

    float32_t getTheta();

private:
    // Internal state variables
    float32_t _w0;
    float32_t _w;
    SogiParams sogi_i_params;
    clarke_t _Iab;
    clarke_t _Vab;
    dqo_t _Idq;
    dqo_t _Vdq;
    SogiPLL sogi_pll;

};

#endif // POWER_AC1PHASE_H
