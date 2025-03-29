#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "logger.h"
#include "posixthread.h"

static void * _threadRunner(void * pThreadArgs) {
	void * pThreadRtn = NULL;

	PosixThread * pThread = (PosixThread *)pThreadArgs;

	logger & log = logger::getInstance();

	bool go = true;

	while (go) {
		try {
			pThreadRtn = pThread->run();
		}
		catch (thread_error & e) {
			log.logError("_threadRunner: Caught exception %s", e.what());
		}

		if (pThread->isRestartable) {
			log.logStatus("Restarting thread...");
			PosixThread::sleep(1);
		}
		else {
			go = false;
		}
	}

	return pThreadRtn;
}

bool PosixThread::start() {
	return this->start(NULL);
}

bool PosixThread::start(void * p) {
	int			err;

	this->threadParameters = p;

	err = pthread_create(&this->tid, NULL, &_threadRunner, this);

	if (err != 0) {
		log.logError("ERROR! Can't create thread :[%s]", strerror(err));
		return false;
	}

	return true;
}

void PosixThread::stop() {
	pthread_kill(this->tid, SIGKILL);
}
