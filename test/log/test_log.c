#include "my_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>

static int32_t g_freq;
static int32_t g_level;
static int32_t g_isRunning = 1;

static void *TestThread(void *arg)
{
	int32_t num = 0;
	char *str = "hello world";
	
	while (g_isRunning) {
		MY_DEBUG("my log: %s%04d\n", str, num);
		num = (num+1) % 2000;
		usleep(g_freq * 1000);
	}
	return NULL;
}

static void ManageTestThread(void)
{
	pthread_t pid;
	int32_t ret = pthread_create(&pid, NULL, TestThread, NULL);
	if (ret != 0) {
		printf("pthread_create failed\n");
		return;
	}
	while (1) {
		scanf("%d", &g_isRunning);
		if (g_isRunning == 0) {
			break;
		}
	}
	pthread_join(pid, NULL);
}

int32_t EnterTestLog(void)
{
	int32_t ret = MyLogInit();
	if (ret != 0) {
		printf("MyLogInit failed");
		return -1;
	}
	
	printf("please input freq:\n");
	scanf("%d", &g_freq);
	if (g_freq <= 0 || g_freq > 5000) {
		g_freq = 100;
	}
	printf("freq: %d\n", g_freq);

	printf("please input level:\n");
	scanf("%d", &g_level);
	if (g_level < DEBUG_LEVEL || g_level > 15) {
		g_level = DEBUG_LEVEL;
	}
	printf("level: %d\n", g_level);
	MyLogSetLevel(g_level);
	ManageTestThread();
	return 0;
}

int main(void)
{
	printf("test my log\n");
	EnterTestLog();
	return 0;
}