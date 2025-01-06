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
 * @brief  This file contains the implementation of the SOGI-PLL class used in single phase inverters.
 *
 * @author Regis Ruelland <rruelland@laas.fr>
 * @author Luiz Villa <luiz.villa@laas.fr>
 */

#include "trigo.h"
#include "sogi_pll.h"

/**
 * @brief Constructor for the SogiPLL class.
 */
SogiPLL::SogiPLL() {
    // Initialize members if needed
}

/**
 * @brief Initializes the SOGI-PLL system.
 *
 * @param Vgrid Grid voltage.
 * @param w0 Angular frequency.
 * @param rise_time Desired rise time.
 * @param Ts Sampling time.
 */
void SogiPLL::init(float32_t Vgrid, float32_t w0, float32_t rise_time, float32_t Ts) {
    float32_t wn = 3.0F / rise_time;
    float32_t xsi = 0.7F;
    float32_t Kp = 2 * wn * xsi / Vgrid;
    float32_t Ki = (wn * wn) / Vgrid;
    _params.Ts = Ts;
    _params.w = w0;
    _params.w_ref = w0;
    float32_t Kr = 500.0;
    _params.theta = 0;
    _params.next_theta = 0;
    // sogi_init(_params.sogi_params, Kr, Ts);


    _params.sogi_params.Kr = Kr;
    _params.sogi_params.Ts = Ts;
    _params.sogi_params.den[0] = 0.0;
    _params.sogi_params.den[1] = 0.0;
    _params.sogi_params.den[2] = 0.0;
    _params.sogi_params.num[0] = 0.0;
    _params.sogi_params.num[1] = 0.0;

    _params.pi_params.Ts = Ts;
    _params.pi_params.Td = 0.0;
    _params.pi_params.N = 1;
    _params.pi_params.Ti = Kp / Ki;
    _params.pi_params.Kp = Kp;
    _params.pi_params.lower_bound = -10.0F * w0;
    _params.pi_params.upper_bound = 10.0F * w0;
    _params.pi.init(_params.pi_params);
    _params.pi.reset(_params.w);
}

/**
 * @brief Performs the SOGI-PLL calculation.
 *
 * @param v_grid Grid voltage.
 */
void SogiPLL::calculate(float32_t v_grid) {
    _params.theta = _params.next_theta;
    _params.Vab = _sogi_calc(v_grid);
    _params.Vdq = Transform::rotation_to_dqo(_params.Vab, _params.theta);
    _params.w = _params.w_ref + _params.pi.calculateWithReturn(0, -1.0*_params.Vdq.q);
    _params.next_theta = ot_modulo_2pi(_params.theta + _params.Ts * _params.w);
}

/**
 * @brief Initializes the SOGI parameters.
 *
 * @param params Reference to the SOGI parameters structure.
 * @param Kr Resonance gain.
 * @param Ts Sampling time.
 */
void SogiPLL::sogi_init(SogiParams &params, float32_t Kr, float32_t Ts) {
    params.Kr = Kr;
    params.Ts = Ts;
    params.den[0] = 0.0;
    params.den[1] = 0.0;
    params.den[2] = 0.0;
    params.num[0] = 0.0;
    params.num[1] = 0.0;
}


/**
 * @brief Performs SOGI calculation.
 *
 * @param input Input signal.
 * @param w0 Angular frequency.
 * @param params Reference to the SOGI parameters structure.
 * @return Clarke transformation result of the input signal.
 */
clarke_t SogiPLL::sogi_calc(float32_t input, float32_t w0, SogiParams &params) {
    float32_t coswt = 1.0F - 0.5F * (w0 * params.Ts) * (w0 * params.Ts);
    float32_t inv_sinwt = 1.0F / (w0 * params.Ts);
    clarke_t result;
    params.num[0] = params.Kr * (input - params.den[0]);
    params.den[0] = params.Ts * (params.num[0] - coswt * params.num[1]) + 2.0F * coswt * params.den[1] - params.den[2];
    result.alpha = params.den[0];
    result.beta = -coswt * inv_sinwt * params.den[0] + inv_sinwt * params.den[1];
    result.o = 0.0;
    params.num[1] = params.num[0];
    params.den[2] = params.den[1];
    params.den[1] = params.den[0];
    return result;
}


/**
 * @brief Performs SOGI calculation.
 *
 * @param input Input signal.
 * @param w0 Angular frequency.
 * @param params Reference to the SOGI parameters structure.
 * @return Clarke transformation result of the input signal.
 */
clarke_t SogiPLL::_sogi_calc(float32_t input) {
    float32_t coswt = 1.0F - 0.5F * (_params.w * _params.sogi_params.Ts) * (_params.w * _params.sogi_params.Ts);
    float32_t inv_sinwt = 1.0F / (_params.w * _params.sogi_params.Ts);
    clarke_t result;
    _params.sogi_params.num[0] = _params.sogi_params.Kr * (input - _params.sogi_params.den[0]);
    _params.sogi_params.den[0] = _params.sogi_params.Ts * (_params.sogi_params.num[0] - coswt * _params.sogi_params.num[1]) + 2.0F * coswt * _params.sogi_params.den[1] - _params.sogi_params.den[2];
    result.alpha = _params.sogi_params.den[0];
    result.beta = -coswt * inv_sinwt * _params.sogi_params.den[0] + inv_sinwt * _params.sogi_params.den[1];
    result.o = 0.0;
    _params.sogi_params.num[1] = _params.sogi_params.num[0];
    _params.sogi_params.den[2] = _params.sogi_params.den[1];
    _params.sogi_params.den[1] = _params.sogi_params.den[0];
    return result;
}
