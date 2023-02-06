#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <librpi/librpi.h>

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
    RPIHANDLE *         rpi;
    SPI *               spi;
    uint8_t             rxBuffer[32];
    weather_packet_t    weather;

    ConfigManager & cfg = ConfigManager::getInstance();

    printf("Initialising RPI...\n");

    rpi = rpi_init();

    rpi_print_info(rpi);

    printf("Opening SPI device [%d]\n", cfg.getValueAsInteger("radio.spiport"));

    spi = spiOpen(rpi, cfg.getValueAsInteger("radio.spiport"));

    printf(
        "Setting up SPI device with freq [%lu]\n", 
        strtoul(cfg.getValue("radio.spifreq"), NULL, 10));

    spiSetup(spi, strtoul(cfg.getValue("radio.spifreq"), NULL, 10));

    printf("Setting up nRF24L01 device...\n");

    nRF24L01_setup(rpi, spi);

    while (go) {
        nRF24L01_receive_blocking(spi, rxBuffer, 32);

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
