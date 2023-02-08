#include <stdint.h>
#include <stdio.h>
#include "dummy_lgpio.h"

int lgGpiochipOpen(int gpioDev) {
    return 0;
}
int lgGpiochipClose(int handle) {
    return 0;
}
int lgGpioGetChipInfo(int handle, lgChipInfo_p chipInfo) {
    return 0;
}
int lgGpioGetLineInfo(int handle, int gpio, lgLineInfo_p lineInfo) {
    return 0;
}
int lgGpioGetMode(int handle, int gpio) {
    return 0;
}
int lgGpioSetUser(int handle, const char *gpiouser) {
    return 0;
}
int lgGpioClaimInput(int handle, int lFlags, int gpio) {
    return 0;
}
int lgGpioClaimOutput(int handle, int lFlags, int gpio, int level) {
    return 0;
}
int lgGpioClaimAlert(int handle, int lFlags, int eFlags, int gpio, int nfyHandle) {
    return 0;
}
int lgGpioFree(int handle, int gpio) {
    return 0;
}
int lgGroupClaimInput(int handle, int lFlags, int count, const int *gpios) {
    return 0;
}
int lgGroupClaimOutput(int handle, int lFlags, int count, const int *gpios, const int *levels) {
    return 0;
}
int lgGroupFree(int handle, int gpio) {
    return 0;
}
int lgGpioRead(int handle, int gpio) {
    return 0;
}
int lgGpioWrite(int handle, int gpio, int level) {
    return 0;
}
int lgGroupRead(int handle, int gpio, uint64_t *groupBits) {
    return 0;
}
int lgGroupWrite(int handle, int gpio, uint64_t groupBits, uint64_t groupMask) {
    return 0;
}
int lgTxPulse(int handle, int gpio, int pulseOn, int pulseOff, int pulseOffset, int pulseCycles) {
    return 0;
}
int lgTxPwm(int handle, int gpio, float pwmFrequency, float pwmDutyCycle, int pwmOffset, int pwmCycles) {
    return 0;
}
int lgTxServo(int handle, int gpio, int pulseWidth, int servoFrequency, int servoOffset, int servoCycles) {
    return 0;
}
int lgTxWave(int handle, int gpio, int count, lgPulse_p pulses) {
    return 0;
}
int lgTxBusy(int handle, int gpio, int kind) {
    return 0;
}
int lgTxRoom(int handle, int gpio, int kind) {
    return 0;
}
int lgGpioSetDebounce(int handle, int gpio, int debounce_us) {
    return 0;
}
int lgGpioSetWatchdog(int handle, int gpio, int watchdog_us) {
    return 0;
}
// int lgGpioSetAlertsFunc(int handle, int gpio, lgGpioAlertsFunc_t cbf, void *userdata) {
//     return 0;
// }
// void lgGpioSetSamplesFunc(lgGpioAlertsFunc_t cbf, void *userdata) {
//     return 0;
// }
int lgNotifyOpen(void) {
    return 0;
}
int lgNotifyResume(int handle) {
    return 0;
}
int lgNotifyPause(int handle) {
    return 0;
}
int lgNotifyClose(int handle) {
    return 0;
}
int lgI2cOpen(int i2cDev, int i2cAddr, int i2cFlags) {
    return 0;
}
int lgI2cClose(int handle) {
    return 0;
}
int lgI2cWriteQuick(int handle, int bitVal) {
    return 0;
}
int lgI2cWriteByte(int handle, int byteVal) {
    return 0;
}
int lgI2cReadByte(int handle) {
    return 0;
}
int lgI2cWriteByteData(int handle, int i2cReg, int byteVal) {
    return 0;
}
int lgI2cWriteWordData(int handle, int i2cReg, int wordVal) {
    return 0;
}
int lgI2cReadByteData(int handle, int i2cReg) {
    return 0;
}
int lgI2cReadWordData(int handle, int i2cReg) {
    return 0;
}
int lgI2cProcessCall(int handle, int i2cReg, int wordVal) {
    return 0;
}
int lgI2cWriteBlockData(int handle, int i2cReg, const char *txBuf, int count) {
    return 0;
}
int lgI2cReadBlockData(int handle, int i2cReg, char *rxBuf) {
    return 0;
}
int lgI2cBlockProcessCall(int handle, int i2cReg, char *ioBuf, int count) {
    return 0;
}
int lgI2cReadI2CBlockData(int handle, int i2cReg, char *rxBuf, int count) {
    return 0;
}
int lgI2cWriteI2CBlockData(int handle, int i2cReg, const char *txBuf, int count) {
    return 0;
}
int lgI2cReadDevice(int handle, char *rxBuf, int count) {
    return 0;
}
int lgI2cWriteDevice(int handle, const char *txBuf, int count) {
    return 0;
}
int lgI2cSegments(int handle, lgI2cMsg_t *segs, int count) {
    return 0;
}
int lgI2cZip(int handle, const char *txBuf, int txCount, char *rxBuf, int rxCount) {
    return 0;
}
int lgSerialOpen(const char *serDev, int serBaud, int serFlags) {
    return 0;
}
int lgSerialClose(int handle) {
    return 0;
}
int lgSerialWriteByte(int handle, int byteVal) {
    return 0;
}
int lgSerialReadByte(int handle) {
    return 0;
}
int lgSerialWrite(int handle, const char *txBuf, int count) {
    return 0;
}
int lgSerialRead(int handle, char *rxBuf, int count) {
    return 0;
}
int lgSerialDataAvailable(int handle) {
    return 0;
}
int lgSpiOpen(int spiDev, int spiChan, int spiBaud, int spiFlags) {
    return 0;
}
int lgSpiClose(int handle) {
    return 0;
}
int lgSpiRead(int handle, char *rxBuf, int count) {
    return 0;
}
int lgSpiWrite(int handle, const char *txBuf, int count) {
    return 0;
}
int lgSpiXfer(int handle, const char *txBuf, char *rxBuf, int count) {
    return 0;
}
// pthread_t *lgThreadStart(lgThreadFunc_t f, void *userdata) {
//     return 0;
// }
// void lgThreadStop(pthread_t *pth) {
//     return 0;
// }
uint64_t lguTimestamp(void) {
    return 0U;
}
double lguTime(void) {
    return 0.0;
}
void lguSleep(double sleepSecs) {
    return;
}
int lguSbcName(char *rxBuf, int count) {
    return 0;
}
int lguVersion(void) {
    return 0;
}
int lguGetInternal(int cfgId, uint64_t *cfgVal) {
    return 0;
}
int lguSetInternal(int cfgId, uint64_t cfgVal) {
    return 0;
}
const char *lguErrorText(int error) {
    return "Dummy error!";
}
void lguSetWorkDir(const char *dirPath) {
    return;
}
const char *lguGetWorkDir(void) {
    return "/some/dir";
}
