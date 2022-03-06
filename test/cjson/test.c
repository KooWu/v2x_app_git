#include <stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "cJSON.h"

static int CreateJsonFile(int fd)
{
	cJSON *root = cJSON_CreateObject();
	if (root == NULL) {
		printf("cJSON_CreateObject failed\n");
		return -1;
	}
	
	cJSON_AddStringToObject(root, "FilePath", "/home/yxw/log/");
	cJSON *process = cJSON_CreateArray();
	if (process == NULL) {
		printf("cJSON_CreateArray failed\n");
		return -1;
	}
	
	cJSON_AddItemToObject(root, "Process", process);
	
	cJSON *v2x = cJSON_CreateObject();
	cJSON_AddItemToArray(process, v2x);
	cJSON_AddStringToObject(v2x, "ProcessName", "v2x_app");
	cJSON_AddNumberToObject(v2x, "MaxFileSize_MB", 1);
    cJSON_AddNumberToObject(v2x, "MaxFileNum", 5);
	cJSON_AddNumberToObject(v2x, "Level", 3);
	cJSON_AddNumberToObject(v2x, "Output", 0);
	
	cJSON *gnss = cJSON_CreateObject();
	cJSON_AddItemToArray(process, gnss);
	cJSON_AddStringToObject(gnss, "ProcessName", "gnss_app");
	cJSON_AddNumberToObject(gnss, "MaxFileSize_MB", 2);
    cJSON_AddNumberToObject(gnss, "MaxFileNum", 5);
	cJSON_AddNumberToObject(gnss, "Level", 7);
	cJSON_AddNumberToObject(gnss, "Output", 1);
	char *jsonString = cJSON_Print(root);
	printf("create: %s\n", jsonString);
	int ret = write(fd, jsonString, strlen(jsonString));
	if (ret != strlen(jsonString)) {
		printf("write failed\n");
		cJSON_Delete(root);
		return -1;
	}
    cJSON_Delete(root);
	return 0;
}

static int CreateLogCfgJsonFile(const char *fileName)
{
	int fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0664);
	if (fd == -1) {
		printf("open failed\n");
		return -1;
	}
	int ret = CreateJsonFile(fd);
	if (ret != 0) {
		printf("CreateJsonFile failed\n");
		(void)close(fd);
		return -1;
	}
	
	(void)close(fd);
	return 0;
}

static int ParseJsonFile(int fd)
{
	struct stat statInfo;
	memset(&statInfo, 0, sizeof(statInfo));
	int ret = fstat(fd, &statInfo);
	if (ret != 0) {
		printf("fstat failed\n");
		return -1;
	}

	if (statInfo.st_size == 0 || statInfo.st_size >= 1024)
	{
		printf("json file too big\n");
		return -1;
	}
	char buf[1024];
	ret = read(fd, buf, statInfo.st_size);
	if (ret != statInfo.st_size) {
		printf("read failed, ret:%d\n", ret);
		return -1;
	}
	
	cJSON *root = cJSON_Parse(buf);
	if(NULL == root) {
		printf("cJSON_Parse failed\n");
		return -1;
	}
	char *jsonString = cJSON_Print(root);
	printf("parse: %s\n", jsonString);

    cJSON *filePath = cJSON_GetObjectItem(root, "FilePath");
    if(filePath != NULL)
    {
         printf("FilePath: %s\n", filePath->valuestring);
    }

    cJSON *process = cJSON_GetObjectItem(root, "Process");
    if(process != NULL)
    {
        int cnt = cJSON_GetArraySize(process);
		printf("process cnt: %d\n", cnt);
		int i;
		for (i = 0; i< cnt; i++) {  
			cJSON *item = cJSON_GetArrayItem(process, i);
			cJSON *subItem = cJSON_GetObjectItem(item, "ProcessName");
			if (subItem != NULL) {
				printf("processName: %s\n", subItem->valuestring);
			}
			subItem = cJSON_GetObjectItem(item, "MaxFileSize_MB");
			if (subItem != NULL) {
				printf("maxFileSize: %d\n", subItem->valueint);
			}
			subItem = cJSON_GetObjectItem(item, "MaxFileNum");
			if (subItem != NULL) {
				printf("MaxFileNum: %d\n", subItem->valueint);
			}
			subItem = cJSON_GetObjectItem(item, "Level");
			if (subItem != NULL) {
				printf("Level: %d\n", subItem->valueint);
			}
			subItem = cJSON_GetObjectItem(item, "Output");
			if (subItem != NULL) {
				printf("Output: %d\n", subItem->valueint);
			}
		}
	}

	return 0;
}

static int ParseLogCfgJsonFile(const char *fileName)
{
	int fd = open(fileName, O_RDONLY);
	if (fd == -1) {
		printf("open failed\n");
		return -1;
	}
	int ret = ParseJsonFile(fd);
	if (ret != 0) {
		printf("ParseJsonFile failed\n");
		(void)close(fd);
		return -1;
	}
	(void)close(fd);
	return 0;
}

int main(void)
{
	char *fileName = "./logCfg.json";
	int ret = CreateLogCfgJsonFile(fileName);
	if (ret != 0) {
		printf("CreateLogCfgJsonFile failed\n");
		return 0;
	}
	ret = ParseLogCfgJsonFile(fileName);
	if (ret != 0) {
		printf("ParseLogCfgJsonFile failed\n");
		return 0;
	}
	return 0;
}