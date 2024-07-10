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


#include "singlePhaseInverter.h"

// Constructor for singlePhaseInverter
singlePhaseInverter::singlePhaseInverter() : _w(0.0F) {}

// Initialization function for singlePhaseInverter
int8_t singlePhaseInverter::init(float32_t grid_Vpk, float32_t grid_w0, float32_t Ts) {

    float32_t rise_time = 1.0F * 2.0F * PI / grid_w0;
    float32_t wn = 3.0F / rise_time;
    float32_t xsi = 0.7F;
    float32_t Kp = 2 * wn * xsi / grid_Vpk;
    float32_t Ki = (wn * wn) / grid_Vpk;
    float32_t Kr = 500.0;

    _grid_Vpk = grid_Vpk;
    _w = grid_w0;
    _w_ref = grid_w0;
    _Ts = Ts;
    _theta = 0;
    _next_theta = 0;

    sogi_v.init(Kr, _Ts);
    sogi_i.init(Kr, _Ts);

    pll_pi_params.Ts = Ts;
    pll_pi_params.Td = 0.0;
    pll_pi_params.N = 1;
    pll_pi_params.Ti = Kp / Ki;
    pll_pi_params.Kp = Kp;
    pll_pi_params.lower_bound = -10.0F * _w_ref;
    pll_pi_params.upper_bound = 10.0F * _w_ref;

    pll_pi.init(pll_pi_params);
    pll_pi.reset(grid_w0);


    return 0;  // Return 0 to indicate success
}


/**
 * @brief Performs a PLL using the SOGI of the voltage calculation.
 *
 * @param v_meas Measured grid voltage.
 */
void singlePhaseInverter::calculatePll(float32_t v_meas) {
    _theta = _next_theta;
    _Vab = sogi_v.calc(v_meas,_w);
    _Vdq = Transform::rotation_to_dqo(_Vab, _theta);
    _w = _w_ref + pll_pi.calculateWithReturn(0, -1.0*_Vdq.q);
    _next_theta = ot_modulo_2pi(_theta + _Ts * _w);
}


// Calculate function for singlePhaseInverter
void singlePhaseInverter::calculatePower(float32_t v_meas, float32_t i_meas) {

    // Perform SOGI-PLL calculation
    calculatePll(v_meas);

    // Perform SOGI calculation for current
    _Iab = sogi_i.calc(i_meas, _w);

    // Transform current from alpha-beta to d-q
    _Idq = Transform::rotation_to_dqo(_Iab, _theta);

    // Calculate active and reactive power
    _power.d = 0.5F * (_Vdq.d * _Idq.d + _Vdq.q * _Idq.q);
    _power.q = 0.5F * (_Idq.d * _Vdq.q - _Idq.q * _Vdq.d);
}

dqo_t singlePhaseInverter::getVdq(){
    return _Vdq;
}


dqo_t singlePhaseInverter::getIdq(){
    return _Idq;
}

dqo_t singlePhaseInverter::getPower(){
    return _power;
}

clarke_t singlePhaseInverter::getIab(){
    return _Iab;
}

clarke_t singlePhaseInverter::getVab(){
    return _Vab;
}


float32_t singlePhaseInverter::getTheta(){
    return _theta;
}


float32_t singlePhaseInverter::getw(){
    return _w;
}
