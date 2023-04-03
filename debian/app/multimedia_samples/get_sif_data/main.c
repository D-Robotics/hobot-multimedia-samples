/*
 * @Author: yaqiang.li
 * @Date:   2022-01-28 10:34:29
 * @Last Modified by: jiale01.luo
 * @Last Modified time: 2023-02-15 17:02:19
 */

#include <stdio.h>
#include "stdint.h"
#include "stddef.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <argp.h>
#include "sensor_handle.h"

#define ISDIGIT(x) ((x) >= '0' && (x) <= '9')

void help_msg()
{
	printf("********************** Command Lists *************************\n");
	printf("  q	-- quit\n");
	printf("  g	-- get one frame\n");
	printf("  l	-- get a set frames\n");
	printf("  h	-- print help message\n");
}

struct arguments
{
    int index;
    char *cam_cfg_file;
    char *vio_cfg_file;
};
static char doc[] = "get sif data sample -- An example of get raw data from XJ3 SIF module";
static struct argp_option options[] = {
    {"config", 'c', "CAM_CONFIG_FILE_PATH", 0, "config file path"},
    {"vio", 'v', "VIO_CONFIG_FILE_PATH", 0, "config file path"},
    {"index", 'i', "INDEX_OF_CAMERA", 0, "the index of camera"},
    {0}};
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;
    switch (key)
    {
    case 'i':
        args->index = atoi(arg);
        break;
    case 'c':
        args->cam_cfg_file = arg;
        break;
    case 'v':
        args->vio_cfg_file = arg;
        break;
    case ARGP_KEY_END:
    {
        if (state->argc != 7 )
        {
           argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
        }
    }
    break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
static struct argp argp = {options, parse_opt, 0, doc};

int main(int argc, char *argv[])
{
	struct arguments args;
    memset(&args, 0, sizeof(args));
    argp_parse(&argp, argc, argv, 0, 0, &args);
	int ret = 0, i = 0;
	char choose[32];
	int select;
	int grade;
	sensor_sif_dev_init(args.cam_cfg_file,args.vio_cfg_file,args.index);
	sif_dump_func();
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
