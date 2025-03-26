#include <stdint.h>

#ifndef __INCL_LTR390
#define __INCL_LTR390

float computeLux(uint8_t * ALS_UV);
float computeUVI(uint8_t * ALS_UV);

#endif
