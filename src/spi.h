#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_SPI
#define __INCL_SPI

int spiWriteReadByte(int hSPI, uint8_t txByte, uint8_t * rxByte);
int spiWriteReadWord(int hSPI, uint16_t txWord, uint16_t * rxWord);
int spiWriteReadData(int hSPI, uint8_t * txData, uint8_t * rxData, uint32_t dataLength);
int spiReadByte(int hSPI, uint8_t * rxByte);
int spiReadWord(int hSPI, uint16_t * rxWord);
int spiReadData(int hSPI, uint8_t * rxData, uint32_t dataLength);
int spiWriteByte(int hSPI, uint8_t txByte);
int spiWriteWord(int hSPI, uint16_t txWord);
int spiWriteData(int hSPI, uint8_t * txData, uint32_t dataLength);

#endif
