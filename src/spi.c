#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#include "dummy_lgpio.h"
#else
#include <lgpio.h>
#endif

#include "spi.h"

#define MAX_XFER_BUFFER_SIZE                512

static char txBuffer[MAX_XFER_BUFFER_SIZE];
static char rxBuffer[MAX_XFER_BUFFER_SIZE];

int spiWriteReadByte(int hSPI, uint8_t txByte, uint8_t * rxByte) {
    char        tx;
    char        rx;
    int         rtn;

    tx = (char)txByte;

    rtn = lgSpiXfer(hSPI, &tx, &rx, 1);

    *rxByte = (uint8_t)rx;

    return rtn;
}

int spiWriteReadWord(int hSPI, uint16_t txWord, uint16_t * rxWord) {
    char        tx[2];
    char        rx[2];
    int         rtn;

    tx[0] = (char)(txWord & 0x00FF);
    tx[1] = (char)((txWord >> 8) & 0x00FF);

    rtn = lgSpiXfer(hSPI, tx, rx, 2);

    *rxWord = (uint16_t)(((uint16_t)rx[1] << 8) | (uint16_t)rx[0]);

    return rtn;
}

int spiWriteReadData(int hSPI, uint8_t * txData, uint8_t * rxData, uint32_t dataLength) {
    int         rtn;

    if (dataLength > MAX_XFER_BUFFER_SIZE) {
        fprintf(stderr, "Exceeded max transfer buffer size\n");
        return -1;
    }

    memcpy(txBuffer, txData, dataLength);

    rtn = lgSpiXfer(hSPI, txBuffer, rxBuffer, dataLength);

    memcpy(rxData, rxBuffer, dataLength);

    return rtn;
}

int spiReadByte(int hSPI, uint8_t * rxByte) {
    char        rx;
    int         rtn;

    rtn = lgSpiRead(hSPI, &rx, 1);

    *rxByte = (uint8_t)rx;

    return rtn;
}

int spiReadWord(int hSPI, uint16_t * rxWord) {
    char        rx[2];
    int         rtn;

    rtn = lgSpiRead(hSPI, rx, 2);

    *rxWord = (uint16_t)(((uint16_t)rx[1] << 8) | (uint16_t)rx[0]);

    return rtn;
}

int spiReadData(int hSPI, uint8_t * rxData, uint32_t dataLength) {
    int         rtn;

    if (dataLength > MAX_XFER_BUFFER_SIZE) {
        fprintf(stderr, "Exceeded max transfer buffer size\n");
        return -1;
    }

    rtn = lgSpiRead(hSPI, rxBuffer, dataLength);

    memcpy(rxData, rxBuffer, dataLength);

    return rtn;
}

int spiWriteByte(int hSPI, uint8_t txByte) {
    char        tx;
    int         rtn;

    tx = (char)txByte;

    rtn = lgSpiWrite(hSPI, &tx, 1);

    return rtn;
}

int spiWriteWord(int hSPI, uint16_t txWord) {
    char        tx[2];
    int         rtn;

    tx[0] = (char)(txWord & 0x00FF);
    tx[1] = (char)((txWord >> 8) & 0x00FF);

    rtn = lgSpiWrite(hSPI, tx, 2);

    return rtn;
}

int spiWriteData(int hSPI, uint8_t * txData, uint32_t dataLength) {
    int         rtn;

    if (dataLength > MAX_XFER_BUFFER_SIZE) {
        fprintf(stderr, "Exceeded max transfer buffer size\n");
        return -1;
    }

    memcpy(txBuffer, txData, dataLength);

    rtn = lgSpiWrite(hSPI, txBuffer, dataLength);

    return rtn;
}
