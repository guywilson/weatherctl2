#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string>
#include <exception>

#include "logger.h"

using namespace std;

#ifndef __INCL_NRF24L01
#define __INCL_NRF24L01

#define NRF24L01_DEFAULT_PACKET_LEN                 32

/*
** nRF24L01 commands
*/
#define NRF24L01_CMD_R_REGISTER                     0x00
#define NRF24L01_CMD_W_REGISTER                     0x20
#define NRF24L01_CMD_R_RX_PAYLOAD                   0x61
#define NRF24L01_CMD_W_TX_PAYLOAD                   0xA0
#define NRF24L01_CMD_FLUSH_TX                       0xE1
#define NRF24L01_CMD_FLUSH_RX                       0xE2
#define NRF24L01_CMD_REUSE_TX_PL                    0xE3
#define NRF24L01_CMD_ACTIVATE                       0x50
#define NRF24L01_CMD_R_RX_PL_WID                    0x60
#define NRF24L01_CMD_W_ACK_PAYLOAD                  0xA8
#define NRF24L01_CMD_W_TX_PAYLOAD_NOACK             0xB0
#define NRF24L01_CMD_NOP                            0xFF

#define NRF24L01_ACTIVATE_ADDITIONAL                0x73

/*
** nRF24L01 register map
*/
#define NRF24L01_REG_CONFIG                         0x00
#define NRF24L01_REG_EN_AA                          0x01
#define NRF24L01_REG_EN_RXADDR                      0x02
#define NRF24L01_REG_SETUP_AW                       0x03
#define NRF24L01_REG_SETUP_RETR                     0x04
#define NRF24L01_REG_RF_CH                          0x05
#define NRF24L01_REG_RF_SETUP                       0x06
#define NRF24L01_REG_STATUS                         0x07
#define NRF24L01_REG_OBSERVE_TX                     0x08
#define NRF24L01_REG_CD                             0x09
#define NRF24L01_REG_RX_ADDR_PO                     0x0A
#define NRF24L01_REG_RX_ADDR_P1                     0x0B
#define NRF24L01_REG_RX_ADDR_P2                     0x0C
#define NRF24L01_REG_RX_ADDR_P3                     0x0D
#define NRF24L01_REG_RX_ADDR_P4                     0x0E
#define NRF24L01_REG_RX_ADDR_P5                     0x0F
#define NRF24L01_REG_TX_ADDR                        0x10
#define NRF24L01_REG_RX_PW_P0                       0x11
#define NRF24L01_REG_RX_PW_P1                       0x12
#define NRF24L01_REG_RX_PW_P2                       0x13
#define NRF24L01_REG_RX_PW_P3                       0x14
#define NRF24L01_REG_RX_PW_P4                       0x15
#define NRF24L01_REG_RX_PW_P5                       0x16
#define NRF24L01_REG_DYNPD                          0x1C
#define NRF24L01_REG_FEATURE                        0x1D

/*
** CONFIG register flags...
*/
#define NRF24L01_CFG_MASK_RX_DR                     0x40
#define NRF24L01_CFG_MASK_TX_DS                     0x20
#define NRF24L01_CFG_MASK_MAX_RT                    0x10
#define NRF24L01_CFG_ENABLE_CRC                     0x08
#define NRF24L01_CFG_DISABLE_CRC                    0x08
#define NRF24L01_CFG_CRC_1_BYTE                     0x00
#define NRF24L01_CFG_CRC_2_BYTE                     0x04
#define NRF24L01_CFG_POWER_UP                       0x02
#define NRF24L01_CFG_POWER_DOWN                     0x00
#define NRF24L01_CFG_MODE_RX                        0x01
#define NRF24L01_CFG_MODE_TX                        0x00

/*
** RF_SETUP register flags...
*/
#define NRF24L01_RF_SETUP_DATA_RATE_1MBPS           0x00
#define NRF24L01_RF_SETUP_DATA_RATE_2MBPS           0x08
#define NRF24L01_RF_SETUP_RF_POWER_LOWEST           0x00
#define NRF24L01_RF_SETUP_RF_POWER_LOW              0x02
#define NRF24L01_RF_SETUP_RF_POWER_MEDIUM           0x04
#define NRF24L01_RF_SETUP_RF_POWER_HIGH             0x06
#define NRF24L01_RF_SETUP_RF_LNA_GAIN_ON            0x01
#define NRF24L01_RF_SETUP_RF_LNA_GAIN_OFF           0x00

/*
** STATUS register flags...
*/
#define NRF24L01_STATUS_CLEAR_RX_DR                 0x40
#define NRF24L01_STATUS_CLEAR_TX_DS                 0x20
#define NRF24L01_STATUS_CLEAR_MAX_RT                0x10

#define NRF24L01_STATUS_R_RX_PIPE_0                 0x00
#define NRF24L01_STATUS_R_RX_PIPE_1                 0x02
#define NRF24L01_STATUS_R_RX_PIPE_2                 0x04
#define NRF24L01_STATUS_R_RX_PIPE_3                 0x06
#define NRF24L01_STATUS_R_RX_PIPE_4                 0x08
#define NRF24L01_STATUS_R_RX_PIPE_5                 0x0A

#define NRF24L01_STATUS_R_TX_FIFO_FULL              0x01
#define NRF24L01_STATUS_R_RX_FIFO_EMPTY             0x0E
#define NRF24L01_STATUS_R_RX_DR                     0x40

/*
** FEATURE register flags...
*/
#define NRF24L01_FEATURE_EN_DYN_PAYLOAD_LEN         0x04
#define NRF24L01_FEATURE_EN_PAYLOAD_WITH_ACK        0x02
#define NRF24L01_FEATURE_EN_TX_NO_ACK               0x01

class nrf24_error : public exception
{
    private:
        string      message;

    public:
        nrf24_error() {
            this->message.assign("nRF24L01 error");
        }

        nrf24_error(const char * msg) {
            this->message.assign("nRF24L01 error: ");
            this->message.append(msg);
        }

        nrf24_error(const char * msg, const char * file, int line) {
            char lineNumBuf[8];

            snprintf(lineNumBuf, 8, ":%d", line);

            this->message.assign("nRF24L01 error: ");
            this->message.append(msg);
            this->message.append(" at ");
            this->message.append(file);
            this->message.append(lineNumBuf);
        }

        virtual const char * what() const noexcept {
            return this->message.c_str();
        }

        static const char * buildMsg(const char * fmt, ...) {
            va_list     args;
            char *      buffer;

            va_start(args, fmt);
            
            buffer = (char *)malloc(strlen(fmt) + 80);
            
            vsnprintf(buffer, strlen(fmt) + 80, fmt, args);
            
            va_end(args);

            return buffer;
        }
};

class nRF24L01
{
public:
    enum addressWidth {
        aw5Bytes = 0x03,
        aw4Bytes = 0x02,
        aw3Bytes = 0x01
    };

private:
    int                 _hSPI;
    int                 _hGPIO;
    int                 _CEPin;

    enum addressWidth   _aw;

    Logger & log = Logger::getInstance();

    int         _handleSPITransferError(
                    int errorCode,
                    const char * pszMessage,
                    const char * pszSourceFile, 
                    int sourceLine);

    uint8_t     _writeRegister(uint8_t registerID, uint8_t value);
    uint8_t     _writeRegister(uint8_t registerID, uint16_t value);
    uint8_t     _writeRegister(uint8_t registerID, uint8_t * data, uint16_t dataLength);

    void        _readRegister(uint8_t registerID, uint8_t * regValue);
    void        _readRegister(uint8_t registerID, uint16_t * regValue);
    void        _readRegister(uint8_t registerID, uint8_t * data, uint16_t dataLength);

    void        _transmit(uint8_t * data, uint16_t dataLength, bool requestACK);

public:
    nRF24L01(int hSPI, int CEPin);
    ~nRF24L01();

    void        powerUp();
    void        powerDown();

    void        writeConfig(const uint8_t flags);
    void        writeRFSetup(const uint8_t flags);
    void        setAddressWidth(enum addressWidth aw);
    void        setAutoAck(int dataPipe, bool isEnabled);
    void        enableRxAddress(int dataPipe, bool isEnabled);
    void        setupAutoRetransmission(
                        uint16_t retransmissionDelay, 
                        uint16_t retransmissionCount);
    void        setRFChannel(const uint8_t channel);
    void        setRxAddress(int dataPipe, uint8_t * address);
    void        setTxAddress(uint8_t * address);
    void        activateAdditionalFeatures();
    void        enableFeatures(const uint8_t flags);

    bool        isRxDataAvailable();
    bool        isRxDataAvailable(int dataPipe);

    void        receive(uint8_t * buffer);
    void        send(uint8_t * buffer, int bufferLength, bool requestACK);
    void        send(char * pszBuffer, bool requestACK);
};

#endif
