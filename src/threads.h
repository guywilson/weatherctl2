#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_THREADS
#define __INCL_THREADS

#pragma pack(push, 1)
typedef struct {                                    // Offset-Description
    char                packetID;                   // Identify this as a weather packet

    uint32_t            chipID;                     // 0x00 - ID of the RP2040

    uint16_t            rawWindDir;                 // 0x04 - Raw ADC wind direction
    uint16_t            rawBatteryVolts;            // 0x06 - Raw ADC value for battery V
    uint16_t            rawBatteryTemperature;      // 0x08 - Raw ADC value for battery temp
    uint16_t            rawChipTemperature;         // 0x0A - Raw ADC value of RP2040 temp

    int16_t             rawTemperature;             // 0x0C - Raw I2C TMP117 value
    uint32_t            rawICPPressure;             // 0x0E - Raw pressure from icp10125
    uint16_t            rawICPTemperature;          // 0x12 - Raw temperature from icp10125
    uint16_t            rawHumidity;                // 0x14 - Raw I2C SHT4x value
    uint16_t            rawLux;                     // 0x16 - Raw I2C veml7700 lux value

    uint16_t            rawRainfall;                // 0x18 - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x1A - Raw anemometer count
}
weather_packet_t;
#pragma pack(pop)

typedef struct {
    float               batteryVoltage;
    float               chipTemperature;
    float               batteryTemperature;
    
    float               temperature;
    float               pressure;
    float               humidity;
    float               lux;
    float               rainfall;
    float               windspeed;
    const char *        windDirection;
}
weather_transform_t;

void        startThreads(void);
void        stopThreads(void);
void *      NRF_listen_thread(void * pParms);
void *      db_update_thread(void * pParms);

#endif
