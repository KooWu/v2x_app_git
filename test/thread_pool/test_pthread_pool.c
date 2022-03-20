#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	int32_t a;
	float b;
} TestStructInfo;

static void TestStructFunc(void *arg)
{
	if (arg == NULL) {
		printf("arg is NULL\n");
		return;
	}
	TestStructInfo *num = (TestStructInfo *)arg;
	printf("TestIntFunc work: %d, %f\n", num->a, num->b);
	sleep(3);
}

static void TestIntFunc(void *arg)
{
	if (arg == NULL) {
		printf("arg is NULL\n");
		return;
	}
	int32_t num = *(int32_t *)arg;
	printf("TestIntFunc work: %d\n", num);
	sleep(3);
}

int32_t EnterTestThreadPool(void)
{
	int32_t threadNum;
	printf("please input thread num: \n");
	scanf("%d", &threadNum);
	int32_t ret = ThreadPoolInit(threadNum);
	if (ret != 0){
		printf("ThreadPoolInit failed\n");
		return -1;
	}
	
	TestStructInfo test;
	test.a = 10;
	test.b = 2.3;
	
	int32_t sleepTime;
	int32_t cycleCnt;
	printf("please input sleep time and cycle cnt: \n");
	scanf("%d %d", &sleepTime, &cycleCnt);
	int32_t idx;
	for (idx = 0; idx < cycleCnt; idx++) {
		sleep(1);
		if (idx % 2 == 0) {
			ThreadPoolAddJob(TestIntFunc, &threadNum);
		} else {
			ThreadPoolAddJob(TestStructFunc, &test);
		}
	}
	
	while (1) {
		scanf("%d", &threadNum);
		if (threadNum == 0) {
			break;
		}
	}
	ThreadPoolDeinit();
	return 0;
}

int main(void)
{
	printf("test thread pool\n");
	EnterTestThreadPool();
	return 0;
}