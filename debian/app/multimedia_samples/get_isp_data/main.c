/*
* @Author: Horizon Robotics
* @Date:   2022-05-11 10:34:29
* @Last Modified by:   liangbin.yang
* @Last Modified time: 2022-05-11 17:59:10
*/

#include <stdio.h>
#include "stdint.h"
#include "stddef.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <cJSON.h>

#include "module.h"
#include <sensor_handle.h>

#define MAX_CAMERAS 3
typedef struct {
    int enable;
    int i2c_bus;
    int mipi_host;
} board_camera_info_t;

int board_bus_num = 0;
int entry_index = 0;

#define ISDIGIT(x)  ((x) >= '0' && (x) <= '9')

void help_msg()
{
	printf("********************** Command Lists *************************\n");
	printf("  q	-- quit\n");
	printf("  g	-- get one frame\n");
	printf("  l	-- get a set frames\n");
	printf("  h	-- print help message\n");
}

static cJSON *open_json_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    int32_t ret = 0;
    if (fp == NULL)
    {
        perror("fopen");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = malloc(fsize + 1);
    ret = fread(buf, fsize, 1, fp);
    if (ret != 1)
    {
        printf("Error fread size:%d\n", ret);
    }
    fclose(fp);

    buf[fsize] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("Error cJSON_Parse: %s\n", error_ptr);
        }
        free(buf);
        return NULL;
    }
    free(buf);

    return root;
}

static int parse_cameras(cJSON *cameras, board_camera_info_t camera_info[])
{
    cJSON *camera = NULL;
    int ret;

    for (int i = 0; i < MAX_CAMERAS; i++) {
        camera_info[i].enable = 0;

        camera = cJSON_GetArrayItem(cameras, i);
        if (camera == NULL) {
            break;
        }

        // parse reset
        cJSON *reset = cJSON_GetObjectItem(camera, "reset");
        if (reset == NULL) {
            ret = -1;
            goto exit;
        }

        int gpio_num = atoi(strtok(reset->valuestring, ":"));
        char *active = strtok(NULL, ":");

        // parse i2c_bus
        cJSON *i2c_bus_obj = cJSON_GetObjectItem(camera, "i2c_bus");
        if (i2c_bus_obj == NULL) {
            ret = -1;
            goto exit;
        }
        camera_info[i].i2c_bus = i2c_bus_obj->valueint;

        // parse mipi_host
        cJSON *mipi_host_obj = cJSON_GetObjectItem(camera, "mipi_host");
        if (mipi_host_obj == NULL) {
            ret = -1;
            goto exit;
        }
        camera_info[i].mipi_host = mipi_host_obj->valueint;
        camera_info[i].enable = 1;

        printf("Camera: gpio_num=%d, active=%s, i2c_bus=%d, mipi_host=%d\n",
            gpio_num, active, i2c_bus_obj->valueint, mipi_host_obj->valueint);
    }
    ret = 0;

exit:
    return ret;
}

static int parse_config(const char *json_file, board_camera_info_t camera_info[])
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *board_config = NULL;
    cJSON *cameras = NULL;
    char som_name[16];
    char board_name[32];

    FILE *fp = fopen("/sys/class/socinfo/som_name", "r");
    if (fp == NULL) {
        return -1 ;
    }
    fscanf(fp, "%s", som_name);
    fclose(fp);

    snprintf(board_name, sizeof(board_name), "board_%s", som_name);

    root = open_json_file(json_file);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON string.\n");
        return -1 ;
    }

    board_config = cJSON_GetObjectItem(root, board_name);
    if (!board_config) {
        fprintf(stderr, "Failed to get board_config object.\n");
        ret = -1;
        goto exit;
    }

    cameras = cJSON_GetObjectItem(board_config, "cameras");
    if (!cameras) {
        fprintf(stderr, "Failed to get cameras array.\n");
        ret = -1;
        goto exit;
    }

    ret = parse_cameras(cameras, camera_info);

exit:
    cJSON_Delete(root);
    return ret;
}

int main(int argc, char *argv[])
{
	int ret = 0, i = 0;
	char choose[32];
	int select;
	int grade;
	int all_failed = 1;
	board_camera_info_t camera_info[MAX_CAMERAS];

	/* 初始化sensor列表，把sensor 标识和参数配置添加到 sensor_lists 里面 */
	sensor_modules_probe();

	/* 输出sensor列表，并且提供输入来选择使用哪个sensor */
	printf("********************** Sensor Lists *************************\n");
	for (i = 0; i < ARRAY_SIZE(sensor_lists); i++) {
		if (0 == strlen(sensor_lists[i].sensor_tag)) break;
		printf("\t%d -- %s\n", i, sensor_lists[i].sensor_tag);
	}
	printf("*************************************************************\n");
	printf("\nPlease select :");
	scanf("%s", choose);
	if (!ISDIGIT(choose[0])) {
		return -1;
	}

	select = atoi(choose);

	memset(camera_info, 0, sizeof(camera_info));
    ret = parse_config("/etc/board_config.json", camera_info);
    if (ret != 0) {
        printf("Failed to parse cameras\n");
        return -1;
    }

	for (int i = 0; i < MAX_CAMERAS; i++) {
        printf("Camera %d:\n", i);
        printf("\tenable: %d\n", camera_info[i].enable);
        printf("\ti2c_bus: %d\n", camera_info[i].i2c_bus);
        printf("\tmipi_host: %d\n", camera_info[i].mipi_host);

		board_bus_num = camera_info[i].i2c_bus;
		entry_index = camera_info[i].mipi_host;

		/* 初始化选择的sensor，会配置sensor、mipi、dev、pipe参数，并且完成模块init和start
		* 本接口调用后，sensor正常就已经出图了
		*/
		ret = sensor_lists[select].func();
		if (ret != 0) {
			printf("init or start sensor pipeline failed\n");
			//return ret;
		}
		else {
			all_failed = 0;
			break;
		}
    }

	if (all_failed) {
    	return -1;
	}

	/* 从vin获取yuv图像，获取出来得yuv是nv12格式，可以使用7yuv工具打开 */
	yuv_dump_func();

	/* 用户交互操作 */
	help_msg();
	printf("\nCommand: ");
	while((grade=getchar())!=EOF)
	{
		switch(grade)
		{
			case 'q':
				printf("quit\n");
				exit(0);
			case 'g':  // get a yuv file
				yuv_dump_func();
				break;
			case 'l': // 循环获取，用于计算帧率
				for(i = 0; i < 12; i++)
					yuv_dump_func();
				break;
			case 'h':
				help_msg();
			case '\n':
				printf("Command: ");
				continue;
			case '\r':
				continue;
			default:
				printf("Command dose not supported!\n");
				help_msg();
				break;
		}
		printf("\nCommand: ");
	}

	return 0;
}
