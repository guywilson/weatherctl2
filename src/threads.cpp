#include <string>
#include <queue>

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
#include "psql.h"
#include "utils.h"
#include "packet.h"
#include "threads.h"

#include "sql.h"

using namespace std;

#define INSERT_STRING_LEN           512
#define URL_STRING_LEN             1024

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

static queue<weather_transform_t> dbq;
static queue<weather_transform_t> webPostQueue;

static char szDumpBuffer[1024];

typedef struct {
    char *      response;
    size_t      length;
}
curl_chunk_t;

static uint8_t _getPacketType(char * packet) {
    return (uint8_t)packet[0];
}

static uint16_t _getExpectedChipID(void) {
    cfgmgr & cfg = cfgmgr::getInstance();

    uint16_t chipID = ((cfg.getValueAsLongUnsignedInteger("radio.stationid") >> 12) & 0xFFFF);

    return chipID;
}

static float _getAltitudeAdjustedPressure(uint32_t rawPressure) {
    static bool         isCalculated = false;
    static double       compensationFactor;
    double              altitude;
    float               adjustedPressure;

    cfgmgr & cfg = cfgmgr::getInstance();

    if (!isCalculated) {
        altitude = cfg.getValueAsDouble("calibration.altitude");

        compensationFactor = pow(
                    ((double)1.0f - ((double)ALITUDE_COMP_FACTOR * altitude)), 
                    (double)ALTITUDE_COMP_POWER) *
                    (double)100.0f;

        isCalculated = true;
    }

    logger & log = logger::getInstance();

    log.logDebug("Altitude compensation factor: %.2f", compensationFactor);

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

    if (tr->windspeed > ds->max_wind_speed) {
        ds->max_wind_speed = tr->windspeed;
    }

    if (tr->gustSpeed > ds->max_wind_gust) {
        ds->max_wind_gust = tr->gustSpeed;
    }

    ds->total_rainfall += tr->rainfall;
}

void ThreadManager::start() {
	logger & log = logger::getInstance();

		if (nrfListenThread.start()) {
			log.logStatus("Started NRFListenThread successfully");
		}
		else {
			throw thread_error("Failed to start NRFListenThread");
		}

		if (dbUpdateThread.start()) {
			log.logStatus("Started DBUpdateThread successfully");
		}
		else {
			throw thread_error("Failed to start DBUpdateThread");
		}

		if (wowUpdateThread.start()) {
			log.logStatus("Started WoWUpdateThread successfully");
		}
		else {
			throw thread_error("Failed to start WoWUpdateThread");
		}
}

void ThreadManager::kill() {
    nrfListenThread.stop();
    dbUpdateThread.stop();
    wowUpdateThread.stop();
}

weather_transform_t * NRFListenThread::transformWeatherPacket(weather_packet_t * source) {
    static weather_transform_t transform;

    weather_transform_t * target = &transform;

    target->packetNum = 0;
    target->packetNum = ((uint32_t)source->packetNum[2] << 16) | ((uint32_t)source->packetNum[1] << 8) | ((uint32_t)source->packetNum[0]);
    target->packetNum &= 0x00FFFFFF;

    logger & log = logger::getInstance();

    log.logDebug("Raw battery volts: %u", (uint32_t)source->rawBatteryVolts);
    log.logDebug("Raw battery percentage: %u", (uint32_t)source->rawBatteryPercentage);
    log.logDebug("Raw battery charge rate: %d", (int)source->rawBatteryChargeRate);

    target->batteryVoltage = (float)source->rawBatteryVolts * 78.125f / 1000000.0f;
    target->batteryPercentage = (float)source->rawBatteryPercentage;
    target->batteryChargeRate = (float)source->rawBatteryChargeRate * 0.208f;

    target->status_bits = (int32_t)(source->status & 0x000000FF);

    log.logDebug("Raw temperature: %d", (int)source->rawTemperature);

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

    log.logDebug("Raw ICP Pressure: %u", source->rawICPPressure);

    target->normalisedPressure = _getAltitudeAdjustedPressure(source->rawICPPressure);
    target->actualPressure = (float)source->rawICPPressure / 100.0f;

    log.logDebug("Raw windspeed: %u", (uint32_t)source->rawWindspeed);
    log.logDebug("Raw rainfall: %u", (uint32_t)source->rawRainfall);

    cfgmgr & cfg = cfgmgr::getInstance();

    float anemometerFactor = (float)cfg.getValueAsDouble("calibration.anemometerfactor");

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

    return target;
}

void * NRFListenThread::run() {
    int                 rtn;
    char                rxBuffer[64];
    uint8_t             packetID;
    weather_packet_t    pkt;
    sleep_packet_t      sleepPkt;
    watchdog_packet_t   wdPkt;
    int                 msgCounter = 0;

    nrf_p nrf = getNRFReference();

    logger & log = logger::getInstance();
    cfgmgr & cfg = cfgmgr::getInstance();

    log.logInfo("Opening NRF24L01 device");

    NRF_init(nrf);

    NRF_set_local_address(nrf, nrf->local_address);
    NRF_set_remote_address(nrf, nrf->remote_address);

	rtn = NRF_read_register(nrf, NRF24L01_REG_CONFIG, rxBuffer, 1);

    if (rtn < 0) {
        log.logError("Failed to transfer SPI data: %s\n", lguErrorText(rtn));

        return NULL;
    }

    log.logInfo("Read back CONFIG reg: 0x%02X\n", (int)rxBuffer[0]);

    if (rxBuffer[0] == 0x00) {
        log.logError("Config read back as 0x00, device is probably not plugged in?\n\n");
        return NULL;
    }

    uint16_t stationID = _getExpectedChipID();

    log.logInfo("Read station ID from config as: 0x%08X", stationID);

    int postCycleSeconds = cfg.getValueAsInteger("wow.postcycletime");

    log.logInfo("Got post cycle time for WOW service [%d]", postCycleSeconds);

    while (true) {
        weather_transform_t * tr;

        while (NRF_data_ready(nrf)) {
            log.logDebug("NRF24L01 has received data...");
            NRF_get_payload(nrf, rxBuffer);

            if (strHexDump(szDumpBuffer, 1024, rxBuffer, NRF_MAX_PAYLOAD) > 0) {
                log.logDebug("%s", szDumpBuffer);
            }

            packetID = _getPacketType(rxBuffer);

            switch (packetID) {
                case PACKET_ID_WEATHER:
                    memcpy(&pkt, rxBuffer, sizeof(weather_packet_t));

                    tr = transformWeatherPacket(&pkt);

                    msgCounter += 30;

                    if (msgCounter == postCycleSeconds) {
                        webPostQueue.push(*tr);

                        msgCounter = 0;
                    }

                    dbq.push(*tr);

                    log.logDebug("Got weather data:");
                    log.logDebug("\tPacket num:  %u", tr->packetNum);
                    log.logDebug("\tStatus:      0x%04X", pkt.status);
                    log.logDebug("\tBat. volts:  %.2f", tr->batteryVoltage);
                    log.logDebug("\tBat. percent:%.2f", tr->batteryPercentage);
                    log.logDebug("\tBat. crate:  %.2f", tr->batteryChargeRate);
                    log.logDebug("\tTemperature: %.2f", tr->temperature);
                    log.logDebug("\tDew point:   %.2f", tr->dewPoint);
                    log.logDebug("\tAdj pressure:%.2f", tr->normalisedPressure);
                    log.logDebug("\tAct pressure:%.2f", tr->actualPressure);
                    log.logDebug("\tHumidity:    %d%%", (int)tr->humidity);
                    log.logDebug("\tWind speed:  %.2f", tr->windspeed);
                    log.logDebug("\tWind gust:   %.2f", tr->gustSpeed);
                    log.logDebug("\tRainfall:    %.2f", tr->rainfall);
                    break;

                case PACKET_ID_SLEEP:
                    memcpy(&sleepPkt, rxBuffer, sizeof(sleep_packet_t));

                    pkt.rawBatteryVolts = sleepPkt.rawBatteryVolts;

                    tr = transformWeatherPacket(&pkt);

                    log.logStatus("Got sleep packet:");
                    log.logStatus("\tStatus:      0x%08X", sleepPkt.status);
                    log.logStatus("\tBat. volts:  %.2f", tr->batteryVoltage);
                    log.logStatus("\tSleep for:   %d", (int)sleepPkt.sleepHours);
                    break;

                case PACKET_ID_WATCHDOG:
                    memcpy(&wdPkt, rxBuffer, sizeof(watchdog_packet_t));

                    log.logStatus("Got watchdog packet");
                    break;

                default:
                    log.logError("Undefined packet type received: ID[0x%02X]", packetID);
                    break;
            }

            PosixThread::sleep_ms(250);
        }

        PosixThread::sleep(2);
    }

    return NULL;
}

void * DBUpdateThread::run() {
    PGconn *                wctlConnection;
    weather_transform_t     tr;
    daily_summary_t         ds;
    bool                    isSummaryDone = false;
    int                     hour;
    int                     minute;
    char                    szInsertStr[INSERT_STRING_LEN];

    logger & log = logger::getInstance();
    cfgmgr & cfg = cfgmgr::getInstance();

    memset(&ds, 0, sizeof(daily_summary_t));

    wctlConnection = dbConnect(
            cfg.getValue("db.host").c_str(), 
            cfg.getValueAsInteger("db.port"),
            cfg.getValue("db.database").c_str(),
            cfg.getValue("db.user").c_str(),
            cfg.getValue("db.password").c_str());

    if (wctlConnection == NULL) {
        log.logError("Could not connect to database %s", cfg.getValue("db.database").c_str());
        return NULL;
    }

    while (true) {
        while (dbq.empty()) {
            PosixThread::sleep_ms(25);
        }

        tr = dbq.front();
        dbq.pop();

        log.logDebug("Updating summary structure");

        updateSummary(&ds, &tr);

        string timestamp = getTimestamp();

        log.logDebug("Inserting weather data");

        snprintf(
            szInsertStr,
            INSERT_STRING_LEN,
            pszWeatherInsertStmt,
            timestamp.c_str(),
            (int32_t)tr.packetNum,
            tr.temperature,
            tr.dewPoint,
            tr.actualPressure,
            tr.normalisedPressure,
            tr.humidity,
            tr.rainfall,
            tr.windspeed,
            tr.gustSpeed
        );

        dbExecute(wctlConnection, szInsertStr);

        PosixThread::sleep_ms(100);

        log.logDebug("Inserting telemetry data");

        snprintf(
            szInsertStr,
            INSERT_STRING_LEN,
            pszTelemetryInsertStmt,
            timestamp.c_str(),
            (int32_t)tr.packetNum,
            tr.batteryVoltage,
            tr.batteryPercentage,
            tr.batteryChargeRate,
            tr.status_bits
        );

        dbExecute(wctlConnection, szInsertStr);

        struct tm * localtime = getLocalTime();

        hour = localtime->tm_hour;
        minute = localtime->tm_min;

        /*
        ** On the stroke of midnight, write the daily_summary
        ** and reset the summary values...
        */
        if (hour == 23 && minute == 59 && !isSummaryDone) {
            PosixThread::sleep_ms(100);

            log.logDebug("Inserting daily summary");

            string date = getTodaysDate();

            snprintf(
                szInsertStr,
                INSERT_STRING_LEN,
                pszSummaryInsertStmt,
                date.c_str(),
                ds.min_temperature,
                ds.max_temperature,
                ds.min_pressure,
                ds.max_pressure,
                ds.min_humidity,
                ds.max_humidity,
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

        PosixThread::sleep_ms(250);
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

void * WoWUpdateThread::run() {
    float                   tempF;
    float                   dewPointF;
    float                   pressureInHg;
    weather_transform_t     tr;
    CURL *                  pCurl;
	CURLcode			    result;
    char                    szCurlError[CURL_ERROR_SIZE];
    char                    szURL[1024];
    curl_chunk_t            chunk;
    char *                  encodedDate;

    logger & log = logger::getInstance();
    cfgmgr & cfg = cfgmgr::getInstance();

    chunk.length = 0;
    chunk.response = NULL;

    while (true) {
        pCurl = curl_easy_init();

        if (pCurl == NULL) {
            curl_easy_cleanup(pCurl);
            log.logError("Failed to initialise curl");
            break;
        }

        curl_easy_setopt(pCurl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
        curl_easy_setopt(pCurl, CURLOPT_ERRORBUFFER, szCurlError);

        string baseURL = cfg.getValue("wow.baseurl");
        string siteID = cfg.getValue("wow.siteid");
        string authKey = cfg.getValue("wow.authkey");
        string softwareType = cfg.getValue("wow.softwareid");

        log.logDebug("Base URL: %s", baseURL.c_str());
        log.logDebug("Site ID: %s", siteID.c_str());
        log.logDebug("Software ID: %s", softwareType.c_str());

        while (webPostQueue.empty()) {
            PosixThread::sleep_ms(25);
        }

        tr = webPostQueue.front();
        webPostQueue.pop();

        encodedDate = getEncodedTimeStamp();

        tempF = (tr.temperature * 1.8) + 32;
        dewPointF = (tr.dewPoint * 1.8) + 32;
        pressureInHg = (tr.normalisedPressure) * HPA_TO_INHG;

        log.logDebug("Preparing to POST to WoW service");

        snprintf(
            szURL, 
            URL_STRING_LEN,
            "%s?siteid=%s&siteAuthenticationKey=%s&dateutc=%s&softwaretype=%s",
            baseURL.c_str(),
            siteID.c_str(),
            authKey.c_str(),
            encodedDate,
            softwareType.c_str());

        free(encodedDate);

        snprintf(
            &szURL[strlen(szURL)],
            (URL_STRING_LEN - strlen(szURL)),
            "&tempf=%.2f&baromin=%.2f&humidity=%.2f&dewptf=%.2f&windspeedmph=%.2f&windgustmph=%.2f",
            tempF,
            pressureInHg,
            tr.humidity,
            dewPointF,
            tr.windspeed,
            tr.gustSpeed);

        log.logInfo("Posting to URL: %s", szURL);

        if (cfg.getValueAsBoolean("wow.isenabled")) {
            curl_easy_setopt(pCurl, CURLOPT_URL, szURL);

            curl_easy_setopt(pCurl, CURLOPT_HTTPGET, 1L);
            curl_easy_setopt(pCurl, CURLOPT_USERAGENT, "libcrp/0.1");
            curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &CurlWrite_CallbackFunc);
            curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &chunk);

            result = curl_easy_perform(pCurl);

            if (result != CURLE_OK) {
                log.logError("Failed to post to %s - Curl error [%s]", szURL, szCurlError);
                return NULL;
            }

            log.logInfo("WoW service responded: %s", chunk.response);
        }
        else {
            log.logInfo("Posting disbaled by config - do nothing");
        }

        PosixThread::sleep_ms(250);
    }

    return NULL;
}
