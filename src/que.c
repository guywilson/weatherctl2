#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "cfgmgr.h"
#include "logger.h"
#include "que.h"

static pthread_mutex_t      _mutex = PTHREAD_MUTEX_INITIALIZER;

int qInit(que_handle_t * hque, uint32_t size) {
    hque->pQueue = (que_item_t *)malloc(sizeof(que_item_t) * size);

    if (hque->pQueue == NULL) {
        lgLogFatal(lgGetHandle(), "qInit() - Failed to allocate queue memory of size %u", size);
        return -1;
    }

    lgLogInfo(lgGetHandle(), "qInit() - Instantiated queue with %u items", size);
    
    hque->queueLength = size;

    hque->headIndex = 0;
    hque->tailIndex = 0;
    hque->numItems = 0;

    return 0;
}

void qDestroy(que_handle_t * hque) {
    free(hque->pQueue);
}

uint32_t qGetQueLength(que_handle_t * hque) {
    uint32_t            length;

	pthread_mutex_lock(&_mutex);
    length = hque->queueLength;
	pthread_mutex_unlock(&_mutex);

    return length;
}

uint32_t qGetNumItems(que_handle_t * hque) {
    uint32_t            numItems;

	pthread_mutex_lock(&_mutex);
    numItems = hque->numItems;
	pthread_mutex_unlock(&_mutex);

    return numItems;
}

que_item_t * qGetItem(que_handle_t * hque, que_item_t * item) {
	pthread_mutex_lock(&_mutex);

    if (hque->numItems == 0) {
        lgLogError(lgGetHandle(), "qGetItem() - Queue is empty");
        return NULL;
    }

    memcpy(item, &hque->pQueue[hque->headIndex++], sizeof(que_item_t));
    hque->numItems--;

    if (hque->headIndex == hque->queueLength) {
        lgLogDebug(lgGetHandle(), "qGetItem() - Head wrap around");
        hque->headIndex = 0;
    }

	pthread_mutex_unlock(&_mutex);

    return item;
}

int qPutItem(que_handle_t * hque, que_item_t item) {
	pthread_mutex_lock(&_mutex);

    if (hque->numItems == hque->queueLength) {
        lgLogError(lgGetHandle(), "qPutItem() - Queue is full");
        return -1;
    }

    memcpy(&hque->pQueue[hque->tailIndex++], &item, sizeof(que_item_t));
    hque->numItems++;

    if (hque->tailIndex == hque->queueLength) {
        lgLogDebug(lgGetHandle(), "qPutItem() - Tail wrap around");
        hque->tailIndex = 0;
    }

	pthread_mutex_unlock(&_mutex);

    return 0;
}
