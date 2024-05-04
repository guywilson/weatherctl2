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

    uint8_t             packetNum[3];               // 0x01 - Packet number (24-bit)

    uint8_t             status;                     // 0x04 - Status bits

    uint8_t             rawBatteryPercentage;       // 0x05 - Raw I2C value for battery %
    uint16_t            rawBatteryChargeRate;       // 0x06 - Raw I2C battery charge rate
    uint16_t            rawBatteryVolts;            // 0x08 - Raw I2C value for battery V

    uint16_t            rawWindDir;                 // 0x0A - Raw ADC wind direction

    int16_t             rawTemperature;             // 0x0C - Raw I2C TMP117 value
    uint32_t            rawICPPressure;             // 0x0E - Raw pressure from icp10125
    uint16_t            rawHumidity;                // 0x12 - Raw I2C SHT4x value
    uint8_t             rawALS_UV[6];               // 0x14 - Raw I2C LTR390 ALS & UV value

    uint16_t            rawRainfall;                // 0x1A - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x1C - Raw wind speed
    uint16_t            rawWindGust;                // 0x1E - Raw wind gust speed
}
weather_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint8_t             reserved;                   // 0x01 - Reserved

    uint16_t            status;                     // 0x02 - Status bits

    uint16_t            sleepHours;                 // 0x04 - How many hours is the weather station sleeping for?
    uint16_t            rawBatteryVolts;            // 0x06 - The last raw I2C value for battery V
    uint16_t            rawBatteryPercentage;       // 0x08 - The last raw I2C value for batttery percentage
    uint8_t             rawALS_UV[6];               // 0x0A - The last raw light level

    uint8_t             padding[16];
}
sleep_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint8_t             reserved;                   // 0x01 - Reserved

    uint16_t            status;                     // 0x02 - Status bits

    uint8_t             padding[28];
}
watchdog_packet_t;
#pragma pack(pop)

typedef struct {
    uint32_t            packetNum;
    
    float               batteryVoltage;
    float               batteryPercentage;
    float               batteryChargeRate;
    float               batteryTemperature;
    int32_t             status_bits;

    float               temperature;
    float               dewPoint;
    float               actualPressure;
    float               normalisedPressure;
    float               humidity;
    float               lux;
    float               uvIndex;
    float               rainfall;
    float               windspeed;
    float               gustSpeed;
    float               windSpeedms;
    float               gustSpeedms;
    const char *        windDirection;
}
weather_transform_t;

#endif
