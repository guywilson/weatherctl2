#ifndef __INCL_ICP10125
#define __INCL_ICP10125

void        icp10125_init(void);
uint32_t    icp10125_get_pressure(uint16_t rawTemperature, uint32_t rawPressure);
#endif
