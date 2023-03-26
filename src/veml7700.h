#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_VEML7700
#define __INCL_VEML7700

float computeLux(uint16_t rawALS, bool corrected);

#endif
