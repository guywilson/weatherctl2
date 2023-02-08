#include <stdint.h>

#ifndef __INCL_DUMMY_LGPIO
#define __INCL_DUMMY_LGPIO

typedef struct {
    int a;
}
lgChipInfo_p;

typedef struct {
    int a;
}
lgLineInfo_p;


typedef struct {
    int a;
}
lgPulse_p;

typedef struct {
    int a;
}
lgI2cMsg_t;

int lgGpiochipOpen(int gpioDev);
int lgGpiochipClose(int handle);
int lgGpioGetChipInfo(int handle, lgChipInfo_p chipInfo);
int lgGpioGetLineInfo(int handle, int gpio, lgLineInfo_p lineInfo);
int lgGpioGetMode(int handle, int gpio);
int lgGpioSetUser(int handle, const char *gpiouser);
int lgGpioClaimInput(int handle, int lFlags, int gpio);
int lgGpioClaimOutput(int handle, int lFlags, int gpio, int level);
int lgGpioClaimAlert(int handle, int lFlags, int eFlags, int gpio, int nfyHandle);
int lgGpioFree(int handle, int gpio);
int lgGroupClaimInput(int handle, int lFlags, int count, const int *gpios);
int lgGroupClaimOutput(int handle, int lFlags, int count, const int *gpios, const int *levels);
int lgGroupFree(int handle, int gpio);
int lgGpioRead(int handle, int gpio);
int lgGpioWrite(int handle, int gpio, int level);
int lgGroupRead(int handle, int gpio, uint64_t *groupBits);
int lgGroupWrite(int handle, int gpio, uint64_t groupBits, uint64_t groupMask);
int lgTxPulse(int handle, int gpio, int pulseOn, int pulseOff, int pulseOffset, int pulseCycles);
int lgTxPwm(int handle, int gpio, float pwmFrequency, float pwmDutyCycle, int pwmOffset, int pwmCycles);
int lgTxServo(int handle, int gpio, int pulseWidth, int servoFrequency, int servoOffset, int servoCycles);
int lgTxWave(int handle, int gpio, int count, lgPulse_p pulses);
int lgTxBusy(int handle, int gpio, int kind);
int lgTxRoom(int handle, int gpio, int kind);
int lgGpioSetDebounce(int handle, int gpio, int debounce_us);
int lgGpioSetWatchdog(int handle, int gpio, int watchdog_us);
//int lgGpioSetAlertsFunc(int handle, int gpio, lgGpioAlertsFunc_t cbf, void *userdata);
//void lgGpioSetSamplesFunc(lgGpioAlertsFunc_t cbf, void *userdata);
int lgNotifyOpen(void);
int lgNotifyResume(int handle);
int lgNotifyPause(int handle);
int lgNotifyClose(int handle);
int lgI2cOpen(int i2cDev, int i2cAddr, int i2cFlags);
int lgI2cClose(int handle);
int lgI2cWriteQuick(int handle, int bitVal);
int lgI2cWriteByte(int handle, int byteVal);
int lgI2cReadByte(int handle);
int lgI2cWriteByteData(int handle, int i2cReg, int byteVal);
int lgI2cWriteWordData(int handle, int i2cReg, int wordVal);
int lgI2cReadByteData(int handle, int i2cReg);
int lgI2cReadWordData(int handle, int i2cReg);
int lgI2cProcessCall(int handle, int i2cReg, int wordVal);
int lgI2cWriteBlockData(int handle, int i2cReg, const char *txBuf, int count);
int lgI2cReadBlockData(int handle, int i2cReg, char *rxBuf);
int lgI2cBlockProcessCall(int handle, int i2cReg, char *ioBuf, int count);
int lgI2cReadI2CBlockData(int handle, int i2cReg, char *rxBuf, int count);
int lgI2cWriteI2CBlockData(int handle, int i2cReg, const char *txBuf, int count);
int lgI2cReadDevice(int handle, char *rxBuf, int count);
int lgI2cWriteDevice(int handle, const char *txBuf, int count);
int lgI2cSegments(int handle, lgI2cMsg_t *segs, int count);
int lgI2cZip(int handle, const char *txBuf, int txCount, char *rxBuf, int rxCount);
int lgSerialOpen(const char *serDev, int serBaud, int serFlags);
int lgSerialClose(int handle);
int lgSerialWriteByte(int handle, int byteVal);
int lgSerialReadByte(int handle);
int lgSerialWrite(int handle, const char *txBuf, int count);
int lgSerialRead(int handle, char *rxBuf, int count);
int lgSerialDataAvailable(int handle);
int lgSpiOpen(int spiDev, int spiChan, int spiBaud, int spiFlags);
int lgSpiClose(int handle);
int lgSpiRead(int handle, char *rxBuf, int count);
int lgSpiWrite(int handle, const char *txBuf, int count);
int lgSpiXfer(int handle, const char *txBuf, char *rxBuf, int count);
//pthread_t *lgThreadStart(lgThreadFunc_t f, void *userdata);
//void lgThreadStop(pthread_t *pth);
uint64_t lguTimestamp(void);
double lguTime(void);
void lguSleep(double sleepSecs);
int lguSbcName(char *rxBuf, int count);
int lguVersion(void);
int lguGetInternal(int cfgId, uint64_t *cfgVal);
int lguSetInternal(int cfgId, uint64_t cfgVal);
const char *lguErrorText(int error);
void lguSetWorkDir(const char *dirPath);
const char *lguGetWorkDir(void);

#endif
