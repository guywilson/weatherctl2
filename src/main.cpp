#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <lgpio.h>

extern "C" {
#include "version.h"
}

#include "cfgmgr.h"
#include "logger.h"
#include "posixthread.h"
#include "nRF24L01.h"
#include "NRF24.h"
#include "threads.h"
#include "utils.h"

static nrf_t               nrf;

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
    
    stopThreads();
    
    NRF_term(&nrf);
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
		fprintf(stderr, "Could not open configuration file %s\n", pszConfigFileName);
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
			log.initlogger(pszLogFileName, defaultLoggingLevel);
			free(pszLogFileName);
		}
		else {
			string filename = cfg.getValue("log.filename");
			string level = cfg.getValue("log.level");
	
			if (filename.length() == 0 && level.length() == 0) {
				log.initlogger(defaultLoggingLevel);
			}
			else if (level.length() == 0) {
				log.initlogger(filename, defaultLoggingLevel);
			}
			else {
				log.initlogger(filename.c_str(), level.c_str());
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

    setupNRF24L01();

    startThreads();

    while (1) {
        pxtSleep(seconds, 5);
    }

	return 0;
}
