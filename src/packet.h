#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_PACKET
#define __INCL_PACKET

#define PACKET_ID_WEATHER                           0x55
#define PACKET_ID_SLEEP                             0xAA
#define PACKET_ID_WATCHDOG                          0x96

#define PACKET_ID_UNKNOWN                           0x00

#pragma pack(push, 1)
typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint16_t            status;                     // 0x01 - Status bits

    uint16_t            rawWindDir;                 // 0x03 - Raw ADC wind direction

    int16_t             rawTemperature;             // 0x05 - Raw I2C TMP117 value
    uint16_t            rawICPPressure;             // 0x07 - Raw pressure from icp10125
    uint16_t            rawHumidity;                // 0x09 - Raw I2C SHT4x value
    uint8_t             rawALS_UV[5];               // 0x0B - Raw I2C LTR390 ALS & UV value
    uint16_t            rawBatteryVolts;            // 0x10 - Raw I2C value for battery V
    uint16_t            rawBatteryPercentage;       // 0x12 - Raw I2C value for battery %

    uint16_t            rawRainfall;                // 0x14 - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x16 - Raw wind speed
    uint16_t            rawWindGust;                // 0x18 - Raw wind gust speed

    uint8_t             padding[6];
}
weather_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint16_t            status;                     // 0x01 - Status bits

    uint16_t            sleepHours;                 // 0x03 - How many hours is the weather station sleeping for?
    uint16_t            rawBatteryVolts;            // 0x05 - The last raw I2C value for battery V
    uint16_t            rawBatteryPercentage;       // 0x07 - The last raw I2C value for batttery percentage
    uint8_t             rawALS_UV[5];               // 0x09 - The last raw light level

    uint8_t             padding[18];
}
sleep_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint16_t            status;                     // 0x01 - Status bits

    uint8_t             padding[29];
}
watchdog_packet_t;
#pragma pack(pop)

typedef struct {
    float               batteryVoltage;
    float               batteryPercentage;
    float               batteryTemperature;
    int32_t             status_bits;

    float               temperature;
    float               pressure;
    float               humidity;
    float               lux;
    float               uvIndex;
    float               rainfall;
    float               windspeed;
    float               gustSpeed;
    const char *        windDirection;
}
weather_transform_t;

#endif
