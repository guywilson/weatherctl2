#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <librpi/librpi.h>

#ifdef __APPLE__
extern "C" {
#include "dummy_lgpio.h"
}
#else
#include <lgpio.h>
#endif

#include "threads.h"
#include "logger.h"
#include "configmgr.h"

extern "C" {
#include "nRF24L01.h"
}

using namespace std;

typedef struct {
    float               temperature;
    float               pressure;
    float               humidity;
    float               rainfall;
    float               windspeed;
    uint16_t            windDirection;
}
weather_packet_t;

void * RadioRxThread::run() {
    int                 hspi;
    int                 counter = 0;
    int                 CEPin;
    char                txBuffer[40];
    char                rxBuffer[40];
    uint8_t             deviceAddress[5];
    uint8_t             dataRate;
    weather_packet_t    weather;
    
    Logger & log = Logger::getInstance();
    ConfigManager & cfg = ConfigManager::getInstance();

    log.logDebug("RadioRxThread::run() - Opening SPI device [%d]", cfg.getValueAsInteger("radio.spiport"));

    hspi = lgSpiOpen(
        cfg.getValueAsInteger("radio.spiport"), 
        cfg.getValueAsInteger("radio.spiport"), 
        cfg.getValueAsInteger("radio.spifreq"), 
        0);

    log.logDebug("RadioRxThread::run() - Setting up nRF24L01 device...");

    int hGPIO = lgGpiochipOpen(0);

    if (hGPIO < 0) {
        log.logError("nRF24L01::init() - Failed to open GPIO device: %s", lguErrorText(hGPIO));
        return NULL;
    }

    CEPin = cfg.getValueAsInteger("radio.spicepin");

    int rtn = lgGpioClaimOutput(hGPIO, 0, CEPin, 0);

    if (rtn < 0) {
        log.logError(
            "nRF24L01::init() - Failed to claim pin %d as output: %s", 
            CEPin, 
            lguErrorText(rtn));

        return NULL;
    }

    log.logDebug("Claimed pin %d for output with return code %d", CEPin, rtn);

    txBuffer[0] = (char)(NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG);

    rtn = lgSpiXfer(hspi, txBuffer, rxBuffer, 1);

    if (rtn < 0) {
        log.logError(
            "Failed to transfer SPI data: %s", 
            lguErrorText(rtn));

        return NULL;
    }

    log.logDebug("STATUS reg: 0x%02X", rxBuffer[0]);

    txBuffer[0] = (char)(0X7F);

    rtn = lgSpiWrite(hspi, txBuffer, 1);

    if (rtn < 0) {
        log.logError(
            "Failed to transfer SPI data: %s", 
            lguErrorText(rtn));

        return NULL;
    }

    txBuffer[0] = (char)(NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG);

    rtn = lgSpiXfer(hspi, txBuffer, rxBuffer, 1);

    if (rtn < 0) {
        log.logError(
            "Failed to transfer SPI data: %s", 
            lguErrorText(rtn));

        return NULL;
    }

    log.logDebug("STATUS reg: 0x%02X", rxBuffer[0]);

    rtn = lgSpiRead(hspi, rxBuffer, 1);

    if (rtn < 0) {
        log.logError(
            "Failed to transfer SPI data: %s", 
            lguErrorText(rtn));

        return NULL;
    }

    log.logDebug("Read back CONFIG reg: 0x%02X", rxBuffer[0]);

    lgSpiClose(hspi);
    lgGpioFree(hGPIO, CEPin);
    lgGpiochipClose(hGPIO);

    return NULL;

    // nRF24L01 radio(hspi, cfg.getValueAsInteger("radio.spicepin"));

    // radio.writeConfig(
    //         NRF24L01_CFG_MASK_RX_DR | 
    //         NRF24L01_CFG_MASK_TX_DS | 
    //         NRF24L01_CFG_MASK_MAX_RT | 
    //         NRF24L01_CFG_ENABLE_CRC | 
    //         NRF24L01_CFG_CRC_2_BYTE);

    // radio.setAddressWidth(nRF24L01::aw5Bytes);
    // radio.enableRxAddress(1, true);
    // radio.setupAutoRetransmission(0, 0);
    // radio.setRFChannel(40);

    // dataRate = 
    //     (strcmp(cfg.getValue("radio.baud"), "1MHz") == 0) ?
    //      NRF24L01_RF_SETUP_DATA_RATE_1MBPS : 
    //      NRF24L01_RF_SETUP_DATA_RATE_2MBPS;

    // radio.writeRFSetup(
    //         dataRate |
    //         NRF24L01_RF_SETUP_RF_POWER_HIGH | 
    //         NRF24L01_RF_SETUP_RF_LNA_GAIN_ON);

    // deviceAddress[0] = 0x34;
    // deviceAddress[1] = 0x43;
    // deviceAddress[2] = 0x10;
    // deviceAddress[3] = 0x10;
    // deviceAddress[4] = 0x01;

    // radio.setRxAddress(1, deviceAddress);
    // radio.setTxAddress(deviceAddress);

    // radio.setRxPayloadSize(1, 32);

    // radio.activateAdditionalFeatures();
    // radio.enableFeatures(NRF24L01_FEATURE_EN_TX_NO_ACK);

    // radio.startListening();
        
    // while (counter < 10) {
    //     radio.receive(rxBuffer);

    //     memcpy(&weather, rxBuffer, sizeof(weather_packet_t));

    //     printf("Got weather data:\n");
    //     printf("\tTemperature:      %.2f\n", weather.temperature);
    //     printf("\tHumidity:         %.2f\n", weather.humidity);
    //     printf("\tPressure:         %.2f\n", weather.pressure);
    //     printf("\tRainfall:         %.2f\n", weather.rainfall);
    //     printf("\tWind speed:       %.2f\n", weather.windspeed);
    //     printf("\tWind direction:   %d\n\n", weather.windDirection);

    //     sleep(seconds, 4U);

    //     counter++;
    // }

    // radio.stopListening();

    // return NULL;
}
