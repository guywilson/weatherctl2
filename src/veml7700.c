#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define VEML7700_RESOLUTION                 0.0072f

float computeLux(uint16_t rawALS, bool corrected) {
    float           lux;
    
    lux = VEML7700_RESOLUTION * (float)rawALS;

    if (corrected) {
        lux = 
            (((6.0135e-13 * lux - 9.3924e-9) * lux + 8.1488e-5) * lux + 1.0023) * lux;
    }

    return lux;
}
