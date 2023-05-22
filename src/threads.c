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
#include "icp10125.h"
#include "veml7700.h"
#include "utils.h"
#include "threads.h"

#include "sql.h"

#define PACKET_TYPE_WEATHER                 0x0100
#define PACKET_TYPE_SLEEP                   0x0200
#define PACKET_TYPE_WATCHDOG                0x0300
#define PACKET_TYPE_PANIC                   0x0F00

#define PACKET_TYPE_UNKNOWN                 0x0000

const char * packetIDWeather    = "WP";
const char * packetIDSleep      = "SP";
const char * packetIDWatchdog   = "WD";
const char * packetIDPanic      = "PN";

const char *    dir_ordinal[16] = {
                    "ESE", "ENE", "E", "SSE", 
                    "SE", "SSW", "S", "NNE", 
                    "NE", "WSW", "SW", "NNW", 
                    "N", "NWN", "NW", "W"};

const uint16_t  dir_adc_min[16] = {
                    450, 593, 686, 832, 
                    1126, 1473, 1749, 2123, 
                    2496, 2838, 3120, 3269, 
                    3479, 3635, 3752, 3881};

/*
** Wind speed in mph:
**
** = anemometer diameter (m) * 
**   pi * 
**   anemometer_factor (1.18) * 
**   3600 (secs per hour) / 
**   (1609.34 (m per mile) * 
**   2 (pulses per revolution))
**
** = 0.18 * pi * 1.18 * 3600 / (1609.34 * 2)
*/
#define ANEMOMETER_MPH              0.74632688f

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

static uint16_t _getPacketType(char * packet) {
    uint16_t            packetType;

    if (packet[0] == packetIDWeather[0] && packet[1] == packetIDWeather[1]) {
        packetType = PACKET_TYPE_WEATHER;
    }
    else if (packet[0] == packetIDSleep[0] && packet[1] == packetIDSleep[1]) {
        packetType = PACKET_TYPE_SLEEP;
    }
    else if (packet[0] == packetIDWatchdog[0] && packet[1] == packetIDWatchdog[1]) {
        packetType = PACKET_TYPE_WATCHDOG;
    }
    else if (packet[0] == packetIDPanic[0] && packet[1] == packetIDPanic[1]) {
        packetType = PACKET_TYPE_PANIC;
    }
    else {
        packetType = PACKET_TYPE_UNKNOWN;
    }

    return packetType;
}

static void _transformWeatherPacket(weather_transform_t * target, weather_packet_t * source) {
    int         i;

    lgLogDebug(lgGetHandle(), "Raw battery volts: %u", (uint32_t)source->rawBatteryVolts);
    lgLogDebug(lgGetHandle(), "Raw battery percentage: %u", (uint32_t)source->rawBatteryPercentage);
    lgLogDebug(lgGetHandle(), "Raw battery temp: %u", (uint32_t)source->rawBatteryTemperature);

    lgLogDebug(lgGetHandle(), "Raw VSYS volts: %u", (uint32_t)source->rawVSYSVoltage);

    target->vsysVoltage = ((float)source->rawVSYSVoltage / 4096.0) * 3 * 3.3;

    target->batteryVoltage = (float)source->rawBatteryVolts / 1000.0;
    target->batteryPercentage = (float)source->rawBatteryPercentage / 10.0;    
    target->batteryTemperature = ((float)source->rawBatteryTemperature / 10) - 273.15;

    lgLogDebug(lgGetHandle(), "Raw temperature: %d", (int)source->rawTemperature);

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

    target->pressure = 
        (float)((float)icp10125_get_pressure(
                            source->rawICPTemperature, 
                            source->rawICPPressure) / 
                            100.0f);

    target->lux = computeLux(source->rawLux, true);

    lgLogDebug(lgGetHandle(), "Raw windspeed: %u", (uint32_t)source->rawWindspeed);
    lgLogDebug(lgGetHandle(), "Raw rainfall: %u", (uint32_t)source->rawRainfall);

    target->windspeed = (float)source->rawWindspeed * ANEMOMETER_MPH;
    target->rainfall = (float)source->rawRainfall * RAIN_GAUGE_MM;

    /*
    ** Find wind direction...
    */
    for (i = 0;i < 16;i++) {
        if (source->rawWindDir >= dir_adc_min[i]) {
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
    uint32_t            stationID;
    char                rxBuffer[64];
    uint16_t            packetType;
    weather_packet_t    pkt;
    sleep_packet_t      sleepPkt;
    watchdog_packet_t   wdPkt;
    weather_transform_t tr;
    que_item_t          qItem;

    nrf_p nrf = getNRFReference();

    lgLogInfo(lgGetHandle(), "Opening NRF24L01 device");

    NRF_init(nrf);

    NRF_set_local_address(nrf, nrf->local_address);
    NRF_set_remote_address(nrf, nrf->remote_address);

	rtn = NRF_read_register(nrf, NRF24L01_REG_CONFIG, rxBuffer, 1);

    if (rtn < 0) {
        lgLogError(lgGetHandle(), "Failed to transfer SPI data: %s\n", lguErrorText(rtn));

        return NULL;
    }

    lgLogInfo(lgGetHandle(), "Read back CONFIG reg: 0x%02X\n", (int)rxBuffer[0]);

    if (rxBuffer[0] == 0x00) {
        lgLogError(lgGetHandle(), "Config read back as 0x00, device is probably not plugged in?\n\n");
        return NULL;
    }

    stationID = cfgGetValueAsLongUnsigned(cfgGetHandle(), "radio.stationid");

    lgLogInfo(lgGetHandle(), "Read station ID from config as: 0x%08X", stationID);

    while (true) {
        while (NRF_data_ready(nrf)) {
            NRF_get_payload(nrf, rxBuffer);

            if (strHexDump(szDumpBuffer, 1024, rxBuffer, NRF_MAX_PAYLOAD) > 0) {
                lgLogDebug(lgGetHandle(), "%s", szDumpBuffer);
            }

            packetType = _getPacketType(rxBuffer);

            switch (packetType) {
                case PACKET_TYPE_WEATHER:
                    memcpy(&pkt, rxBuffer, sizeof(weather_packet_t));

                    if (pkt.chipID == stationID) {
                        _transformWeatherPacket(&tr, &pkt);

                        qItem.item = &tr;
                        qItem.itemLength = sizeof(weather_transform_t);

                        qPutItem(&dbq, qItem);

                        lgLogDebug(lgGetHandle(), "Got weather data:");
                        lgLogDebug(lgGetHandle(), "\tChipID:      0x%08X", pkt.chipID);
                        lgLogDebug(lgGetHandle(), "\tVSYS volts:  %.2f", tr.vsysVoltage);
                        lgLogDebug(lgGetHandle(), "\tBat. volts:  %.2f", tr.batteryVoltage);
                        lgLogDebug(lgGetHandle(), "\tBat. percent:%.2f", tr.batteryPercentage);
                        lgLogDebug(lgGetHandle(), "\tBat. temp:   %.2f", tr.batteryTemperature);
                        lgLogDebug(lgGetHandle(), "\tTemperature: %.2f", tr.temperature);
                        lgLogDebug(lgGetHandle(), "\tPressure:    %.2f", tr.pressure);
                        lgLogDebug(lgGetHandle(), "\tHumidity:    %d%%", (int)tr.humidity);
                        lgLogDebug(lgGetHandle(), "\tLux:         %.2f", tr.lux);
                        lgLogDebug(lgGetHandle(), "\tWind speed:  %.2f", tr.windspeed);
                        lgLogDebug(lgGetHandle(), "\tRainfall:    %.2f", tr.rainfall);
                    }
                    else {
                        lgLogStatus(lgGetHandle(), "Failed chipID check, got 0x%08X", pkt.chipID);
                    }
                    break;

                case PACKET_TYPE_SLEEP:
                    memcpy(&sleepPkt, rxBuffer, sizeof(sleep_packet_t));

                    if (sleepPkt.chipID == stationID) {
                        pkt.rawBatteryVolts = sleepPkt.rawBatteryVolts;
                        pkt.rawBatteryTemperature = sleepPkt.rawBatteryTemperature;
                        pkt.rawLux = sleepPkt.rawLux;

                        _transformWeatherPacket(&tr, &pkt);

                        lgLogStatus(lgGetHandle(), "Got sleep packet:");
                        lgLogStatus(lgGetHandle(), "\tChipID:      0x%08X", sleepPkt.chipID);
                        lgLogStatus(lgGetHandle(), "\tBat. volts:  %.2f", tr.batteryVoltage);
                        lgLogStatus(lgGetHandle(), "\tLux:         %.2f", tr.lux);
                        lgLogStatus(lgGetHandle(), "\tSleep for:   %d", (int)sleepPkt.sleepHours);
                    }
                    else {
                        lgLogStatus(lgGetHandle(), "Failed chipID check, got 0x%08X", sleepPkt.chipID);
                    }
                    break;

                case PACKET_TYPE_WATCHDOG:
                    memcpy(&wdPkt, rxBuffer, sizeof(watchdog_packet_t));

                    if (wdPkt.chipID == stationID) {
                        lgLogStatus(lgGetHandle(), "Got watchdog packet");
                    }
                    else {
                        lgLogStatus(lgGetHandle(), "Failed chipID check, got 0x%08X", wdPkt.chipID);
                    }
                    break;

                default:
                    lgLogError(lgGetHandle(), "Undefined packet type received: ID[0x%04X]('%c%c')", packetType, rxBuffer[0], rxBuffer[1]);
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
    hour = tmGetHour();

    lgLogDebug(lgGetHandle(), "Adding rainfall for hour %d", hour);

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
            cfgGetValue(cfgGetHandle(), "db.host"), 
            cfgGetValueAsInteger(cfgGetHandle(), "db.port"),
            cfgGetValue(cfgGetHandle(), "db.database"),
            cfgGetValue(cfgGetHandle(), "db.user"),
            cfgGetValue(cfgGetHandle(), "db.password"));

    if (wctlConnection == NULL) {
        lgLogError(lgGetHandle(), "Could not connect to database %s", cfgGetValue(cfgGetHandle(), "db.database"));
        return NULL;
    }

    while (true) {
        while (!qGetNumItems(&dbq)) {
            pxtSleep(milliseconds, 25);
        }

        if (qGetItem(&dbq, &item) != NULL) {
            tr = (weather_transform_t *)item.item;

            lgLogDebug(lgGetHandle(), "Updating summary structure");

            updateSummary(&ds, tr);

            hour = tmGetHour();
            minute = tmGetMinute();
            tmGetSimpleTimeStamp(timestamp, TIMESTAMP_STR_LEN);

            lgLogDebug(lgGetHandle(), "Inserting weather data");

            sprintf(
                szInsertStr,
                pszWeatherInsertStmt,
                timestamp,
                tr->temperature,
                tr->pressure,
                tr->humidity,
                tr->lux,
                tr->rainfall,
                tr->windspeed,
                tr->windDirection
            );

            dbExecute(wctlConnection, szInsertStr);

            pxtSleep(milliseconds, 100);

            lgLogDebug(lgGetHandle(), "Inserting telemetry data");

            sprintf(
                szInsertStr,
                pszTelemetryInsertStmt,
                timestamp,
                tr->batteryVoltage,
                tr->batteryTemperature
            );

            dbExecute(wctlConnection, szInsertStr);

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

                lgLogDebug(lgGetHandle(), "Inserting daily summary");

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
