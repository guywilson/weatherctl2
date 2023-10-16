#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <lgpio.h>
#include <postgresql/libpq-fe.h>
#include <curl/curl.h>

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

const float     dir_degrees[16] = {
                    112.5,  67.5,  90.0, 157.5,
                    135.0, 202.5, 180.0,  22.5,
                     45.0, 247.5, 225.0, 337.5,
                      0.0, 292.5, 315.0, 270.0
};

const uint16_t  dir_adc_max[16] = {
                    63,   82,   91,  128,
                   196,  274,  336,  538,
                   651, 1013, 1113, 1392,
                  1808, 2071, 2542, 3149};

/*
** Wind speed in mph:
*/
#define ANEMOMETER_MPH              0.0052658575613333f
#define ANEMOMETER_METRES_PER_SEC   0.0565486677646163f

#define ALITUDE_COMP_FACTOR         0.0000225577f
#define ALTITUDE_COMP_POWER         5.25588f

#define TEMPERATURE_CELCIUS_FACTOR  0.0078125f
#define HUMIDITY_RH_FACTOR          0.0019074f

#define HPA_TO_INHG                 0.02952998057228486f

#define TIME_BUFFER_SIZE            24

/*
** Each tip of the bucket in the rain gauge equates
** to 0.2794mm of rainfall, so just multiply this
** by the pulse count to get mm/hr...
*/
#define RAIN_GAUGE_MM               0.2794f

static que_handle_t             dbq;
static que_handle_t             webPostQueue;
static pxt_handle_t             nrfListenThread;
static pxt_handle_t             dbUpdateThread;
static pxt_handle_t             wowPostThread;

static char                     szDumpBuffer[1024];

typedef struct {
    char *      response;
    size_t      length;
}
curl_chunk_t;

static uint8_t _getPacketType(char * packet) {
    return (uint8_t)packet[0];
}

static uint16_t _getExpectedChipID(void) {
    uint16_t            chipID;

    chipID = ((cfgGetValueAsLongUnsigned("radio.stationid") >> 12) & 0xFFFF);

    return chipID;
}

static float _getAltitudeAdjustedPressure(uint32_t rawPressure) {
    static bool         isCalculated = false;
    static double       compensationFactor;
    double              altitude;
    float               adjustedPressure;

    if (!isCalculated) {
        altitude = strtod(cfgGetValue("calibration.altitude"), NULL);

        compensationFactor = pow(
                    ((double)1.0f - ((double)ALITUDE_COMP_FACTOR * altitude)), 
                    (double)ALTITUDE_COMP_POWER) *
                    (double)100.0f;

        isCalculated = true;
    }

    lgLogDebug("Altitude compensation factor: %.2f", compensationFactor);

    adjustedPressure = (float)((double)rawPressure / compensationFactor);

    return adjustedPressure;
}

static float inline _computeTemperature(int16_t rawTemperature) {
    return (float)rawTemperature * TEMPERATURE_CELCIUS_FACTOR;
}

static float inline _computeHumidity(uint16_t rawHumidity) {
    return (-6.0f + ((float)rawHumidity * HUMIDITY_RH_FACTOR));
}

static float _computeDewPoint(uint16_t rawTemperature, uint16_t rawHumidity) {
    float           dewPoint;
    float           lnHumidity;
    float           temperature;

    lnHumidity = (float)log((double)_computeHumidity(rawHumidity) / (double)100.0f);
    temperature = _computeTemperature(rawTemperature);

    dewPoint = 
        243.04f * 
        (lnHumidity + 
        ((17.625f * temperature) / 
        (243.04f + temperature))) / 
        (17.625f - lnHumidity -
        ((17.625f * temperature) / 
        (243.04f + temperature)));

    return dewPoint;
}

static void _transformWeatherPacket(weather_transform_t * target, weather_packet_t * source) {
    int         i;
    float       anemometerFactor;

    lgLogDebug("Raw battery volts: %u", (uint32_t)source->rawBatteryVolts);
    lgLogDebug("Raw battery percentage: %u", (uint32_t)source->rawBatteryPercentage);

    target->batteryVoltage = (float)source->rawBatteryVolts / 1000.0;
    target->batteryPercentage = (float)source->rawBatteryPercentage / 10.0;    

    target->status_bits = (int32_t)(source->status & 0x0000FFFF);

    lgLogDebug("Raw temperature: %d", (int)source->rawTemperature);

    /*
    ** TMP117 temperature
    */
    target->temperature = _computeTemperature(source->rawTemperature);
    
    /*
    ** SHT4x temperature & humidity
    */
    target->humidity = _computeHumidity(source->rawHumidity);;

    if (target->humidity < 0.0) {
        target->humidity = 0.0;
    }
    else if (target->humidity > 100.0) {
        target->humidity = 100.0;
    }

    target->dewPoint = _computeDewPoint(source->rawTemperature, source->rawHumidity);

    lgLogDebug("Raw ICP Pressure: %u", source->rawICPPressure);

    target->normalisedPressure = _getAltitudeAdjustedPressure(source->rawICPPressure);
    target->actualPressure = (float)source->rawICPPressure / 100.0f;

    target->lux = computeLux(source->rawALS_UV);
    target->uvIndex = computeUVI(source->rawALS_UV);

    lgLogDebug("Raw windspeed: %u", (uint32_t)source->rawWindspeed);
    lgLogDebug("Raw rainfall: %u", (uint32_t)source->rawRainfall);

    anemometerFactor = strtof(cfgGetValue("calibration.anemometerfactor"), NULL);

    target->windspeed = 
                (float)source->rawWindspeed * 
                ANEMOMETER_MPH * 
                anemometerFactor;
    target->gustSpeed = 
                (float)source->rawWindGust * 
                ANEMOMETER_MPH * 
                anemometerFactor;

    target->windSpeedms = 
                (float)source->rawWindspeed * 
                ANEMOMETER_METRES_PER_SEC * 
                anemometerFactor;
    target->gustSpeedms = 
                (float)source->rawWindGust * 
                ANEMOMETER_METRES_PER_SEC * 
                anemometerFactor;

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

static void updateSummary(daily_summary_t * ds, weather_transform_t * tr) {
    if (tr->temperature > ds->max_temperature) {
        ds->max_temperature = tr->temperature;
    }
    if (tr->temperature < ds->min_temperature || (ds->min_temperature == 0.00 && tr->temperature > 0.00)) {
        ds->min_temperature = tr->temperature;
    }

    if (tr->normalisedPressure > ds->max_pressure) {
        ds->max_pressure = tr->normalisedPressure;
    }
    if (tr->normalisedPressure < ds->min_pressure || (ds->min_pressure == 0.00 && tr->normalisedPressure > 0.00)) {
        ds->min_pressure = tr->normalisedPressure;
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

    if (tr->gustSpeed > ds->max_wind_gust) {
        ds->max_wind_gust = tr->gustSpeed;
    }

    ds->total_rainfall += tr->rainfall;
}

static void * NRF_listen_thread(void * pParms) {
    int                 rtn;
    uint16_t            stationID;
    char                rxBuffer[64];
    uint8_t             packetID;
    weather_packet_t    pkt;
    sleep_packet_t      sleepPkt;
    watchdog_packet_t   wdPkt;
    weather_transform_t tr;
    que_item_t          qItem;
    que_item_t          webPostItem;
    int                 msgCounter = 0;
    int                 postCycleSeconds;

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

    postCycleSeconds = cfgGetValueAsInteger("wow.postcycletime");

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

                    msgCounter += 30;

                    if (msgCounter == postCycleSeconds) {
                        webPostItem.item = &tr;
                        webPostItem.itemLength = sizeof(weather_transform_t);

                        qPutItem(&webPostQueue, webPostItem);

                        msgCounter = 0;
                    }

                    qItem.item = &tr;
                    qItem.itemLength = sizeof(weather_transform_t);

                    qPutItem(&dbq, qItem);

                    lgLogDebug("Got weather data:");
                    lgLogDebug("\tStatus:      0x%04X", pkt.status);
                    lgLogDebug("\tBat. volts:  %.2f", tr.batteryVoltage);
                    lgLogDebug("\tBat. percent:%.2f", tr.batteryPercentage);
                    lgLogDebug("\tTemperature: %.2f", tr.temperature);
                    lgLogDebug("\tDew point:   %.2f", tr.dewPoint);
                    lgLogDebug("\tAdj pressure:%.2f", tr.normalisedPressure);
                    lgLogDebug("\tAct pressure:%.2f", tr.actualPressure);
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

static void * db_update_thread(void * pParms) {
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
                tr->dewPoint,
                tr->actualPressure,
                tr->normalisedPressure,
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

static char * getEncodedTimeStamp(void) {
    struct tm *         utc;
	time_t				t;
    char *              pszTimeBuffer;

    pszTimeBuffer = (char *)malloc(TIME_BUFFER_SIZE);

	t = time(NULL);
	utc = gmtime(&t);

    snprintf(
        pszTimeBuffer,
        TIME_BUFFER_SIZE,
        "%d-%02d-%02d+%02d%%3A%02d%%3A%02d",
        utc->tm_year + 1900,
        utc->tm_mon + 1,
        utc->tm_mday,
        utc->tm_hour,
        utc->tm_min,
        utc->tm_sec);

	return pszTimeBuffer;
}

static float getWindDir(const char * windOrdinal) {
    int         i;

    for (i = 0;i < 16;i++) {
        if (strcmp(dir_ordinal[i], windOrdinal) == 0) {
            return dir_degrees[i];
        }
    }

    return 0.0;
} 

size_t CurlWrite_CallbackFunc(void * contents, size_t size, size_t nmemb, void * p)
{
    curl_chunk_t *      pChunk = (curl_chunk_t *)p;
    size_t              newLength =  size * nmemb;
    char *              s;

    s = (char *)realloc(pChunk->response, (pChunk->length + newLength + 1));

    if (s == NULL) {
        return 0;
    }

    pChunk->response = s;

    memcpy(&pChunk->response[pChunk->length], contents, newLength);
    pChunk->length += newLength;

    pChunk->response[pChunk->length] = 0;

    return newLength;
}

static void * wow_post_thread(void * pParms) {
    float                   tempF;
    float                   dewPointF;
    float                   pressureInHg;
    float                   windDegrees;
    que_item_t              item;
    weather_transform_t *   tr;
    CURL *                  pCurl;
	CURLcode			    result;
    char                    szCurlError[CURL_ERROR_SIZE];
    char                    szURL[1024];
    curl_chunk_t            chunk;
    char *                  encodedDate;
    const char *            baseURL;
    const char *            siteID;
    const char *            authKey;
    const char *            softwareType;

    chunk.length = 0;
    chunk.response = NULL;

    while (true) {
        pCurl = curl_easy_init();

        if (pCurl == NULL) {
            curl_easy_cleanup(pCurl);
            lgLogError("Failed to initialise curl");
            break;
        }

        curl_easy_setopt(pCurl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
        curl_easy_setopt(pCurl, CURLOPT_ERRORBUFFER, szCurlError);

        baseURL = cfgGetValue("wow.baseurl");
        siteID = cfgGetValue("wow.siteid");
        authKey = cfgGetValue("wow.authkey");
        softwareType = cfgGetValue("wow.softwareid");

        lgLogDebug("Base URL: %s", baseURL);
        lgLogDebug("Site ID: %s", siteID);
        lgLogDebug("Software ID: %s", softwareType);

        while (!qGetNumItems(&webPostQueue)) {
            pxtSleep(milliseconds, 25);
        }

        if (qGetItem(&webPostQueue, &item) != NULL) {
            tr = (weather_transform_t *)item.item;

            encodedDate = getEncodedTimeStamp();

            tempF = (tr->temperature * 1.8) + 32;
            dewPointF = (tr->dewPoint * 1.8) + 32;
            pressureInHg = (tr->normalisedPressure) * HPA_TO_INHG;

            lgLogDebug("Preparing to POST to WoW service");

            windDegrees = getWindDir(tr->windDirection);

            sprintf(
                szURL, 
                "%s?siteid=%s&siteAuthenticationKey=%s&dateutc=%s&softwaretype=%s",
                baseURL,
                siteID,
                authKey,
                encodedDate,
                softwareType);

            free(encodedDate);

            sprintf(
                &szURL[strlen(szURL)],
                "&tempf=%.2f&baromin=%.2f&humidity=%.2f&dewptf=%.2f&windspeedmph=%.2f&windgustmph=%.2f&winddir=%.1f",
                tempF,
                pressureInHg,
                tr->humidity,
                dewPointF,
                tr->windspeed,
                tr->gustSpeed,
                windDegrees);

            lgLogInfo("Posting to URL: %s", szURL);

            if (cfgGetValueAsBoolean("wow.isenabled")) {
                curl_easy_setopt(pCurl, CURLOPT_URL, szURL);

                curl_easy_setopt(pCurl, CURLOPT_HTTPGET, 1L);
                curl_easy_setopt(pCurl, CURLOPT_USERAGENT, "libcrp/0.1");
                curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &CurlWrite_CallbackFunc);
                curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &chunk);

                result = curl_easy_perform(pCurl);

                if (result != CURLE_OK) {
                    lgLogError("Failed to post to %s - Curl error [%s]", szURL, szCurlError);
                    return NULL;
                }

                lgLogInfo("WoW service responded: %s", chunk.response);
            }
            else {
                lgLogInfo("Posting disbaled by config - do nothing");
            }
        }

        pxtSleep(milliseconds, 250);
    }

    return NULL;
}

void startThreads(void) {
    qInit(&dbq, 10U);
    qInit(&webPostQueue, 10U);

    pxtCreate(&nrfListenThread, &NRF_listen_thread, false);
    pxtStart(&nrfListenThread, NULL);

    pxtCreate(&dbUpdateThread, &db_update_thread, true);
    pxtStart(&dbUpdateThread, NULL);

    pxtCreate(&wowPostThread, &wow_post_thread, true);
    pxtStart(&wowPostThread, NULL);
}

void stopThreads(void) {
    pxtStop(&dbUpdateThread);
    pxtStop(&nrfListenThread);
    pxtStop(&wowPostThread);

    qDestroy(&webPostQueue);
    qDestroy(&dbq);
}
