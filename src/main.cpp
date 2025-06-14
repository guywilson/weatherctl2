#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include <lgpio.h>

extern "C" {
#include "version.h"
}

#include "cfgmgr.h"
#include "logger.h"
#include "posixthread.h"
#include "threads.h"
#include "radio.h"
#include "utils.h"

void printUsage(void) {
	printf("\n Usage: wctl [OPTIONS]\n\n");
	printf("  Options:\n");
	printf("   -h/?             Print this help\n");
	printf("   -version         Print the program version\n");
	printf("   -cfg configfile  Specify the cfg file, default is ./webconfig.cfg\n");
    printf("   --dump-config    Dump the config contents and exit\n");
	printf("   -d               Daemonise this application\n");
	printf("   -log  filename   Write logs to the file\n");
	printf("\n");
}

bool logFileExists(const string & logFileName) {
	FILE * fp = fopen(logFileName.c_str(), "r");

	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}

void getDateString(char * dateBuffer, int bufferLength) {
	time_t t = time(NULL);

	struct tm * tmInfo = localtime(&t);

	strftime(dateBuffer, bufferLength, "%Y%m%d", tmInfo);
}

int archiveLogFile(const string & logFileName) {
	FILE * fpIn = fopen(logFileName.c_str(), "r");

	if (fpIn == NULL) {
		return - 1;
	}

	char dateBuffer[32];
	getDateString(dateBuffer, 32);

	string archiveFileName = logFileName + "_" + dateBuffer;

	FILE * fpOut = fopen(archiveFileName.c_str(), "w");

	if (fpOut == NULL) {
		return -1;
	}

	uint8_t buffer[4096];

	while (!feof(fpIn)) {
		int bytesRead = fread(buffer, sizeof(uint8_t), 4096, fpIn);
		fwrite(buffer, sizeof(uint8_t), bytesRead, fpOut);
	}

	fclose(fpIn);
	fclose(fpOut);

	return 0;
}

void handleSignal(int sigNum) {
	logger & log = logger::getInstance();

	switch (sigNum) {
		case SIGINT:
			log.logStatus("Detected SIGINT, cleaning up...");
			break;

		case SIGTERM:
			log.logStatus("Detected SIGTERM, cleaning up...");
			break;

		case SIGUSR1:
			/*
			** We're interpreting this as a request to turn on/off debug logging...
			*/
			log.logStatus("Detected SIGUSR1...");

			if (log.isLogLevel(LOG_LEVEL_INFO)) {
				int level = log.getLogLevel();
				level &= ~LOG_LEVEL_INFO;
				log.setLogLevel(level);
			}
			else {
				int level = log.getLogLevel();
				level |= LOG_LEVEL_INFO;
				log.setLogLevel(level);
			}

			if (log.isLogLevel(LOG_LEVEL_DEBUG)) {
				int level = log.getLogLevel();
				level &= ~LOG_LEVEL_DEBUG;
				log.setLogLevel(level);
			}
			else {
				int level = log.getLogLevel();
				level |= LOG_LEVEL_DEBUG;
				log.setLogLevel(level);
			}
			return;
	}

    puts("\n");
    
	ThreadManager & threadMgr = ThreadManager::getInstance();
    threadMgr.kill();
    
	nrf24l01 & radio = nrf24l01::getInstance();
	radio.close();

    log.closelogger();

    exit(0);
}

int main(int argc, char ** argv) {
	char *			    pszLogFileName = NULL;
	char *			    pszConfigFileName = NULL;
	int				    i;
	bool			    isDaemonised = false;
	bool			    isDumpConfig = false;
	const char *	    defaultLoggingLevel = "LOG_LEVEL_INFO | LOG_LEVEL_ERROR | LOG_LEVEL_FATAL";

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
		daemonise();
	}

	cfgmgr & cfg = cfgmgr::getInstance();

	try {
		cfg.initialise(pszConfigFileName);
	}
	catch (cfg_error & e) {
		fprintf(stderr, "Could not open configuration file '%s':'%s'\n", pszConfigFileName, e.what());
		exit(-1);
	}

	if (pszConfigFileName != NULL) {
		free(pszConfigFileName);
	}

	if (isDumpConfig) {
        cfg.dumpConfig();
        return 0;
	}

	logger & log = logger::getInstance();

	try {
		if (pszLogFileName != NULL) {
			if (logFileExists(pszLogFileName)) {
				archiveLogFile(pszLogFileName);
			}

			log.initlogger(pszLogFileName, defaultLoggingLevel);
			free(pszLogFileName);
		}
		else {
			string filename = cfg.getValue("log.filename");
			string level = cfg.getValue("log.level");
	
			if (logFileExists(filename) && filename.length() > 0) {
				archiveLogFile(filename);
			}

			if (filename.length() == 0 && level.length() == 0) {
				log.initlogger(defaultLoggingLevel);
			}
			else if (level.length() == 0) {
				log.initlogger(filename, defaultLoggingLevel);
			}
			else {
				log.initlogger(filename, level.c_str());
			}
		}
	}
	catch (log_error & e) {
		fprintf(stderr, "Could not initialise logger");
		exit(-1);
	}

	/*
	 * Register signal handler for cleanup...
	 */
	if (signal(SIGINT, &handleSignal) == SIG_ERR) {
		log.logFatal("Failed to register signal handler for SIGINT");
		return -1;
	}

	if (signal(SIGTERM, &handleSignal) == SIG_ERR) {
		log.logFatal("Failed to register signal handler for SIGTERM");
		return -1;
	}

	if (signal(SIGUSR1, &handleSignal) == SIG_ERR) {
		log.logFatal("Failed to register signal handler for SIGUSR1");
		return -1;
	}

	if (signal(SIGUSR2, &handleSignal) == SIG_ERR) {
		log.logFatal("Failed to register signal handler for SIGUSR2");
		return -1;
	}

	ThreadManager & threadMgr = ThreadManager::getInstance();
	threadMgr.start();

    while (1) {
        PosixThread::sleep(5);
    }

	return 0;
}
