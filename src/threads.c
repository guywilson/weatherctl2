#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <lgpio.h>
#include <postgresql/libpq-fe.h>

#include "NRF24.h"
#include "nRF24L01.h"
#include "logger.h"
#include "cfgmgr.h"
#include "posixthread.h"
#include "timeutils.h"
#include "psql.h"
#include "que.h"
#include "ltr390.h"
#include "utils.h"
#include "packet.h"
#include "threads.h"

#include "sql.h"

const char *    dir_ordinal[16] = {
                    "ESE", "ENE", "E", "SSE", 
                    "SE", "SSW", "S", "NNE", 
                    "NE", "WSW", "SW", "NNW", 
                    "N", "NWN", "NW", "W"};

const uint16_t  dir_adc_max[16] = {
                    63,   82,   91,  128,
                   196,  274,  336,  538,
                   651, 1013, 1113, 1392,
                  1808, 2071, 2542, 3149};

/*
** Wind speed in mph:
*/
#define ANEMOMETER_MPH              0.0052658575613333f

#define HPA_ALITUDE_COMPENSATION    0.103448276f

/*
** Each tip of the bucket in the rain gauge equates
** to 0.2794mm of rainfall, so just multiply this
** by the pulse count to get mm/hr...
*/
#define RAIN_GAUGE_MM               0.2794f

static que_handle_t             dbq;
static pxt_handle_t             nrfListenThread;
static pxt_handle_t             dbUpdateThread;

static char                     szDumpBuffer[1024];

static uint8_t _getPacketType(char * packet) {
    return (uint8_t)packet[0];
}

static uint16_t _getExpectedChipID(void) {
    uint16_t            chipID;

    chipID = ((cfgGetValueAsLongUnsigned("radio.stationid") >> 12) & 0xFFFF);

    return chipID;
}

static void _transformWeatherPacket(weather_transform_t * target, weather_packet_t * source) {
    int         i;
    float       anemometerFactor;
    float       altitudeCompensation;

    lgLogDebug("Raw battery volts: %u", (uint32_t)source->rawBatteryVolts);
    lgLogDebug("Raw battery percentage: %u", (uint32_t)source->rawBatteryPercentage);

    target->batteryVoltage = (float)source->rawBatteryVolts / 1000.0;
    target->batteryPercentage = (float)source->rawBatteryPercentage / 10.0;    

    target->status_bits = (int32_t)(source->status & 0x0000FFFF);

    lgLogDebug("Raw temperature: %d", (int)source->rawTemperature);

    /*
    ** TMP117 temperature
    */
    target->temperature = (float)source->rawTemperature * 0.0078125;
    
    /*
    ** SHT4x temperature & humidity
    */
    target->humidity = -6.0f + ((float)source->rawHumidity * 0.0019074);

    if (target->humidity < 0.0) {
        target->humidity = 0.0;
    }
    else if (target->humidity > 100.0) {
        target->humidity = 100.0;
    }

    lgLogDebug("Raw ICP Pressure: %u", source->rawICPPressure);

    altitudeCompensation = 
                    strtof(
                        cfgGetValue(
                            "calibration.altitude"), 
                        NULL) * 
                    HPA_ALITUDE_COMPENSATION;

    lgLogDebug("Barometer altitude offset: %2f", altitudeCompensation);
    
    target->pressure = 
        ((float)source->rawICPPressure / 100.0f) + 
        altitudeCompensation;

    target->lux = computeLux(source->rawALS_UV);
    target->uvIndex = computeUVI(source->rawALS_UV);

    lgLogDebug("Raw windspeed: %u", (uint32_t)source->rawWindspeed);
    lgLogDebug("Raw rainfall: %u", (uint32_t)source->rawRainfall);

    anemometerFactor = strtof(cfgGetValue("calibration.anemometerfactor"), NULL);

    target->windspeed = (float)source->rawWindspeed * ANEMOMETER_MPH * anemometerFactor;
    target->gustSpeed = (float)source->rawWindGust * ANEMOMETER_MPH * anemometerFactor;
    target->rainfall = (float)source->rawRainfall * RAIN_GAUGE_MM;

    target->windDirection = "SSE";

    /*
    ** Find wind direction...
    */
    for (i = 0;i < 16;i++) {
        if (source->rawWindDir < dir_adc_max[i]) {
            target->windDirection = dir_ordinal[i];
            break;
        }
    }
}

void startThreads(void) {
    qInit(&dbq, 10U);

    pxtCreate(&nrfListenThread, &NRF_listen_thread, false);
    pxtStart(&nrfListenThread, NULL);

    pxtCreate(&dbUpdateThread, &db_update_thread, true);
    pxtStart(&dbUpdateThread, NULL);
}

void stopThreads(void) {
    pxtStop(&dbUpdateThread);
    pxtStop(&nrfListenThread);

    qDestroy(&dbq);
}

void * NRF_listen_thread(void * pParms) {
    int                 rtn;
    uint16_t            stationID;
    char                rxBuffer[64];
    uint8_t             packetID;
    weather_packet_t    pkt;
    sleep_packet_t      sleepPkt;
    watchdog_packet_t   wdPkt;
    weather_transform_t tr;
    que_item_t          qItem;

    nrf_p nrf = getNRFReference();

    lgLogInfo("Opening NRF24L01 device");

    NRF_init(nrf);

    NRF_set_local_address(nrf, nrf->local_address);
    NRF_set_remote_address(nrf, nrf->remote_address);

	rtn = NRF_read_register(nrf, NRF24L01_REG_CONFIG, rxBuffer, 1);

    if (rtn < 0) {
        lgLogError("Failed to transfer SPI data: %s\n", lguErrorText(rtn));

        return NULL;
    }

    lgLogInfo("Read back CONFIG reg: 0x%02X\n", (int)rxBuffer[0]);

    if (rxBuffer[0] == 0x00) {
        lgLogError("Config read back as 0x00, device is probably not plugged in?\n\n");
        return NULL;
    }

    stationID = _getExpectedChipID();

    lgLogInfo("Read station ID from config as: 0x%08X", stationID);

    while (true) {
        while (NRF_data_ready(nrf)) {
            NRF_get_payload(nrf, rxBuffer);

            if (strHexDump(szDumpBuffer, 1024, rxBuffer, NRF_MAX_PAYLOAD) > 0) {
                lgLogDebug("%s", szDumpBuffer);
            }

            packetID = _getPacketType(rxBuffer);

            switch (packetID) {
                case PACKET_ID_WEATHER:
                    memcpy(&pkt, rxBuffer, sizeof(weather_packet_t));

                    _transformWeatherPacket(&tr, &pkt);

                    qItem.item = &tr;
                    qItem.itemLength = sizeof(weather_transform_t);

                    qPutItem(&dbq, qItem);

                    lgLogDebug("Got weather data:");
                    lgLogDebug("\tStatus:      0x%04X", pkt.status);
                    lgLogDebug("\tBat. volts:  %.2f", tr.batteryVoltage);
                    lgLogDebug("\tBat. percent:%.2f", tr.batteryPercentage);
                    lgLogDebug("\tTemperature: %.2f", tr.temperature);
                    lgLogDebug("\tPressure:    %.2f", tr.pressure);
                    lgLogDebug("\tHumidity:    %d%%", (int)tr.humidity);
                    lgLogDebug("\tLux:         %.2f", tr.lux);
                    lgLogDebug("\tUV Index:    %.1f", tr.uvIndex);
                    lgLogDebug("\tWind speed:  %.2f", tr.windspeed);
                    lgLogDebug("\tWind gust:   %.2f", tr.gustSpeed);
                    lgLogDebug("\tRainfall:    %.2f", tr.rainfall);
                    break;

                case PACKET_ID_SLEEP:
                    memcpy(&sleepPkt, rxBuffer, sizeof(sleep_packet_t));

                    pkt.rawBatteryVolts = sleepPkt.rawBatteryVolts;

                    memcpy(pkt.rawALS_UV, sleepPkt.rawALS_UV, 5);

                    _transformWeatherPacket(&tr, &pkt);

                    lgLogStatus("Got sleep packet:");
                    lgLogStatus("\tStatus:      0x%08X", sleepPkt.status);
                    lgLogStatus("\tBat. volts:  %.2f", tr.batteryVoltage);
                    lgLogStatus("\tLux:         %.2f", tr.lux);
                    lgLogStatus("\tSleep for:   %d", (int)sleepPkt.sleepHours);
                    break;

                case PACKET_ID_WATCHDOG:
                    memcpy(&wdPkt, rxBuffer, sizeof(watchdog_packet_t));

                    lgLogStatus("Got watchdog packet");
                    break;

                default:
                    lgLogError("Undefined packet type received: ID[0x%02X]", packetID);
                    break;
            }

            pxtSleep(milliseconds, 250);
        }

        pxtSleep(seconds, 2);
    }

    return NULL;
}

void updateSummary(daily_summary_t * ds, weather_transform_t * tr) {
    int         hour;

    if (tr->temperature > ds->max_temperature) {
        ds->max_temperature = tr->temperature;
    }
    if (tr->temperature < ds->min_temperature || (ds->min_temperature == 0.00 && tr->temperature > 0.00)) {
        ds->min_temperature = tr->temperature;
    }

    if (tr->pressure > ds->max_pressure) {
        ds->max_pressure = tr->pressure;
    }
    if (tr->pressure < ds->min_pressure || (ds->min_pressure == 0.00 && tr->pressure > 0.00)) {
        ds->min_pressure = tr->pressure;
    }

    if (tr->humidity > ds->max_humidity) {
        ds->max_humidity = tr->humidity;
    }
    if (tr->humidity < ds->min_humidity || (ds->min_humidity == 0.00 && tr->humidity > 0.00)) {
        ds->min_humidity = tr->humidity;
    }

    if (tr->lux > ds->max_lux) {
        ds->max_lux = tr->lux;
    }

    if (tr->windspeed > ds->max_wind_speed) {
        ds->max_wind_speed = tr->windspeed;
    }

    /*
    ** When calculating the total rainfall for the day,
    ** we only want to count the rainfall reading (mm/h)
    ** once per hour...
    */
    tmUpdate();
    hour = tmGetHour();

    lgLogDebug("Adding rainfall for hour %d", hour);

    if (!(ds->isHourAccountedBitmap & (1 << hour))) {
        ds->total_rainfall += tr->rainfall;

        ds->isHourAccountedBitmap |= (1 << hour);
    }
}

void * db_update_thread(void * pParms) {
    PGconn *                wctlConnection;
    que_item_t              item;
    weather_transform_t *   tr;
    daily_summary_t         ds;
    bool                    isSummaryDone = false;
    int                     hour;
    int                     minute;
    char                    szInsertStr[512];
    char                    timestamp[TIMESTAMP_STR_LEN];

    memset(&ds, 0, sizeof(daily_summary_t));

    wctlConnection = dbConnect(
            cfgGetValue("db.host"), 
            cfgGetValueAsInteger("db.port"),
            cfgGetValue("db.database"),
            cfgGetValue("db.user"),
            cfgGetValue("db.password"));

    if (wctlConnection == NULL) {
        lgLogError("Could not connect to database %s", cfgGetValue("db.database"));
        return NULL;
    }

    while (true) {
        while (!qGetNumItems(&dbq)) {
            pxtSleep(milliseconds, 25);
        }

        if (qGetItem(&dbq, &item) != NULL) {
            tr = (weather_transform_t *)item.item;

            lgLogDebug("Updating summary structure");

            updateSummary(&ds, tr);

            tmGetSimpleTimeStamp(timestamp, TIMESTAMP_STR_LEN);

            lgLogDebug("Inserting weather data");

            sprintf(
                szInsertStr,
                pszWeatherInsertStmt,
                timestamp,
                tr->temperature,
                tr->pressure,
                tr->humidity,
                tr->lux,
                tr->uvIndex,
                tr->rainfall,
                tr->windspeed,
                tr->gustSpeed,
                tr->windDirection
            );

            dbExecute(wctlConnection, szInsertStr);

            pxtSleep(milliseconds, 100);

            lgLogDebug("Inserting telemetry data");

            sprintf(
                szInsertStr,
                pszTelemetryInsertStmt,
                timestamp,
                tr->batteryVoltage,
                tr->batteryPercentage,
                tr->status_bits
            );

            dbExecute(wctlConnection, szInsertStr);

            tmUpdate();
            
            hour = tmGetHour();
            minute = tmGetMinute();

            /*
            ** On the stroke of midnight, write the daily_summary
            ** and reset the summary values...
            */
            if (hour == 23 && minute == 59 && !isSummaryDone) {
                /*
                ** We only want the date in the format
                ** "YYYY-MM-DD" so truncate the timestamp
                ** to just leave the date part...
                */
                timestamp[10] = 0;

                pxtSleep(milliseconds, 100);

                lgLogDebug("Inserting daily summary");

                sprintf(
                    szInsertStr,
                    pszSummaryInsertStmt,
                    timestamp,
                    ds.min_temperature,
                    ds.max_temperature,
                    ds.min_pressure,
                    ds.max_pressure,
                    ds.min_humidity,
                    ds.max_humidity,
                    ds.max_lux,
                    ds.total_rainfall,
                    ds.max_wind_speed,
                    ds.max_wind_gust
                );

                dbExecute(wctlConnection, szInsertStr);

                memset(&ds, 0, sizeof(daily_summary_t));

                isSummaryDone = true;
            }
            else if (hour == 1) {
                /*
                ** Once we've got to 1am, reset isSummaryDone flag...
                */
                isSummaryDone = false;
            }

            pxtSleep(milliseconds, 250);
        }
    }

    dbFinish(wctlConnection);

    return NULL;
}
