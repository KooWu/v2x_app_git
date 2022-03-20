#include "my_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <semaphore.h>
#include "cJSON.h"

#define LOG_PATH_LEN 255 // /home/yxw/log/v2x/
#define LOG_FILE_NAME 32 // /home/yxw/log/v2x/v2x_app.log
#define LOG_BAK_FILE_NAME (LOG_PATH_LEN + LOG_FILE_NAME + 16) // /home/yxw/log/v2x/v2x_app.log.bak
#define LOG_BAK_FILE_TAR_NAME (LOG_PATH_LEN + LOG_FILE_NAME + 24) // /home/yxw/log/v2x/v2x_app01.tar.gz
#define LOG_BAK_FILE_TIME_NAME (LOG_PATH_LEN + LOG_FILE_NAME + 32) // /home/yxw/log/v2x/v2x_app_20220312150612.log
#define LOG_CFG_FILE_NAME "/home/yxw/share/work/v2x/etc/logCfg.json"
#define LOG_DEFAULT_DIR "/home/yxw/share/log/v2x/"
#define LOG_DEFAULT_LEVEL 15
#define LOG_DEFAULT_SIZE (60 * 1024)
#define LOG_SIZE_MAX (10 * 1024 * 1024)
#define LOG_SIZE_MIN 1024
#define LOG_FILE_NUM_MAX 20
#define LOG_FILE_NUM_MIN 2
#define LOG_CACHE_SIZE (8 * 1024)
#define LOG_LINE_MAX 1024
#define LOG_FLUSH_SIZE_MAX (LOG_CACHE_SIZE - LOG_LINE_MAX)
#define LOG_TIME_OUT 3

typedef enum {
    OUTPUT_FILE_TYPE,
    OUTPUT_PRINT_TYPE,
    OUTPUT_MAX_TYPE
} MyLogOutputType;

typedef struct {
    char logPath[LOG_PATH_LEN + 1];
    char logName[LOG_PATH_LEN + LOG_FILE_NAME + 1];
    char logBakName[LOG_PATH_LEN + LOG_BAK_FILE_NAME + 1];
    char logIdxFile[LOG_PATH_LEN + LOG_FILE_NAME + 1];
    uint32_t level;
    uint32_t size;
    uint32_t maxFileNum;
    MyLogOutputType output;
} LogCfgInfo;

static bool g_isInit = false;
static pthread_mutex_t g_logMutex = PTHREAD_MUTEX_INITIALIZER;
static LogCfgInfo g_logCfg;
static pthread_mutex_t g_initMutex = PTHREAD_MUTEX_INITIALIZER;
static char g_logCache[LOG_CACHE_SIZE];
static sem_t g_timedSem;
static int32_t g_logFd = -1;
static bool g_isRunning = false;

static void WriteLogToFile(char *buf, int32_t writeLen)
{
    struct stat fileInfo;

    if (g_logFd != -1) {
        if (access(g_logCfg.logName, F_OK) != 0) {
            (void)close(g_logFd);
            g_logFd = -1;
            return;
        }
    }

    if (g_logFd == -1) {
        g_logFd = open(g_logCfg.logName, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        if (g_logFd == -1) {
            (void)printf("WriteLogToFile open failed\n");
            return;
        }
    }
    (void)write(g_logFd, buf, writeLen);
    (void)memset(&fileInfo, 0, sizeof(fileInfo));
    (void)fstat(g_logFd, &fileInfo);
    if (g_logCfg.size < fileInfo.st_size) {
        (void)close(g_logFd);
        g_logFd = -1;
        (void)rename(g_logCfg.logName, g_logCfg.logBakName);
        (void)sem_post(&g_timedSem);
    }
}

static void WriteLogToCache(char *buf, int32_t writeLen)
{
    (void)pthread_mutex_lock(&g_logMutex);
    struct timespec cur;
    (void)clock_gettime(CLOCK_MONOTONIC, &cur);
    static uint32_t offset = 0;
    static uint64_t lastTime = 0;
    if (cur.tv_sec < lastTime) {
        lastTime = cur.tv_sec;
    }

    if ((buf != NULL) && (writeLen != 0)) {
        strncpy(g_logCache+offset, buf, writeLen);
        offset += writeLen;
    }

    if (offset > LOG_FLUSH_SIZE_MAX || (cur.tv_sec - lastTime) > LOG_TIME_OUT) {
        WriteLogToFile(g_logCache, offset);
        offset = 0;
        lastTime = cur.tv_sec;
    }
    (void)pthread_mutex_unlock(&g_logMutex);
}

void MyPrintLog(char *fileName, uint32_t line, MyLogLevel level, char *format, ...)
{
    struct tm tm;
    struct timeval tv;
    char buf[LOG_LINE_MAX];
    uint16_t offset;
    int32_t len;
    va_list ap;

    if (!g_isInit) {
        (void)printf("MyPrintLog line: %d\n", __LINE__);
        return;
    }
    if ((fileName == NULL) || (format == NULL)) {
        (void)printf("MyPrintLog line: %d\n", __LINE__);
        return;
    }
    if (g_logCfg.level & level == 0) {
        (void)printf("MyPrintLog line: %d\n", __LINE__);
        return;
    }
    if (strrchr(fileName, '/') != NULL) {
        fileName++;
    }

    const char *key[DEBUG_LEVEL+1] = { "UNKOWN", "ERROR", "WARN", "UNKOWN", "INFO", "UNKOWN", "UNKOWN", "UNKOWN", "DEBUG"};
    (void)gettimeofday(&tv, NULL);
    (void)localtime_r(&tv.tv_sec, &tm);
    (void)memset(buf, 0, sizeof(buf));
    offset = sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%03ld %s %05d %s ", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, tm.tm_hour, \
                     tm.tm_min, tm.tm_sec, tv.tv_usec / 1000, fileName, line, key[level]);
    va_start(ap, format);
    len = vsnprintf(buf + offset, sizeof(buf) - offset, format, ap);
    va_end(ap);
    if (len < 0) {
        (void)printf("MyPrintLog line: %d\n", __LINE__);
        return;
    }
    len = len + offset;
    if (g_logCfg.output == OUTPUT_PRINT_TYPE) {
        (void)printf("%s\n", buf);
        return;
    }

    WriteLogToCache(buf, len);
}

static int32_t MyLogGetProcessName(char *processName, uint32_t processNameLen)
{
    dbg();
    int32_t nameLen;
    char name[LOG_PATH_LEN];
    char *string = NULL;

    if ((processName == NULL) || (processNameLen < LOG_PATH_LEN)) {
        dbg("invalid param\n");
        return -1;
    }

    nameLen = readlink("/proc/self/exe", name, sizeof(name) - 1);
    if ((nameLen == -1) || (nameLen > LOG_PATH_LEN)) {
        dbg("readlink error or path is too long processNameLen:%d\n", processNameLen);
        return -1;
    }
    name[nameLen] = '\0';
    string = strrchr(name, '/');
    if (string == NULL) {
        return -1;
    }
    string++;
    if (strlen(string) == '\0') {
        return -1;
    }
    (void)strncpy(processName, string, strlen(string));

    return 0;
}

static int32_t MyLogCheckLogPathIsValid(void)
{
    dbg();
    DIR *dir = opendir(g_logCfg.logPath);
    if (dir == NULL) {
        if (mkdir(g_logCfg.logPath, 0644) != 0) {
            dbg("mkdir failed");
            return -1;
        }
    } else {
        (void)closedir(dir);
        dir = NULL;
    }

    return 0;
}

static void MyLogParseJsonCfg(cJSON *item)
{
    dbg();
    g_logCfg.level = LOG_DEFAULT_LEVEL;
    g_logCfg.size = LOG_DEFAULT_SIZE;
    g_logCfg.maxFileNum = LOG_FILE_NUM_MIN;
    g_logCfg.output = OUTPUT_PRINT_TYPE;
    cJSON *subItem = cJSON_GetObjectItem(item, "MaxFileSize_KB");
    if (subItem != NULL) {
        g_logCfg.size = subItem->valueint * 1024;
        if ((g_logCfg.size == 0) || (g_logCfg.size > LOG_SIZE_MAX)) {
            g_logCfg.size = LOG_DEFAULT_SIZE;
        }
    }
    subItem = cJSON_GetObjectItem(item, "MaxFileNum");
    if (subItem != NULL) {
        g_logCfg.maxFileNum = subItem->valueint;
        if ((g_logCfg.maxFileNum < LOG_FILE_NUM_MIN) || (g_logCfg.maxFileNum > LOG_FILE_NUM_MAX)) {
            g_logCfg.maxFileNum = LOG_FILE_NUM_MIN;
        }
    }
    subItem = cJSON_GetObjectItem(item, "Level");
    if (subItem != NULL) {
        g_logCfg.level = subItem->valueint;
        if (g_logCfg.level < ERROR_LEVEL) {
            g_logCfg.level = LOG_DEFAULT_LEVEL;
        }
    }
    subItem = cJSON_GetObjectItem(item, "Output");
    if (subItem != NULL) {
        g_logCfg.output = subItem->valueint;
        if ((g_logCfg.output < OUTPUT_FILE_TYPE) || (g_logCfg.output > OUTPUT_MAX_TYPE)) {
            g_logCfg.output = OUTPUT_PRINT_TYPE;
        }
    }
}

static int32_t MyLogParseJsonFile(char *buf, char *processName)
{
    dbg();
    cJSON *root = cJSON_Parse(buf);
    if(NULL == root) {
        dbg("cJSON_Parse failed\n");
        return -1;
    }

    int32_t ret = -1;
    do {
        cJSON *filePath = cJSON_GetObjectItem(root, "FilePath");
        if(filePath == NULL) {
             dbg("cJSON_GetObjectItem FilePath failed\n");
             break;
        }

        cJSON *process = cJSON_GetObjectItem(root, "Process");
        if(process == NULL) {
             dbg("cJSON_GetObjectItem Process failed\n");
             break;
        }
        bool isFind = false;
        cJSON *item;
        int32_t cnt = cJSON_GetArraySize(process);
        int32_t i;
        for (i = 0; i< cnt; i++) {  
            item = cJSON_GetArrayItem(process, i);
            cJSON *subItem = cJSON_GetObjectItem(item, "ProcessName");
            if (subItem == NULL) {
                dbg("processName: %s\n", subItem->valuestring);
                isFind = false;
                break;
            }
            
            if (strncmp(subItem->valuestring, processName, strlen(processName)) == 0) {
                isFind = true;
                break;
            }
        }
        if (!isFind) {
            dbg("not find %s\n", processName);
            break;
        }
        (void)strncpy(g_logCfg.logPath, filePath->valuestring, strlen(filePath->valuestring));
        MyLogParseJsonCfg(item);
        ret = 0;
    } while (0);

    cJSON_Delete(root);
    return ret;
}

static int32_t MyLogReadFileCfgBuf(char *logJsonBuf, uint32_t size)
{
    dbg();
    int32_t fd = open(LOG_CFG_FILE_NAME, O_RDONLY);
    if (fd == -1) {
        dbg("open failed\n");
        return -1;
    }
    
    int32_t ret;
    do {
        struct stat statInfo;
        memset(&statInfo, 0, sizeof(statInfo));
        ret = fstat(fd, &statInfo);
        if (ret != 0) {
            dbg("fstat failed\n");
            break;
        }

        if (statInfo.st_size == 0 || statInfo.st_size >= size) {
            dbg("json file too big\n");
            ret = -1;
            break;
        }
        
        ret = read(fd, logJsonBuf, statInfo.st_size);
        if (ret != statInfo.st_size) {
            dbg("read failed, ret:%d\n", ret);
            break;
        }
        ret = 0;
    } while (0);
    (void)close(fd);

    return ret;
}

static int32_t MyLogLoadFileCfg(char *processName)
{
    dbg();
    char buf[1024];
    int32_t ret = MyLogReadFileCfgBuf(buf, sizeof(buf));
    if (ret != 0) {
        dbg("MyLogReadFileCfgBuf failed\n");
        return -1;
    }
    ret = MyLogParseJsonFile(buf, processName);
    if (ret != 0) {
        dbg("MyLogParseJsonFile failed\n");
        return -1;
    }
    return 0;
}

static void MyLogLoadDefaultCfg(void)
{
    g_logCfg.level = LOG_DEFAULT_LEVEL;
    g_logCfg.size = LOG_DEFAULT_SIZE;
    g_logCfg.maxFileNum = LOG_FILE_NUM_MIN;
    g_logCfg.output = OUTPUT_PRINT_TYPE;
    (void)strncpy(g_logCfg.logPath, LOG_DEFAULT_DIR, strlen(LOG_DEFAULT_DIR));
}

static int32_t MyLogCfgLoad(char *processName)
{
    dbg();
    if (MyLogLoadFileCfg(processName) != 0) {
        MyLogLoadDefaultCfg();
    }

    if (MyLogCheckLogPathIsValid() != 0) {
        return -1;
    }

    return 0;
}

static void MyLogPathCreate(char *processName)
{
    (void)sprintf(g_logCfg.logName, "%s/%s.log", g_logCfg.logPath, processName);
    (void)sprintf(g_logCfg.logBakName, "%s/%s.log.bak", g_logCfg.logPath, processName);
    (void)sprintf(g_logCfg.logIdxFile, "%s/%s.idx", g_logCfg.logPath, processName);
}

static int32_t MyLogReadIdxFile(void)
{
    dbg();
    int fd = open(g_logCfg.logIdxFile, O_RDWR | O_CREAT | O_TRUNC, 0664);
    if (fd == -1) {
        dbg("open failed\n");
        return 0;
    }
    char idxstr[4] = {0};
    int32_t ret = read(fd, idxstr, 2);
    if (ret != 2) {
        (void)close(fd);
        return 0;
    }
    int32_t idx = atoi(idxstr);
    if ((idx < 0) || (idx >= g_logCfg.maxFileNum)) {
        (void)close(fd);
        return 0;
    }
    return idx;
}

static void MyLogWriteIdxFile(int32_t idx)
{
    int fd = open(g_logCfg.logIdxFile, O_RDWR);
    if (fd == -1) {
        return;
    }

    char idxstr[4] = {0};
    sprintf(idxstr, "%02d", idx);
    (void)write(fd, idxstr, strlen(idxstr));
}

static void MyLogTarLogFile(int32_t index)
{
    struct tm tm;
    struct timeval tv;
    (void)gettimeofday(&tv, NULL);
    (void)localtime_r(&tv.tv_sec, &tm);

    char timeLogFile[LOG_BAK_FILE_TIME_NAME] = {0};
    (void)sprintf(timeLogFile, "%s_%04d%02d%02d%02d%02d%02d", g_logCfg.logName, 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    if (rename(g_logCfg.logBakName, timeLogFile) != 0) {
        (void)remove(g_logCfg.logBakName);
        return;
    }
    char compressFile[LOG_BAK_FILE_TIME_NAME] = {0};
    (void)sprintf(compressFile, "%s%02d.tar.gz", g_logCfg.logName, index);
    char compressCmd[LOG_BAK_FILE_TIME_NAME + LOG_BAK_FILE_TIME_NAME + 32] = {0};
    (void)sprintf(compressCmd, "tar -czvPf %s %s", compressFile, timeLogFile);
    (void)system(compressCmd);
    (void)remove(timeLogFile);
}

static void *MyLogCycleThread(void *arg)
{
    (void)pthread_detach(pthread_self());
    int32_t index = MyLogReadIdxFile();
    g_isRunning = true;
    while (g_isRunning) {
        struct timespec cur;
        (void)clock_gettime(CLOCK_REALTIME, &cur);
        cur.tv_sec += LOG_TIME_OUT;
        errno = 0;
        int32_t ret = sem_timedwait(&g_timedSem, &cur);
        if (ret != 0 || errno == ETIMEDOUT) {
            WriteLogToCache(NULL, 0);
        } else {
            MyLogTarLogFile(index);
            index = (index + 1) % g_logCfg.maxFileNum;
            MyLogWriteIdxFile(index);
        }
    }
}

static void MyLogInitSuccessLog(char *processName)
{
    MY_INFO("%s log start\n", processName);
    MY_INFO("logName: %s, level: %u, size: %u, maxFileNum: %u, output: %d\n",
            g_logCfg.logName, g_logCfg.level, g_logCfg.size, g_logCfg.maxFileNum, g_logCfg.output);
    dbg("logName: %s, level: %u, size: %u, maxFileNum: %u, output: %d\n",
            g_logCfg.logName, g_logCfg.level, g_logCfg.size, g_logCfg.maxFileNum, g_logCfg.output);
}

int32_t MyLogInit(void)
{
    dbg();
    (void)pthread_mutex_lock(&g_initMutex);
    if (g_isInit) {
        (void)pthread_mutex_unlock(&g_initMutex);
        return 0;
    }
    char processName[LOG_PATH_LEN];
    int32_t ret = -1;
    do {
        memset(&g_logCfg, 0, sizeof(g_logCfg));
        memset(g_logCache, 0, sizeof(g_logCache));
        (void)sem_init(&g_timedSem, 0, 0);
        ret = MyLogGetProcessName(processName, sizeof(processName));
        if (ret != 0) {
            dbg("MyLogGetProcessName failed\n");
            break;
        }

        if (MyLogCfgLoad(processName) != 0) {
            dbg("LogCfgLoad failed\n");
            break;
        }

        MyLogPathCreate(processName);
        if (g_logCfg.output == OUTPUT_FILE_TYPE) {
            pthread_t logThread;
            ret = pthread_create(&logThread, NULL, MyLogCycleThread, NULL);
        }
        g_isInit = true;
        ret = 0;
    } while (0);
    (void)pthread_mutex_unlock(&g_initMutex);
    if (ret == 0) {
        MyLogInitSuccessLog(processName);
    }

    return ret;
}

void MyLogDeinit(void)
{
    (void)pthread_mutex_lock(&g_logMutex);

    g_isRunning = false;
    (void)pthread_mutex_lock(&g_initMutex);
    g_isInit = false;
    (void)pthread_mutex_unlock(&g_initMutex);
    sleep(2);
    (void)close(g_logFd);
    g_logFd = -1;
    memset(&g_logCfg, 0, sizeof(g_logCfg));
    memset(g_logCache, 0, sizeof(g_logCache));
    (void)sem_destroy(&g_timedSem);
    (void)pthread_mutex_unlock(&g_logMutex);
}

int32_t MyLogGetLevel(void)
{
    (void)pthread_mutex_lock(&g_logMutex);
    int32_t level = g_logCfg.level;
    (void)pthread_mutex_unlock(&g_logMutex);
    return level;
}

int32_t MyLogSetLevel(int32_t level)
{
    if (level < DEBUG_LEVEL) {
        return -1;
    }
    (void)pthread_mutex_lock(&g_logMutex);
    g_logCfg.level = level;
    (void)pthread_mutex_unlock(&g_logMutex);
    return 0;
}