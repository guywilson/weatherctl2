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
    int                 hgpio;
    int                 hspi;
    uint8_t             rxBuffer[32];
    weather_packet_t    weather;

    ConfigManager & cfg = ConfigManager::getInstance();

    printf("Initialising RPI...\n");

    hgpio = lgGpiochipOpen(0);

    printf("Opening SPI device [%d]\n", cfg.getValueAsInteger("radio.spiport"));

    hspi = lgSpiOpen(
        cfg.getValueAsInteger("radio.spiport"), 
        cfg.getValueAsInteger("radio.spiport"), 
        cfg.getValueAsInteger("radio.spifreq"), 
        0);

    printf("Setting up nRF24L01 device...\n");

    nRF24L01_setup(hgpio, hspi);

    while (go) {
        nRF24L01_receive_blocking(hspi, rxBuffer, 32);

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
