#include <iostream>
#include <cstring>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#ifdef __APPLE__
extern "C" {
#include "dummy_lgpio.h"
}
#else
#include <lgpio.h>
#endif

#include <strutils.h>

#include "wctl_error.h"
#include "currenttime.h"
#include "logger.h"
#include "configmgr.h"
#include "posixthread.h"
#include "threads.h"
#include "nRF24L01.h"

extern "C" {
	#include "version.h"
}

using namespace std;

void printUsage()
{
	printf("\n Usage: wctl [OPTIONS]\n\n");
	printf("  Options:\n");
	printf("   -h/?             Print this help\n");
	printf("   -version         Print the program version\n");
	printf("   -port device     Serial port device\n");
	printf("   -baud baudrate   Serial port baud rate\n");
	printf("   -cfg configfile  Specify the cfg file, default is ./webconfig.cfg\n");
	printf("   -d               Daemonise this application\n");
	printf("   -log  filename   Write logs to the file\n");
	printf("\n");
}

int read_register(int hSPI, int reg, char *rxBuf, int count)
{
	int i;
	char txBuf[64];
	char buf[64];

	txBuf[0] = NRF24L01_CMD_R_REGISTER | reg;
	
	for (i = 1; i <= count; i++) {
		txBuf[i] = 0;
	}

	i = lgSpiXfer(hSPI, txBuf, buf, count+1);

	if (i >= 0) {
		for (i = 0; i < count; i++) {
			rxBuf[i] = buf[i+1];
		}

		return count;
	}

	return i;
}

int write_register(int hSPI, int reg, char *txBuf, int count)
{
	int i;
	char buf[64];
	char rxBuf[64];

	buf[0] = NRF24L01_CMD_W_REGISTER | reg;

	for (i = 0; i < count; i++) {
		buf[i + 1] = txBuf[i];
	}

	i = lgSpiXfer(hSPI, buf, rxBuf, count + 1);

	return i;
}

int setupRadio() {
    int                 CEPin;
	int					spiPort;
	int					spiChannel;
	int					spiFreq;
    char                txBuffer[64];
    char                rxBuffer[64];

	ConfigManager & cfg = ConfigManager::getInstance();
	Logger & log = Logger::getInstance();

	spiPort = cfg.getValueAsInteger("radio.spiport");
	spiChannel = cfg.getValueAsInteger("radio.spichannel");
	spiFreq = cfg.getValueAsInteger("radio.spifreq");

	log.logDebug("Opening SPI device %d on channel %d with clk freq %d", spiPort, spiChannel, spiFreq);

    int hspi = lgSpiOpen(
					spiPort, 
					spiChannel, 
					spiFreq, 
					0);

    if (hspi < 0) {
        log.logError("Failed to open SPI device: %s", lguErrorText(hspi));
		return -1;
    }

    log.logDebug("Setting up nRF24L01 device...");

    int hGPIO = lgGpiochipOpen(0);

    if (hGPIO < 0) {
        log.logError("Failed to open GPIO device: %s", lguErrorText(hGPIO));
        return -1;
    }

    CEPin = cfg.getValueAsInteger("radio.spicepin");

    int rtn = lgGpioClaimOutput(hGPIO, 0, CEPin, 0);

    if (rtn < 0) {
        log.logError(
            "Failed to claim pin %d as output: %s", 
            CEPin, 
            lguErrorText(rtn));

        return -1;
    }

    log.logDebug("Claimed pin %d for output with return code %d", CEPin, rtn);

	PosixThread::sleep(PosixThread::milliseconds, 100);

	txBuffer[0] = 40;

	rtn = write_register(hspi, NRF24L01_REG_RF_CH, txBuffer, 1);

    if (rtn < 0) {
        log.logError(
            "Failed to transfer SPI data: %s", 
            lguErrorText(rtn));

        return -1;
    }

	rtn = read_register(hspi, NRF24L01_REG_RF_CH, rxBuffer, 1);

    if (rtn < 0) {
        log.logError(
            "Failed to transfer SPI data: %s", 
            lguErrorText(rtn));

        return -1;
    }

    log.logDebug("Read back RF_CH reg: %d", (int)rxBuffer[0]);

	txBuffer[0] = NRF24L01_CFG_ENABLE_CRC | NRF24L01_CFG_CRC_2_BYTE;

	rtn = write_register(hspi, NRF24L01_REG_CONFIG, txBuffer, 1);

    if (rtn < 0) {
        log.logError(
            "Failed to transfer SPI data: %s", 
            lguErrorText(rtn));

        return -1;
    }

	PosixThread::sleep(PosixThread::milliseconds, 500);

	rtn = read_register(hspi, NRF24L01_REG_CONFIG, rxBuffer, 1);

    if (rtn < 0) {
        log.logError(
            "Failed to transfer SPI data: %s", 
            lguErrorText(rtn));

        return -1;
    }

    log.logDebug("Read back CONFIG reg: 0x%02X", (int)rxBuffer[0]);

    lgSpiClose(hspi);
    lgGpioFree(hGPIO, CEPin);
    lgGpiochipClose(hGPIO);

	return 0;
}

int main(int argc, char ** argv) {
	char *			pszLogFileName = NULL;
	char *			pszConfigFileName = NULL;
	int				i;
	bool			isDaemonised = false;
	bool			isDumpConfig = false;
	int				defaultLoggingLevel = LOG_LEVEL_INFO | LOG_LEVEL_ERROR | LOG_LEVEL_FATAL;

	CurrentTime::initialiseUptimeClock();
	
	if (argc > 1) {
		for (i = 1;i < argc;i++) {
			if (argv[i][0] == '-') {
				if (argv[i][1] == 'd') {
					isDaemonised = true;
				}
				else if (strcmp(&argv[i][1], "log") == 0) {
					pszLogFileName = strdup(&argv[++i][0]);
				}
				else if (strcmp(&argv[i][1], "cfg") == 0) {
					pszConfigFileName = strdup(&argv[++i][0]);
				}
				else if (strcmp(&argv[i][1], "-dump-config") == 0) {
					isDumpConfig = true;
				}
				else if (argv[i][1] == 'h' || argv[i][1] == '?') {
					printUsage();
					return 0;
				}
				else if (strcmp(&argv[i][1], "version") == 0) {
					printf("%s Version: [wctl], Build date: [%s]\n\n", getVersion(), getBuildDate());
					return 0;
				}
				else {
					printf("Unknown argument '%s'", &argv[i][0]);
					printUsage();
					return 0;
				}
			}
		}
	}
	else {
		printUsage();
		return -1;
	}

	if (isDaemonised) {
//		daemonise();
	}

	ConfigManager & cfg = ConfigManager::getInstance();
	
	try {
		cfg.initialise(pszConfigFileName);
	}
	catch (wctl_error & e) {
		fprintf(stderr, "Could not read config file: %s [%s]\n", pszConfigFileName, e.what());
		fprintf(stderr, "Aborting!\n\n");
		fflush(stderr);
		exit(EXIT_FAILURE);
	}

	if (pszConfigFileName != NULL) {
		free(pszConfigFileName);
	}

	if (isDumpConfig) {
		cfg.dumpConfig();
	}

	Logger & log = Logger::getInstance();

	if (pszLogFileName != NULL) {
		log.initLogger(pszLogFileName, defaultLoggingLevel);
		free(pszLogFileName);
	}
	else {
		const char * filename = cfg.getValue("log.filename");
		const char * level = cfg.getValue("log.level");

		if (strlen(filename) == 0 && strlen(level) == 0) {
			log.initLogger(defaultLoggingLevel);
		}
		else if (strlen(level) == 0) {
			log.initLogger(filename, defaultLoggingLevel);
		}
		else {
			log.initLogger(filename, level);
		}
	}

// #ifndef __APPLE__
//     RadioRxThread * rxThread = new RadioRxThread();
//     rxThread->start();
// #endif

//     while (1) {
//         PosixThread::sleep(PosixThread::seconds, 5U);
//

	setupRadio();

    return 0;
}
