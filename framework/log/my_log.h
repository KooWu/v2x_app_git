#ifndef _MY_LOG_H_
#define _MY_LOG_H_

#include <stdint.h>
#include <stdio.h>

typedef enum {
	ERROR_LEVEL = 1,
	WARN_LEVEL = 2,
	INFO_LEVEL = 4,
    DEBUG_LEVEL  = 8,
} MyLogLevel;

void MyPrintLog(char *fileName, uint32_t line, MyLogLevel level, char *format, ...);

#define dbg(fmt, args... ) printf("file[%s], line[%d]: "fmt"\n", __FILE__, __LINE__, ##args)
#define MY_DEBUG(format, args...) MyPrintLog(__FILE__, __LINE__, DEBUG_LEVEL, format, ##args)
#define MY_INFO(format, args...)  MyPrintLog(__FILE__, __LINE__, INFO_LEVEL, format, ##args)
#define MY_WARN(format, args...) MyPrintLog(__FILE__, __LINE__, WARN_LEVEL, format, ##args)
#define MY_ERROR(format, args...) MyPrintLog(__FILE__, __LINE__, ERROR_LEVEL, format, ##args)
#define MY_ENTER() MY_DEBUG("%s enter\n", __FUNCTION__)
#define MY_EXIT() MY_DEBUG("%s exit\n", __FUNCTION__)

int32_t MyLogInit(void);
void MyLogDeinit();
int32_t MyLogGetLevel(void);
int32_t MyLogSetLevel(int32_t level);

#endif