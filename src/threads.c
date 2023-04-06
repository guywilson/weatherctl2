#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lgpio.h>
#include <postgresql/libpq-fe.h>

#include "NRF24.h"
#include "nRF24L01.h"
#include "logger.h"
#include "cfgmgr.h"
#include "posixthread.h"
#include "psql.h"
#include "que.h"
#include "icp10125.h"
#include "veml7700.h"
#include "utils.h"

que_handle_t            dbq;

static void _transformWeatherPacket(weather_transform_t * target, weather_packet_t * source) {
    const float conversionFactor = 0.000806f;

    lgLogDebug(lgGetHandle(), "Raw battery volts ADC: %u", (uint32_t)source->rawBatteryVolts);
    lgLogDebug(lgGetHandle(), "Raw battery temp ADC: %u", (uint32_t)source->rawBatteryTemperature);
    lgLogDebug(lgGetHandle(), "Raw chip temp ADC: %u", (uint32_t)source->rawChipTemperature);

    target->batteryVoltage = ((float)source->rawBatteryVolts / 4096.0) * 3 * 3.3;
    target->batteryTemperature = (float)source->rawBatteryTemperature;
    target->chipTemperature = 27.0f - (((float)source->rawChipTemperature * conversionFactor) - 0.706f) / 0.001721f;

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
}

void startThreads(void) {

}

void stopThreads(void) {

}

void * NRF_listen_thread(void * pParms) {
    int                 rtn;
    uint32_t            stationID;
    char                rxBuffer[64];
    weather_packet_t    pkt;
    weather_transform_t tr;

    nrf_p nrf = (nrf_p)pParms;

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

            hexDump(rxBuffer, NRF_MAX_PAYLOAD);

            memcpy(&pkt, rxBuffer, sizeof(weather_packet_t));

            if (pkt.chipID == stationID) {
                _transformWeatherPacket(&tr, &pkt);

                lgLogDebug(lgGetHandle(), "Got weather data:");
                lgLogDebug(lgGetHandle(), "\tChipID:      0x%08X", pkt.chipID);
                lgLogDebug(lgGetHandle(), "\tBat. volts:  %.2f", tr.batteryVoltage);
                lgLogDebug(lgGetHandle(), "\tRP2040 temp: %.2f", tr.chipTemperature);
                lgLogDebug(lgGetHandle(), "\tTemperature: %.2f", tr.temperature);
                lgLogDebug(lgGetHandle(), "\tPressure:    %.2f", tr.pressure);
                lgLogDebug(lgGetHandle(), "\tHumidity:    %d%%", (int)tr.humidity);
                lgLogDebug(lgGetHandle(), "\tLux:         %.2f", tr.lux);
            }
            else {
                lgLogDebug(lgGetHandle(), "Failed chipID check, got 0x%08X", pkt.chipID);
            }

            pxtSleep(milliseconds, 250);
        }

        pxtSleep(seconds, 2);
    }

    return NULL;
}

void * db_update_thread(void * pParms) {
    PGconn *            wctlConnection;
    que_handle_t *      dbQueue;
    
    dbQueue = (que_handle_t *)pParms;

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
        while (!qGetQueLength(dbQueue)) {
            pxtSleep(milliseconds, 250);
        }

        dbTransactionBegin(wctlConnection);
        dbExecute(wctlConnection, "INSERT INTO weather_data;");
        dbTransactionEnd(wctlConnection);
    }

    dbFinish(wctlConnection);
    qDestroy(dbQueue);

    return NULL;
}
