#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define VEML7700_MAX_RESOLUTION             0.0036f
#define VEML7700_MAX_IT                     800.0f
#define VEML7700_MAX_GAIN                   2.0f

/*
** These are the parameters we've set on the sensor
** attached to the Pico...
*/
#define VEML7700_SELECTED_IT                100.0f
#define VEML7700_SELECTED_GAIN              0.125f

static float getResolution(void) {
  return (
        VEML7700_MAX_RESOLUTION * 
        (VEML7700_MAX_IT / VEML7700_SELECTED_IT) * 
        (VEML7700_MAX_GAIN / VEML7700_SELECTED_GAIN));
}

float computeLux(uint16_t rawALS, bool corrected) {
    float           lux;
    
    lux = getResolution() * (float)rawALS;

    if (corrected) {
        lux = 
            (((6.0135e-13 * lux - 9.3924e-9) * lux + 8.1488e-5) * lux + 1.0023) * lux;
    }

    return lux;
}
