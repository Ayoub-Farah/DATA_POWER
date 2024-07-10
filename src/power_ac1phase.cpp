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


#include "power_ac1phase.h"

// Constructor for PowerAC1Phase
PowerAC1Phase::PowerAC1Phase() : w0(0.0F) {}

// Initialization function for PowerAC1Phase
int8_t PowerAC1Phase::init(PowerAC1PhaseParams params) {
    float32_t rise_time = 1.0F * 2.0F * PI / params.w0;
    this->w0 = params.w0;

    // Initialize SOGI-PLL for voltage
    sogi_pll.init(params.grid_voltage, params.w0, rise_time, params.Ts);

    // Initialize SOGI for current
    sogi_i_params.Kr = 500;
    sogi_i_params.Ts = params.Ts;
    sogi_i_params.den[0] = sogi_i_params.den[1] = sogi_i_params.den[2] = 0.0F;
    sogi_i_params.num[0] = sogi_i_params.num[1] = 0.0F;

    return 0;  // Return 0 to indicate success
}

// Reset function for PowerAC1Phase
void PowerAC1Phase::reset() {
    sogi_i_params.den[0] = sogi_i_params.den[1] = sogi_i_params.den[2] = 0.0F;
    sogi_i_params.num[0] = sogi_i_params.num[1] = 0.0F;
}

// Calculate function for PowerAC1Phase
PowerAC1PhaseOutput PowerAC1Phase::calculate(float32_t v, float32_t i) {
    PowerAC1PhaseOutput power;
    // Perform SOGI-PLL calculation
    sogi_pll.calculate(v);

    // Perform SOGI calculation for current
    clarke_t Iab = sogi_pll.sogi_calc(i, w0, sogi_i_params);

    // Transform current from alpha-beta to d-q
    dqo_t Idq = Transform::rotation_to_dqo(Iab, sogi_pll.params.theta);

    // Save internal states
    _Iab = Iab;
    _Idq = Idq;
    _Vdq = sogi_pll.params.Vdq;
    _Vab = sogi_pll.params.Vab;
    _w = sogi_pll.params.w;

    // Calculate active and reactive power
    power.p = 0.5F * (_Vdq.d * _Idq.d + _Vdq.q * _Idq.q);
    power.q = 0.5F * (_Idq.d * _Vdq.q - _Idq.q * _Vdq.d);

    return power;
}

dqo_t PowerAC1Phase::getVdq(){
    return _Vdq;
}

clarke_t PowerAC1Phase::getIab(){
    return _Iab;
}

clarke_t PowerAC1Phase::getVab(){
    return _Vab;
}


dqo_t PowerAC1Phase::getIdq(){
    return _Idq;
}

float32_t PowerAC1Phase::getTheta(){
    return sogi_pll.params.theta;
}


float32_t PowerAC1Phase::getw(){
    return sogi_pll.params.w;
}