#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "logger.h"
#include "icp10125.h"

typedef struct {
    float       sensor_constants[4]; // OTP values
    float       p_Pa_calib[3];
    float       LUT_lower;
    float       LUT_upper;
    float       quadr_factor;
    float       offst_factor;

    uint16_t    rawTemperature;
    uint32_t    rawPressure;

    float       LUT_values[3];

    float       A;
    float       B;
    float       C;
}
inv_invpres_t;

inv_invpres_t               icpConfigParms;

/*
** These have been previously read from our ICP10125 pressure sensor,
** other sensors will have different calibration values...
*/
const uint16_t otpValues[4] = {0x013D, 0x013D, 0x013D, 0x013D};

void icp10125_calculate_conversion_constants(inv_invpres_t * s) { 
    s->C = 
        (s->LUT_values[0] * s->LUT_values[1] * (s->p_Pa_calib[0] - s->p_Pa_calib[1]) + 
        s->LUT_values[1] * s->LUT_values[2] * (s->p_Pa_calib[1] - s->p_Pa_calib[2]) + 
        s->LUT_values[2] * s->LUT_values[0] * (s->p_Pa_calib[2] - s->p_Pa_calib[0])) / 
        (s->LUT_values[2] * (s->p_Pa_calib[0] - s->p_Pa_calib[1]) + 
        s->LUT_values[0] * (s->p_Pa_calib[1] - s->p_Pa_calib[2]) + 
        s->LUT_values[1] * (s->p_Pa_calib[2] - s->p_Pa_calib[0])); 

    s->A = 
        (s->p_Pa_calib[0] * 
            s->LUT_values[0] - 
            s->p_Pa_calib[1] * 
            s->LUT_values[1] - 
                (s->p_Pa_calib[1] - 
                    s->p_Pa_calib[0]) * 
                    s->C) / 
                (s->LUT_values[0] - 
            s->LUT_values[1]); 

    s->B = 
        (s->p_Pa_calib[0] - 
        s->A) * 
        (s->LUT_values[0] + 
        s->C); 
}

void icp10125_init() {
    int                 i;
    inv_invpres_t *     s = &icpConfigParms;

    s->p_Pa_calib[0] = 45000.0f; 
    s->p_Pa_calib[1] = 80000.0f; 
    s->p_Pa_calib[2] = 105000.0f; 
    s->LUT_lower = 3670016.0f; 
    s->LUT_upper = 12058624.0f; 
    s->quadr_factor = 1.0f / 16777216.0f; 
    s->offst_factor = 2048.0f;

    for (i = 0;i < 4;i++) {
        s->sensor_constants[i] = (float)otpValues[i];
    }
}

uint32_t icp10125_get_pressure(uint16_t rawTemperature, uint32_t rawPressure) {
    uint32_t            pressurePa;
    float               t;
    inv_invpres_t *     s = &icpConfigParms;

    t = (float)(rawTemperature - 32768);

    s->LUT_values[0] = s->LUT_lower + (float)(s->sensor_constants[0] * t * t) * s->quadr_factor; 
    s->LUT_values[1] = s->offst_factor * s->sensor_constants[3] + (float)(s->sensor_constants[1] * t * t) * s->quadr_factor; 
    s->LUT_values[2] = s->LUT_upper + (float)(s->sensor_constants[2] * t * t) * s->quadr_factor; 

    icp10125_calculate_conversion_constants(s);

    pressurePa = (uint32_t)(s->A + s->B / (s->C + rawPressure));

    return pressurePa;
}
