#ifndef SOGI_PLL_H
#define SOGI_PLL_H

#include "pid.h"
#ifndef __arm__
#include "my_types.h"
#else
#include "arm_math.h"
#endif

#include "transform.h"

/**
 * @brief Structure containing parameters for the SOGI (Second-Order Generalized Integrator) filter.
 */
struct SogiParams {
    float32_t num[2]; ///< Numerator coefficients of the SOGI filter.
    float32_t den[3]; ///< Denominator coefficients of the SOGI filter.
    float32_t Ts;     ///< Sampling time.
    float32_t Kr;     ///< Resonance gain.
};

/**
 * @brief Structure containing parameters for the SOGI-PLL system.
 */
struct SogiPllParams {
    SogiParams sogi_params; ///< SOGI parameters.
    PidParams pi_params;    ///< PI controller parameters.
    Pid pi;                 ///< PI controller.
    float32_t Ts;           ///< Sampling time.
    dqo_t Vdq;              ///< DQ axis voltage.
    clarke_t Vab;           ///< Clarke transformation of voltage.
    float32_t theta;        ///< Current phase angle.
    float32_t next_theta;   ///< Next phase angle.
    float32_t w;            ///< Angular frequency.
    float32_t w_ref;        ///< Reference angular frequency.
};

/**
 * @brief Class implementing the SOGI-PLL (Second-Order Generalized Integrator Phase-Locked Loop) system.
 */
class SogiPLL {
public:
    /**
     * @brief Constructor for the SogiPLL class.
     */
    SogiPLL();

    /**
     * @brief Initializes the SOGI-PLL system.
     *
     * @param Vgrid Grid voltage.
     * @param w0 Angular frequency.
     * @param rise_time Desired rise time.
     * @param Ts Sampling time.
     */
    void init(float32_t Vgrid, float32_t w0, float32_t rise_time, float32_t Ts);

    /**
     * @brief Performs the SOGI-PLL calculation.
     *
     * @param v_grid Grid voltage.
     */
    void calculate(float32_t v_grid);

    SogiPllParams params; ///< Parameters for the SOGI-PLL system.

    /**
     * @brief Initializes the SOGI parameters.
     *
     * @param params Reference to the SOGI parameters structure.
     * @param Kr Resonance gain.
     * @param Ts Sampling time.
     */
    void sogi_init(SogiParams &params, float32_t Kr, float32_t Ts);

    /**
     * @brief Performs SOGI calculation.
     *
     * @param input Input signal.
     * @param w0 Angular frequency.
     * @param params Reference to the SOGI parameters structure.
     * @return Clarke transformation result of the input signal.
     */
    clarke_t sogi_calc(float32_t input, float32_t w0, SogiParams &params);

private:

};

#endif // SOGI_PLL_H