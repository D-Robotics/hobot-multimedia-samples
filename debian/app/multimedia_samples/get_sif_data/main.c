/*
* @Author: yaqiang.li
* @Date:   2022-01-28 10:34:29
* @Last Modified by:   yaqiang.li
* @Last Modified time: 2022-01-28 11:08:11
*/

#include <stdio.h>
#include "stdint.h"
#include "stddef.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "module.h"
#include <sensor_handle.h>

#define ISDIGIT(x)  ((x) >= '0' && (x) <= '9')

void help_msg()
{
	printf("********************** Command Lists *************************\n");
	printf("  q	-- quit\n");
	printf("  g	-- get one frame\n");
	printf("  l	-- get a set frames\n");
	printf("  h	-- print help message\n");
}

int main(int argc, char *argv[])
{
	int ret = 0, i = 0;
	char choose[32];
	int select;
	int grade;

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

	/* 初始化选择的sensor，会配置sensor、mipi、dev、pipe参数，并且完成模块init和start
	 * 本接口调用后，sensor正常就已经出图了
	 */
	ret = sensor_lists[select].func();
	if (ret != 0) {
		printf("init or start sensor pipeline failed\n");
		return ret;
	}

	/* 从sif获取sensor图像，如果是raw图，需要使用hobotplay工具打开 */
	sif_dump_func();

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
			case 'g':  // get a raw file
				sif_dump_func();
				break;
			case 'l': // 循环获取，用于计算帧率
				for(i = 0; i < 12; i++)
					sif_dump_func();
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
