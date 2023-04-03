
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <argp.h>
#include "sample.h"

#define TAG "[COMMON]"

atomic_int running = 1;
time_t start_time, now;
time_t run_time = 10000000;
time_t end_time = 0;

int sample_mode = 0;
char *g_inputfilename = NULL;

int allocate_ion_mem(int size, ion_buffer_t *ionbuf)
{
	int32_t ionType = 25;
	uint64_t hbmem_flags = 0;

	hbmem_flags |= (ionType) << 16;
	hbmem_flags = (BACKEND_ION_CMA << 24) | (hbmem_flags << 32);

	ionbuf->virt_addr = (unsigned long)hbmem_alloc(size, hbmem_flags, NULL);
	if (ionbuf->virt_addr == 0)
	{
		printf("%s hbmem_alloc failed\n", TAG);
		return -1;
	}
	ionbuf->phys_addr = (uint64_t)hbmem_phyaddr(ionbuf->virt_addr);
	if (ionbuf->phys_addr == 0)
	{
		printf("%s hbmem_phyaddr failed\n", TAG);
		hbmem_free(ionbuf->virt_addr);
		return -1;
	}
	printf("hbmem_alloc, virt: %lx, phys: %llx\n", ionbuf->virt_addr, ionbuf->phys_addr);

	return 0;
}

int release_ion_mem(ion_buffer_t *ionbuf)
{
	if (ionbuf->virt_addr)
	{
		hbmem_free(ionbuf->virt_addr);
		ionbuf->virt_addr = 0;
	}

	return 0;
}
static char doc[] = "Codec sample -- An example of encode/decode";
static struct argp_option options[] = {
	{"mode", 'm', "MODE", 0, "0.encode 1.decode"},
	{"type", 't', "TYPE", 0, "0.h264 1.h265 2.jpeg"},
	{"input1", 0x80, "PATH1", 0, "NV12 data to encode"},
	{"input2", 0x81, "PATH2", 0, "NV12 data to encode(thread 2)"},
	{"output1", 0x82, "PATH1", 0, "the path of output file"},
	{"output2", 0x83, "PATH2", 0, "the path of output file (thread 2)"},
	{"prefix1", 0x84, "PREFIX1", 0, "the prefix of decoded picture"},
	{"prefix2", 0x85, "PREFIX2", 0, "the prefix of decoded picture (thread 2)"},
	{"width1", 0x86, "WIDTH1", 0, "the width of NV12 data"},
	{"width2", 0x87, "WIDTH2", 0, "the width of NV12 data (thread 2)"},
	{"height1", 0x88, "HEIGHT1", 0, "the height of NV12 data"},
	{"height2", 0x89, "HEIGHT2", 0, "the height of NV12 data (thread 2)"},
	{"ext_buf", 'e', "BOOL", 0, "whether to use external buffer 1. enable 0.disable"},
	{"dual", 'd', "BOOL", 0, "whether to enable dual-channel decoding/encoding 1. enable 0.disable"},
	{"rc_mode", 'r', "MODE", 0, "0 H264CBR, 1 H264VBR, 2 H264AVBR, 3 H264FIXQP, 4 H264QPMAP, 5 H265CBR, 6 H265VBR, 7 H265AVBR, 8 H265FIXQP, 9 H265QPMAP,10 MJPEGFIXQP"},
	{0}};
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *args = state->input;
	switch (key)
	{
	case 'm':
		args->mode = atoi(arg);
		break;
	case 0x80:
		args->input1 = arg;
		break;
	case 0x81:
		args->input2 = arg;
		break;
	case 0x82:
		args->output1 = arg;
		break;
	case 0x83:
		args->output2 = arg;
		break;
	case 0x84:
		args->prefix1 = arg;
		break;
	case 0x85:
		args->prefix2 = arg;
		break;
	case 0x86:
		args->width1 = atoi(arg);
		break;
	case 0x87:
		args->width2 = atoi(arg);
		break;
	case 0x88:
		args->height1 = atoi(arg);
		break;
	case 0x89:
		args->height2 = atoi(arg);
		break;
	case 't':
		args->type = atoi(arg);
		break;
	case 'r':
		args->rc_mode = atoi(arg);
		break;
	case 'e':
		args->ext_buf = atoi(arg);
		break;
	case 'd':
		args->dual = atoi(arg);
		break;
	case ARGP_KEY_END:
	{
		if (args->dual == 0)
		{
			if (args->mode == 0)
			{
				if (args->type != 2 && state->argc < 14)
					argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
				else if (args->type == 2 && state->argc < 12)
				{
					argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
				}
			}
			else if (args->mode == 1 && state->argc < 10)
			{
				argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
			}
		}
		else
		{
			if (args->mode == 0 && state->argc < 16)
			{
				argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
			}
			else if (args->mode == 1 && state->argc < 14)
			{
				argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
			}
		}
	}
	break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

void intHandler(int dummy)
{
	running = 0;
	printf("rcv int signal\n");
}
static struct argp argp = {options, parse_opt, 0, doc};

int main(int argc, char *argv[])
{

	//parse args
	struct arguments args;
	memset(&args, 0, sizeof(args));
	argp_parse(&argp, argc, argv, 0, 0, &args);

	signal(SIGINT, intHandler);
	printf("PRESS CTRL+C TO STOP\n");
	if (args.mode == 0)//ENCODE
	{
		printf("NOW RUNNING IN ENCODE MODE\n");
		venc_param param1;
		param1.width = args.width1;
		param1.height = args.height1;
		param1.type = args.type;
		param1.ext_buf = args.ext_buf;
		param1.input = args.input1;
		param1.output = args.output1;
		param1.rc_mode = args.rc_mode;
		if (args.dual == 0)
		{
			printf("SINGAL CHN MODE\n");
			sample_venc((void *)&param1);
		}
		else
		{
			printf("DUAL CHN MODE\n");
			venc_param param2;

			param2.width = args.width2;
			param2.height = args.height2;
			param2.type = args.type;
			param2.ext_buf = args.ext_buf;
			param2.input = args.input2;
			param2.output = args.output2;
			param2.rc_mode = args.rc_mode;

			pthread_t thread1, thread2;
			pthread_create(&thread1, NULL, sample_venc, (void *)&param1);
			pthread_create(&thread2, NULL, sample_venc, (void *)&param2);

			pthread_join(thread1, NULL);
			pthread_join(thread2, NULL);
		}
	}
	else if (args.mode == 1)
	{
		printf("NOW RUNNING IN DECODE MODE\n");
		vdec_param param1;
		param1.width = args.width1;
		param1.height = args.height1;
		param1.type = args.type;
		param1.input = args.input1;
		param1.prefix = args.prefix1;
		if (args.dual == 0)
		{
			printf("SINGAL CHN MODE\n");

			sample_vdec((void *)&param1);
		}
		else
		{
			printf("DUAL CHN MODE\n");

			vdec_param param2;
			param2.width = args.width2;
			param2.height = args.height2;
			param2.type = args.type;
			param2.input = args.input2;
			param2.prefix = args.prefix2;
			pthread_t thread1, thread2;
			pthread_create(&thread1, NULL, sample_vdec, (void *)&param1);
			pthread_create(&thread2, NULL, sample_vdec, (void *)&param2);

			pthread_join(thread1, NULL);
			pthread_join(thread2, NULL);
		}
	}
	return 0;
}
