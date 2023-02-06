#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <librpi/librpi.h>

#include "nRF24L01.h"

GPIO *              pGPIO;

void _powerUp(SPI * spi) {
    uint8_t             statusReg;
    uint8_t             configReg;

    spiWriteReadByte(spi, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiReadByte(spi, &configReg, false);

    configReg |= 0x02;

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiWriteByte(spi, configReg, false);
}

void _powerDown(SPI * spi) {
    uint8_t             statusReg;
    uint8_t             configReg;

    spiWriteReadByte(spi, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiReadByte(spi, &configReg, false);

    configReg &= 0xFD;

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiWriteByte(spi, configReg, false);
}

bool _isRxDataAvailable(SPI * spi) {
    uint8_t             statusReg;
    bool                isDataAvailable = false;

    spiWriteReadByte(spi, NRF24L01_CMD_NOP, &statusReg, false);

    isDataAvailable = (statusReg & 0x40) ? true : false;

    return isDataAvailable;
}

int _transmit(
            SPI * spi, 
            uint8_t * buf, 
            bool requestACK)
{
    uint8_t             command;
    uint8_t             statusReg;

    if (requestACK) {
        command = NRF24L01_CMD_W_TX_PAYLOAD;
    }
    else {
        command = NRF24L01_CMD_W_TX_PAYLOAD_NOACK;
    }

    spiWriteReadByte(spi, NRF24L01_CMD_FLUSH_TX, &statusReg, false);
    spiWriteReadByte(spi, command, &statusReg, true);
    spiWriteData(spi, buf, 32, false);

    // sprintf(szTemp, "St: 0x%02X\n", statusReg);
    // uart_puts(uart0, szTemp);

    /*
    ** Pulse the CE line for > 10us to enable 
    ** the device in TX mode to send the data
    */
    gpio_set_pin_state(pGPIO, NRF24L01_SPI_PIN_CE, GPIO_HIGH);

    usleep(11U);

    gpio_set_pin_state(pGPIO, NRF24L01_SPI_PIN_CE, GPIO_LOW);

    /*
    ** Clear the TX_DS bit...
    */
    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_STATUS, &statusReg, true);
    spiWriteByte(spi, (statusReg & 0xDF), false);

    return 0;
}

int nRF24L01_setup(RPIHANDLE * rpi, SPI * spi) {
    int             error = 0;
    uint8_t         statusReg = 0;
    uint8_t         configReg = 0;
    uint8_t         configData[8];
    uint8_t         txAddr[5]; // {0xE0, 0xE0, 0xF1, 0xF1, 0xE0};

    printf("Opening GPIO...\n");

    pGPIO = gpio_open(rpi);

	/*
	** SPI CE
	*/
    gpio_set_pin_function(pGPIO, NRF24L01_SPI_PIN_CE, GPIO_FN_OUTPUT);
	gpio_set_pin_state(pGPIO, NRF24L01_SPI_PIN_CE, GPIO_LOW);

    sleep(100);

    configData[0] = 0x7F;       // CONFIG register
    configData[1] = 0x00;       // EN_AA register
    configData[2] = 0x01;       // EN_RXADDR register
    configData[3] = 0x03;       // SETUP_AW register
    configData[4] = 0x00;       // SETUP_RETR register
    configData[5] = 40;         // RF_CH register
    configData[6] = 0x07;       // RF_SETUP register
    configData[7] = 0x70;       // STATUS register

    printf("Configuring nRF24L01 device..\n");

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiWriteByte(spi, configData[0], false);

    sleep(2);

    spiWriteReadByte(spi, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiReadByte(spi, &configReg, false);

    printf("Cfg: 0x%02X\n", configReg);

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_EN_AA, &statusReg, true);
    spiWriteByte(spi, configData[1], false);

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_EN_RXADDR, &statusReg, true);
    spiWriteByte(spi, configData[2], false);

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_SETUP_AW, &statusReg, true);
    spiWriteByte(spi, configData[3], false);

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_SETUP_RETR, &statusReg, true);
    spiWriteByte(spi, configData[4], false);

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RF_CH, &statusReg, true);
    spiWriteByte(spi, configData[5], false);

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RF_SETUP, &statusReg, true);
    spiWriteByte(spi, configData[6], false);

    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_STATUS, &statusReg, true);
    spiWriteByte(spi, configData[7], false);

    printf("St: 0x%02X\n", statusReg);

    txAddr[0] = 0x34;
    txAddr[1] = 0x43;
    txAddr[2] = 0x10;
    txAddr[3] = 0x10;
    txAddr[4] = 0x01;

    spiWriteReadByte(
                spi, 
                NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RX_ADDR_PO, 
                &statusReg, 
                true);
    spiWriteData(spi, txAddr, 5, false);

    spiWriteReadByte(
                spi, 
                NRF24L01_CMD_W_REGISTER | NRF24L01_REG_TX_ADDR, 
                &statusReg, 
                true);
    spiWriteData(spi, txAddr, 5, false);

    printf("St: 0x%02X\n", statusReg);

    /*
    ** Activate additional features...
    */
    spiWriteReadByte(spi, NRF24L01_CMD_ACTIVATE, &statusReg, true);
    spiWriteByte(spi, 0x73, false);

    /*
    ** Enable NOACK transmit...
    */
    spiWriteReadByte(spi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_FEATURE, &statusReg, true);
    spiWriteByte(spi, 0x01, false);

//    _powerUp(spi);

    return error;
}

int nRF24L01_transmit_buffer(
            SPI * spi, 
            uint8_t * buf, 
            int length, 
            bool requestACK)
{
    int             error = 0;
    uint8_t         buffer[32];

    if (length > 32) {
        return -1;
    }

    memset(buffer, 0, 32);
    memcpy(buffer, buf, length);

    _transmit(spi, buffer, requestACK);

    return error;
}

int nRF24L01_transmit_string(
            SPI * spi, 
            char * pszText, 
            bool requestACK)
{
    int             error = 0;
    uint8_t         buffer[32];
    int             strLength;

    strLength = strlen(pszText);

    if (strLength > 32) {
        strLength = 32;
    }

    memset(buffer, 0, 32);
    memcpy(buffer, pszText, strLength);

    _transmit(spi, buffer, requestACK);

    return error;
}

int nRF24L01_receive_blocking(SPI * spi, uint8_t * buffer, int length) {
    uint8_t             statusReg;
    uint8_t             buf[32];
    int                 error = 0;

    while (!_isRxDataAvailable(spi)) {
        usleep(100U);
    }

    spiWriteReadByte(spi, NRF24L01_CMD_R_RX_PAYLOAD, &statusReg, true);
    spiReadData(spi, buf, 32, false);

    memcpy(buffer, buf, length);

    return error;
}
