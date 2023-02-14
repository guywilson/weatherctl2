#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
extern "C" {
#include "dummy_lgpio.h"
}
#else
#include <lgpio.h>
#endif

extern "C" {
#include "spi.h"
}

#include "nRF24L01.h"

#define TEMP_MESSAGE_LEN                        256

using namespace std;

static char szTemp[TEMP_MESSAGE_LEN];

void nRF24L01::_setCEPin(bool state) {
    lgGpioWrite(_hGPIO, _CEPin, (state ? 1 : 0));
}

int nRF24L01::_handleSPITransferError(
                    int errorCode,
                    const char * pszMessage,
                    const char * pszSourceFile, 
                    int sourceLine)
{
    if (errorCode < 0) {
        strncpy(szTemp, pszMessage, TEMP_MESSAGE_LEN);
        strncat(szTemp, ": %s", TEMP_MESSAGE_LEN);
        log.logError(szTemp, lguErrorText(errorCode));
        throw nrf24_error(
                nrf24_error::buildMsg(
                                szTemp, 
                                lguErrorText(errorCode)), 
                pszSourceFile, 
                sourceLine);
    }

    return errorCode;
}

uint8_t nRF24L01::_writeRegister(uint8_t registerID, uint8_t value) {
    uint8_t         statusReg;
    int             rtn;

    rtn = spiWriteReadByte(
                _hSPI, 
                NRF24L01_CMD_W_REGISTER | registerID, 
                &statusReg);

    _handleSPITransferError(rtn, "Error issuing write command", __FILE__, __LINE__);

    log.logDebug("nRF24L01::_writeRegister(0x%02X) - STATUS: 0x%02X", registerID, statusReg);

    rtn = spiWriteByte(_hSPI, value);

    _handleSPITransferError(rtn, "Error writing register", __FILE__, __LINE__);

    return statusReg;
}

uint8_t nRF24L01::_writeRegister(uint8_t registerID, uint16_t value) {
    uint8_t         statusReg;
    int             rtn;

    rtn = spiWriteReadByte(
                _hSPI, 
                NRF24L01_CMD_W_REGISTER | registerID, 
                &statusReg);

    _handleSPITransferError(rtn, "Error issuing write command", __FILE__, __LINE__);

    log.logDebug("nRF24L01::_writeRegister(0x%02X) - STATUS: 0x%02X", registerID, statusReg);

    rtn = spiWriteWord(_hSPI, value);

    _handleSPITransferError(rtn, "Error writing register", __FILE__, __LINE__);

    return statusReg;
}

uint8_t nRF24L01::_writeRegister(uint8_t registerID, uint8_t * data, uint16_t dataLength) {
    uint8_t         statusReg;
    int             rtn;

    rtn = spiWriteReadByte(
                _hSPI, 
                NRF24L01_CMD_W_REGISTER | registerID, 
                &statusReg);

    _handleSPITransferError(rtn, "Error issuing write command", __FILE__, __LINE__);

    log.logDebug("nRF24L01::_writeRegister(0x%02X) - STATUS: 0x%02X", registerID, statusReg);

    rtn = spiWriteData(_hSPI, data, dataLength);

    _handleSPITransferError(rtn, "Error writing register", __FILE__, __LINE__);

    return statusReg;
}

void nRF24L01::_readRegister(uint8_t registerID, uint8_t * regValue) {
    uint8_t         statusReg;
    int             rtn;

    rtn = spiWriteReadByte(
                _hSPI, 
                NRF24L01_CMD_R_REGISTER | registerID, 
                &statusReg);

    _handleSPITransferError(rtn, "Error sending read command", __FILE__, __LINE__);

    log.logDebug("nRF24L01::_readRegister(0x%02X) - STATUS: 0x%02X", registerID, statusReg);

    rtn = spiReadByte(_hSPI, regValue);

    _handleSPITransferError(rtn, "Error reading register", __FILE__, __LINE__);
}

void nRF24L01::_readRegister(uint8_t registerID, uint16_t * regValue) {
    uint8_t         statusReg;
    int             rtn;

    rtn = spiWriteReadByte(
                _hSPI, 
                NRF24L01_CMD_R_REGISTER | registerID, 
                &statusReg);

    _handleSPITransferError(rtn, "Error sending read command", __FILE__, __LINE__);

    log.logDebug("nRF24L01::_readRegister(0x%02X) - STATUS: 0x%02X", registerID, statusReg);

    rtn = spiReadWord(_hSPI, regValue);

    _handleSPITransferError(rtn, "Error reading register", __FILE__, __LINE__);
}

void nRF24L01::_readRegister(uint8_t registerID, uint8_t * data, uint16_t dataLength) {
    uint8_t         statusReg;
    int             rtn;

    rtn = spiWriteReadByte(
                _hSPI, 
                NRF24L01_CMD_R_REGISTER | registerID, 
                &statusReg);

    _handleSPITransferError(rtn, "Error sending read command", __FILE__, __LINE__);

    log.logDebug("nRF24L01::_readRegister(0x%02X) - STATUS: 0x%02X", registerID, statusReg);

    rtn = spiReadData(_hSPI, data, dataLength);

    _handleSPITransferError(rtn, "Error reading register", __FILE__, __LINE__);
}

void nRF24L01::_transmit(uint8_t * data, uint16_t dataLength, bool requestACK) {
    uint8_t         statusReg;
    uint8_t         command;
    int             rtn;

   if (requestACK) {
        command = NRF24L01_CMD_W_TX_PAYLOAD;
        log.logDebug("Transmitting %d bytes with ACK", dataLength);
    }
    else {
        command = NRF24L01_CMD_W_TX_PAYLOAD_NOACK;
        log.logDebug("Transmitting %d bytes with no ACK", dataLength);
    }

    rtn = spiWriteReadByte(_hSPI, NRF24L01_CMD_FLUSH_TX, &statusReg);

    _handleSPITransferError(rtn, "Error flushing TX buffer", __FILE__, __LINE__);

    rtn = spiWriteReadByte(_hSPI, command, &statusReg);

    _handleSPITransferError(rtn, "Error writing command", __FILE__, __LINE__);

    rtn = spiWriteData(_hSPI, data, 32);

    _handleSPITransferError(rtn, "Error writing TX data", __FILE__, __LINE__);

    _powerUp(false);

    // sprintf(szTemp, "St: 0x%02X\n", statusReg);
    // uart_puts(uart0, szTemp);

    /*
    ** Pulse the CE line for > 10us to enable 
    ** the device in TX mode to send the data
    */
    _setCEPin(true);

    usleep(11U);

    _setCEPin(false);

    _powerDown();

    /*
    ** Clear the TX_DS bit...
    */
    _readRegister(NRF24L01_REG_STATUS, &statusReg);
    _writeRegister(NRF24L01_REG_STATUS, (uint8_t)(statusReg & 0xDF));

    _handleSPITransferError(rtn, "Error writing status register", __FILE__, __LINE__);
}

nRF24L01::nRF24L01(int hSPI, int CEPin) {
    int             rtn;

    _hGPIO = lgGpiochipOpen(0);

    if (_hGPIO < 0) {
        log.logError("nRF24L01::init() - Failed to open GPIO device: %s", lguErrorText(_hGPIO));
        throw nrf24_error(
                nrf24_error::buildMsg(
                                "nRF24L01::init() - Failed to open GPIO device: %s", 
                                lguErrorText(_hGPIO)),
                __FILE__,
                __LINE__);
    }

    log.logDebug("Opened GPIO device with handle %d", _hGPIO);

    _CEPin = CEPin;

	/*
	** SPI CE
	*/
    rtn = lgGpioClaimOutput(_hGPIO, 0, _CEPin, 0);

    if (rtn < 0) {
        log.logError("nRF24L01::init() - Failed to claim pin %d as output: %s", _CEPin, lguErrorText(_hGPIO));
        throw nrf24_error(
                nrf24_error::buildMsg(
                                "nRF24L01::init() - Failed to claim pin %d as output: %s",
                                _CEPin,
                                lguErrorText(_hGPIO)),
                __FILE__,
                __LINE__);
    }

    log.logDebug("Claimed pin %d for output with return code %d", _CEPin, rtn);

    _setCEPin(false);

    usleep(100000);
}

nRF24L01::~nRF24L01() {
}

void nRF24L01::_powerUp(bool isRx) {
    uint8_t             regCONFIG;

    _readRegister(NRF24L01_REG_CONFIG, &regCONFIG);

    regCONFIG |= NRF24L01_CFG_POWER_UP;

    if (isRx) {
        regCONFIG |= NRF24L01_CFG_MODE_RX;
    }

    _writeRegister(NRF24L01_REG_CONFIG, regCONFIG);

    _setCEPin(true);
}

void nRF24L01::_powerDown() {
    uint8_t             regCONFIG;

    _setCEPin(false);
    
    _readRegister(NRF24L01_REG_CONFIG, &regCONFIG);

    regCONFIG &= ~(NRF24L01_CFG_POWER_UP | NRF24L01_CFG_MODE_RX);

    _writeRegister(NRF24L01_REG_CONFIG, regCONFIG);
}

void nRF24L01::writeConfig(const uint8_t flags) {
    uint8_t             statusReg;
    uint8_t             configReg;

    log.logDebug("nRF24L01::writeConfig() - Writing config register with 0x%02X", flags);

    statusReg = _writeRegister(NRF24L01_REG_CONFIG, flags);

    usleep(2000);

    _readRegister(NRF24L01_REG_CONFIG, &configReg);

    log.logDebug("nRF24L01::writeConfig() - set CONFIG, read back as: 0x%02X, STATUS: 0x%02X", configReg, statusReg);

    statusReg = _writeRegister(
                        NRF24L01_REG_STATUS, 
                        (uint8_t)(NRF24L01_STATUS_CLEAR_RX_DR | 
                        NRF24L01_STATUS_CLEAR_TX_DS | 
                        NRF24L01_STATUS_CLEAR_MAX_RT));

    log.logDebug("nRF24L01::writeConfig() - STATUS: 0x%02X", statusReg);
}

void nRF24L01::writeRFSetup(uint8_t flags) {
    _writeRegister(NRF24L01_REG_RF_SETUP, flags);
}

void nRF24L01::setAddressWidth(enum addressWidth aw) {
    this->_aw = aw;

    _writeRegister(NRF24L01_REG_SETUP_AW, (uint8_t)(aw & 0x00FF));
}

void nRF24L01::setAutoAck(int dataPipe, bool isEnabled) {
    uint8_t             regEN_AA;

    if (dataPipe >= 0 && dataPipe < 6) {
        _readRegister(NRF24L01_REG_EN_AA, &regEN_AA);

        if (isEnabled) {
            regEN_AA |= (1 << dataPipe);
        }
        else {
            regEN_AA &= ~(1 << dataPipe);
        }
    }
    else {
        log.logError("nRF24L01::setAutoAck: Invalid data pipe %d", dataPipe);
        throw nrf24_error("nRF24L01::setAutoAck: Invalid data pipe", __FILE__, __LINE__);
    }
}

void nRF24L01::enableRxAddress(int dataPipe, bool isEnabled) {
    uint8_t             regEN_RXADDR;

    if (dataPipe >= 0 && dataPipe < 6) {
        _readRegister(NRF24L01_REG_EN_RXADDR, &regEN_RXADDR);

        if (isEnabled) {
            regEN_RXADDR |= (1 << dataPipe);
        }
        else {
            regEN_RXADDR &= ~(1 << dataPipe);
        }
    }
    else {
        log.logError("nRF24L01::enableRxAddress: Invalid data pipe %d", dataPipe);
        throw nrf24_error("nRF24L01::enableRxAddress: Invalid data pipe", __FILE__, __LINE__);
    }

    _writeRegister(NRF24L01_REG_EN_RXADDR, regEN_RXADDR);
}

void nRF24L01::setupAutoRetransmission(uint16_t retransmissionDelay, uint16_t retransmissionCount) {
    uint8_t                 regSETUP_RETR = 0x00;

    if (retransmissionDelay <= 4000) {
        regSETUP_RETR |= (uint8_t)(((retransmissionDelay / 250) - 1) << 4);
    }
    else {
        log.logError(
            "nRF24L01::setupAutoRetransmission: Invalid retransmission delay %d", 
            retransmissionDelay);
        throw nrf24_error(
            "nRF24L01::setupAutoRetransmission: Invalid retransmition delay", 
            __FILE__, 
            __LINE__);
    }

    if (retransmissionCount <= 15) {
        regSETUP_RETR |= (uint8_t)(retransmissionCount & 0x000F);
    }
    else {
        log.logError(
            "nRF24L01::setupAutoRetransmission: Invalid retransmission count %d", 
            retransmissionDelay);
        throw nrf24_error(
            "nRF24L01::setupAutoRetransmission: Invalid retransmition count", 
            __FILE__, 
            __LINE__);
    }

    _writeRegister(NRF24L01_REG_SETUP_RETR, regSETUP_RETR);
}

void nRF24L01::setRFChannel(const uint8_t channel) {
    if (channel <= 127) {
        _writeRegister(NRF24L01_REG_RF_CH, (uint8_t)channel);
    }
    else {
        log.logError(
            "nRF24L01::setRFChannel: Invalid channel num %d", 
            channel);
        throw nrf24_error(
            "nRF24L01::setRFChannel: Invalid channel num", 
            __FILE__, 
            __LINE__);
    }
}

void nRF24L01::setRxAddress(int dataPipe, uint8_t * address) {
    uint8_t             statusReg;

    if (dataPipe >= 0 && dataPipe < 6) {
        statusReg = _writeRegister(
                        (uint8_t)(NRF24L01_REG_RX_ADDR_PO + dataPipe), 
                        address, 
                        (uint16_t)(_aw + 2));

        log.logDebug("nRF24L01::setRxAddress() - STATUS reg: 0x%02X", statusReg);
    }
    else {
        log.logError("nRF24L01::setRxAddress: Invalid data pipe %d", dataPipe);
        throw nrf24_error("nRF24L01::setRxAddress: Invalid data pipe", __FILE__, __LINE__);
    }
}

void nRF24L01::setTxAddress(uint8_t * address) {
    uint8_t             statusReg;

    statusReg = _writeRegister(
                    NRF24L01_REG_TX_ADDR, 
                    address, 
                    (uint16_t)(_aw + 2));

    log.logDebug("nRF24L01::setTxAddress() - STATUS reg: 0x%02X", statusReg);
}

void nRF24L01::setRxPayloadSize(int dataPipe, uint8_t payloadSize) {
    uint8_t             statusReg;

    if (dataPipe >= 0 && dataPipe < 6) {
        if (payloadSize > 32) {
            log.logError("nRF24L01::setRxPayloadSize: Invalid payload size %d", (int)payloadSize);
            throw nrf24_error("nRF24L01::setRxPayloadSize: Invalid payload size", __FILE__, __LINE__);
        }

        statusReg = _writeRegister(
                        (uint8_t)(NRF24L01_REG_RX_PW_P0 + dataPipe), 
                        payloadSize);

        log.logDebug("nRF24L01::setRxPayloadSize() - STATUS reg: 0x%02X", statusReg);
    }
    else {
        log.logError("nRF24L01::setRxPayloadSize: Invalid data pipe %d", dataPipe);
        throw nrf24_error("nRF24L01::setRxPayloadSize: Invalid data pipe", __FILE__, __LINE__);
    }
}

void nRF24L01::activateAdditionalFeatures() {
    uint8_t             statusReg;
    int                 rtn;

    rtn = spiWriteReadByte(_hSPI, NRF24L01_CMD_ACTIVATE, &statusReg);

    _handleSPITransferError(rtn, "Error sending activate cmd", __FILE__, __LINE__);

    log.logDebug("nRF24L01::activateAdditionalFeatures() - STATUS reg: 0x%02X", statusReg);

    rtn = spiWriteByte(_hSPI, NRF24L01_ACTIVATE_ADDITIONAL);

    _handleSPITransferError(rtn, "Error writing activate byte", __FILE__, __LINE__);
}

void nRF24L01::enableFeatures(const uint8_t flags) {
    _writeRegister(NRF24L01_REG_FEATURE, flags);
}

bool nRF24L01::isRxDataAvailable() {
    uint8_t         statusReg;
    bool            isRx;

    _readRegister(NRF24L01_REG_STATUS, &statusReg);

    isRx = (!(statusReg & NRF24L01_STATUS_R_RX_FIFO_EMPTY) && (statusReg & NRF24L01_STATUS_R_RX_DR)) ? true : false;

    return isRx;
}

bool nRF24L01::isRxDataAvailable(int dataPipe) {
    uint8_t         statusReg;
    bool            isRx;

    _readRegister(NRF24L01_REG_STATUS, &statusReg);

    if (statusReg & NRF24L01_STATUS_R_RX_FIFO_EMPTY) {
        isRx = false;
    }
    else {
        isRx = (statusReg & (dataPipe << 1)) ? true : false;
    }

    return isRx;
}

void nRF24L01::startListening() {
    _powerUp(true);
}

void nRF24L01::stopListening() {
    _powerDown();
}

void nRF24L01::receive(uint8_t * buffer) {
    uint8_t             statusReg;

    while (!isRxDataAvailable()) {
        usleep(100);
    }

    spiWriteReadByte(_hSPI, NRF24L01_CMD_R_RX_PAYLOAD, &statusReg);
    spiReadData(_hSPI, buffer, 32);
}

void nRF24L01::send(uint8_t * buffer, int bufferLength, bool requestACK) {
    uint8_t         buf[32];

    if (bufferLength > 32) {
        log.logError("nRF24L01::send() - Buffer length too long: %d", bufferLength);
        throw nrf24_error("nRF24L01::send() - Buffer length too long", __FILE__, __LINE__);
    }

    memset(buf, 0, 32);
    memcpy(buf, buffer, bufferLength);

    _transmit(buf, 32, requestACK);
}

void nRF24L01::send(char * pszBuffer, bool requestACK) {
    int             strLength;
    uint8_t         buf[32];

    strLength = strlen(pszBuffer);

    if (strLength > 32) {
        log.logInfo(
            "nRF24L01::send() - Shortening tx str to 32 bytes, %d bytes truncated", 
            (strLength - 32));
        strLength = 32;
    }

    memset(buf, 0, 32);
    memcpy(buf, pszBuffer, strLength);

    _transmit(buf, 32, requestACK);
}
