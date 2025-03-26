#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "logger.h"

/*
** Common light levels
** -------------------
**
** Bright Summer Day:           100,000 Lux (~10,000 footcandles)
** Full Daylight:               10,000 Lux (~1,000 footcandles)
** Overcast Day:                1,000 Lux (~100 footcandles)
** Traditional Office Lighting: 300-500 Lux (30-50 footcandles)
** Common Stairway:             50-100 Lux (5-10 footcandles)
** Twilight:                    10 Lux (1 footcandle)
** Full Moon:                   <1 Lux (<0.1 footcandle)
*/

float computeLux(uint8_t * ALS_UV) {
    float           lux;
    uint32_t        rawALS = 0;

    rawALS = (uint32_t)ALS_UV[0] | ((uint32_t)ALS_UV[1] << 8) | (((uint32_t)ALS_UV[2] & 0x000F) << 16);

    lgLogDebug("Raw ALS: 0x%08X", rawALS);

    lux = ((float)rawALS * 0.6) / (18 * 4);

    return lux;
}

float computeUVI(uint8_t * ALS_UV) {
    float           uvi;
    uint32_t        rawUVS;

    rawUVS = (uint32_t)ALS_UV[3] | ((uint32_t)ALS_UV[4] << 8) | (((uint32_t)ALS_UV[5] & 0x000F) << 16);

    lgLogDebug("Raw UVS: 0x%08X", rawUVS);

    // self.uvs
    // / (
    //     (Gain.factor[self.gain] / 18)
    //     * (2 ** Resolution.factor[self.resolution])
    //     / (2**20)
    //     * 2300
    // )

    uvi = rawUVS / 2300;

    return uvi;
}
