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

#ifndef SINGLEPHASEINVERTER_H
#define SINGLEPHASEINVERTER_H

#include "sogi.h"
#include "transform.h"


class singlePhaseInverter {
public:
    // Constructor
    singlePhaseInverter();

    // Initialization function
    int8_t init(float32_t grid_Vpk, float32_t grid_w0, float32_t Ts);

    // Reset function
    void reset();

    // Calculate function
    void calculatePower(float32_t v, float32_t i);

    void calculatePll(float32_t v_meas);

    dqo_t getVdq();

    clarke_t getIab();

    clarke_t getVab();

    dqo_t getIdq();

    dqo_t getPower();

    float32_t getw();

    float32_t getTheta();


private:
    // Internal state variables
    clarke_t _Iab;
    clarke_t _Vab;
    dqo_t _Idq;
    dqo_t _Vdq;
    dqo_t _power;


    float32_t _grid_Vpk;
    float32_t _grid_w0;
    float32_t _Ts;

    Sogi sogi_i;
    Sogi sogi_v;
    PidParams pll_pi_params;    ///< PI controller parameters.
    Pid pll_pi;                 ///< PI controller.
    float32_t _theta;        ///< Current phase angle.
    float32_t _next_theta;   ///< Next phase angle.
    float32_t _w;            ///< Angular frequency.
    float32_t _w_ref;        ///< Reference angular frequency.


};

#endif // POWER_AC1PHASE_H
