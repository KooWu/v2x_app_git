#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <stdint.h>

typedef void (*ThreadPoolJobFunc)(void *arg);

typedef struct {
	ThreadPoolJobFunc jobFunc;
	void *arg;
} ThreadPoolJobInfo;

int32_t ThreadPoolInit(int32_t threadNum);
void ThreadPoolDeinit(void);
int32_t ThreadPoolAddJob(ThreadPoolJobFunc jobFunc, void *arg);

#endif