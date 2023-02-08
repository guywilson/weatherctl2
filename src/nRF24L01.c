#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#include "dummy_lgpio.h"
#else
#include <lgpio.h>
#endif

#include "spi.h"
#include "nRF24L01.h"

static int         hGPIO;

void _powerUp(int hSPI) {
    uint8_t             statusReg;
    uint8_t             configReg;

    spiWriteReadByte(hSPI, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg);
    spiReadByte(hSPI, &configReg);

    configReg |= 0x02;

    spiWriteReadByte(hSPI, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg);
    spiWriteByte(hSPI, configReg);
}

void _powerDown(int hSPI) {
    uint8_t             statusReg;
    uint8_t             configReg;

    spiWriteReadByte(hSPI, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg);
    spiReadByte(hSPI, &configReg);

    configReg &= 0xFD;

    spiWriteReadByte(hSPI, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg);
    spiWriteByte(hSPI, configReg);
}

bool _isRxDataAvailable(int hSPI) {
    uint8_t             statusReg;
    bool                isDataAvailable = false;

    spiWriteReadByte(hSPI, NRF24L01_CMD_NOP, &statusReg);

    isDataAvailable = (statusReg & 0x40) ? true : false;

    return isDataAvailable;
}

int _transmit(
            int hSPI, 
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

    spiWriteReadByte(hSPI, NRF24L01_CMD_FLUSH_TX, &statusReg);
    spiWriteReadByte(hSPI, command, &statusReg);
    spiWriteData(hSPI, buf, 32);

    // sprintf(szTemp, "St: 0x%02X\n", statusReg);
    // uart_puts(uart0, szTemp);

    /*
    ** Pulse the CE line for > 10us to enable 
    ** the device in TX mode to send the data
    */
    lgGpioWrite(hGPIO, NRF24L01_SPI_PIN_CE, 1);

    usleep(11U);

    lgGpioWrite(hGPIO, NRF24L01_SPI_PIN_CE, 0);

    /*
    ** Clear the TX_DS bit...
    */
    spiWriteReadByte(hSPI, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_STATUS, &statusReg);
    spiWriteByte(hSPI, (statusReg & 0xDF));

    return 0;
}

int nRF24L01_setup(int hgpio, int hspi) {
    int             error = 0;
    uint8_t         statusReg = 0;
    uint8_t         configReg = 0;
    uint8_t         configData[8];
    uint8_t         txAddr[5]; // {0xE0, 0xE0, 0xF1, 0xF1, 0xE0};

    hGPIO = hgpio;

	/*
	** SPI CE
	*/
    lgGpioClaimOutput(hGPIO, 0, NRF24L01_SPI_PIN_CE, 0);

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

    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg);
    spiWriteByte(hspi, configData[0]);

    sleep(2);

    spiWriteReadByte(hspi, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg);
    spiReadByte(hspi, &configReg);

    printf("Cfg: 0x%02X\n", configReg);

    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_EN_AA, &statusReg);
    spiWriteByte(hspi, configData[1]);

    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_EN_RXADDR, &statusReg);
    spiWriteByte(hspi, configData[2]);

    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_SETUP_AW, &statusReg);
    spiWriteByte(hspi, configData[3]);

    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_SETUP_RETR, &statusReg);
    spiWriteByte(hspi, configData[4]);

    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RF_CH, &statusReg);
    spiWriteByte(hspi, configData[5]);

    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RF_SETUP, &statusReg);
    spiWriteByte(hspi, configData[6]);

    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_STATUS, &statusReg);
    spiWriteByte(hspi, configData[7]);

    printf("St: 0x%02X\n", statusReg);

    txAddr[0] = 0x34;
    txAddr[1] = 0x43;
    txAddr[2] = 0x10;
    txAddr[3] = 0x10;
    txAddr[4] = 0x01;

    spiWriteReadByte(
                hspi, 
                NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RX_ADDR_PO, 
                &statusReg);
    spiWriteData(hspi, txAddr, 5);

    spiWriteReadByte(
                hspi, 
                NRF24L01_CMD_W_REGISTER | NRF24L01_REG_TX_ADDR, 
                &statusReg);
    spiWriteData(hspi, txAddr, 5);

    printf("St: 0x%02X\n", statusReg);

    /*
    ** Activate additional features...
    */
    spiWriteReadByte(hspi, NRF24L01_CMD_ACTIVATE, &statusReg);
    spiWriteByte(hspi, 0x73);

    /*
    ** Enable NOACK transmit...
    */
    spiWriteReadByte(hspi, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_FEATURE, &statusReg);
    spiWriteByte(hspi, 0x01);

//    _powerUp(spi);

    return error;
}

int nRF24L01_transmit_buffer(
            int hSPI, 
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

    _transmit(hSPI, buffer, requestACK);

    return error;
}

int nRF24L01_transmit_string(
            int hSPI, 
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

    _transmit(hSPI, buffer, requestACK);

    return error;
}

int nRF24L01_receive_blocking(int hSPI, uint8_t * buffer, int length) {
    uint8_t             statusReg;
    uint8_t             buf[32];
    int                 error = 0;

    while (!_isRxDataAvailable(hSPI)) {
        usleep(100U);
    }

    spiWriteReadByte(hSPI, NRF24L01_CMD_R_RX_PAYLOAD, &statusReg);
    spiReadData(hSPI, buf, 32);

    memcpy(buffer, buf, length);

    return error;
}
