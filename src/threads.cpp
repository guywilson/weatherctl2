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
    bool                go = true;
    int                 hspi;
    uint8_t             rxBuffer[32];
    uint8_t             deviceAddress[5];
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

    nRF24L01 radio(hspi, NRF24L01_SPI_PIN_CE);

    radio.writeConfig(
            NRF24L01_CFG_MASK_RX_DR | 
            NRF24L01_CFG_MASK_TX_DS | 
            NRF24L01_CFG_MASK_MAX_RT | 
            NRF24L01_CFG_ENABLE_CRC | 
            NRF24L01_CFG_CRC_2_BYTE | 
            NRF24L01_CFG_MODE_RX | 
            NRF24L01_CFG_POWER_UP);

    radio.setAddressWidth(nRF24L01::aw5Bytes);
    radio.enableRxAddress(1, true);
    radio.setupAutoRetransmission(0, 0);
    radio.setRFChannel(40);
    radio.writeRFSetup(
            NRF24L01_RF_SETUP_RF_POWER_HIGH | 
            NRF24L01_RF_SETUP_RF_LNA_GAIN_ON);

    deviceAddress[0] = 0x34;
    deviceAddress[1] = 0x43;
    deviceAddress[2] = 0x10;
    deviceAddress[3] = 0x10;
    deviceAddress[4] = 0x01;

    radio.setRxAddress(1, deviceAddress);
    radio.setTxAddress(deviceAddress);

    radio.activateAdditionalFeatures();
    radio.enableFeatures(NRF24L01_FEATURE_EN_TX_NO_ACK);

    while (go) {
        radio.receive(rxBuffer);

        memcpy(&weather, rxBuffer, sizeof(weather_packet_t));

        printf("Got weather data:\n");
        printf("\tTemperature:      %.2f\n", weather.temperature);
        printf("\tHumidity:         %.2f\n", weather.humidity);
        printf("\tPressure:         %.2f\n", weather.pressure);
        printf("\tRainfall:         %.2f\n", weather.rainfall);
        printf("\tWind speed:       %.2f\n", weather.windspeed);
        printf("\tWind direction:   %d\n\n", weather.windDirection);

        sleep(seconds, 4U);
    }

    return NULL;
}
