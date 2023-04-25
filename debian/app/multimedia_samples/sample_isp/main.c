/***************************************************************************
* COPYRIGHT NOTICE
* Copyright 2019 Horizon Robotics.
* All rights reserved.
***************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "hb_isp_api.h"
#include "hb_isp_algo.h"

#define PORT    (7100)
#define IP_ADDR "127.0.0.1"

#define DEBUG
#define INFO

#ifdef INFO
#define	pr_info(format, ...)	printf("[isp_intf_test]: %s[%d]: "format "\n" , __func__, __LINE__, ##__VA_ARGS__)
#else
#define	pr_info(format, ...)
#endif

#ifdef DEBUG
#define	pr_dbg(format, ...)	printf("[isp_intf_test]: %s[%d]: "format "\n" , __func__, __LINE__, ##__VA_ARGS__)
#else
#define	pr_dbg(format, ...)
#endif

#define	pr_err(format, ...)	printf("[isp_intf_test]: %s[%d]: "format "\n" , __func__, __LINE__, ##__VA_ARGS__)

#define DWORD sizeof(uint32_t)
#define WORD sizeof(uint16_t)
#define BYTE sizeof(uint8_t)

enum isp_intf_type_e {
	IDX_CSC = 65,
	IDX_MESH_SHADING,
	IDX_MESH_SHADING_LUT,
	IDX_RADIAL_SHADING,
	IDX_RADIAL_SHADING_LUT,
	IDX_IRIDIX_STRENGTH_LEVEL,
	IDX_IRQ_SYNC,
	IDX_AWB_ZONE,
	IDX_AF_ZONE,
	IDX_AE5BIN_ZONE,
	IDX_AE_ZONE,
	IDX_AF_KERNEL,
	IDX_AEROI_INFO,
	IDX_LUMA_INFO,
	IDX_AEPARAM_INFO,
	IDX_AE_EX,
	IDX_AE = 97,
	IDX_AF,
	IDX_AWB,
	IDX_BL,
	IDX_DEMOSAIC,
	IDX_SHARPEN,
	IDX_GAMMA,
	IDX_IRIDIX,
	IDX_CNR,
	IDX_SINTER,
	IDX_TEMPER,
	IDX_SCENE_MODES,
	IDX_FWSTATE,
	IDX_MODCTRL,
	IDX_REGISTER,
	IDX_LIBREG_AE,
	IDX_LIBREG_AWB,
	IDX_LIBREG_AF,
	IDX_METERING_AE,
	IDX_METERING_AWB,
	IDX_METERING_AF,
	IDX_METERING_AE_5BIN,
	IDX_METERING_DATA_TIME,
	IDX_SWITCH_SCENCE,
	IDX_MAX,
};

ISP_AE_ATTR_S stAeAttr = {
	255, 255, 100, 60, 255, 1280, 63, 1, 2, 3, 4,
	1, //manual mode
};

ISP_AWB_ATTR_S stAwbAttr = {
	256, 256,
	1, //manual mode
};

ISP_BLACK_LEVEL_ATTR_S stBackLevelAttr = {
	168, 168, 168, 168,
	1, //manual mode
};

ISP_DEMOSAIC_ATTR_S stDemosaicAttr = {
	150, 85, 0,
	{
		15, 15, 15,
	},
	{
		15, 15, 15,
	},
	{
		15, 15, 15,
	},
	{
		15, 15, 15,
	},
	{
		15, 15, 15,
	},
	{
		15, 15, 15,
	},
	{
		15, 15,
	},
	{
		15, 15, 15,
	},
	{
		15, 15,
	},
	3000,
	3000,
	3000,
	3000,
	{
		{ 0, 1 },
		{ 256, 1 },
		{ 512, 1 },
		{ 768, 3 },
		{ 1024, 18 },
		{ 1280, 18 },
		{ 1536, 15 },
	},
};

ISP_SHARPEN_ATTR_S stSharpenAttr = {
	{15,15,15},
	{15,15,15},
	15, 15, 15, 15,
	{15,15,15,15,15,15,},
	{
		{	{ 0, 42 },
			{ 256, 27 },
			{ 512, 20 },
			{ 768, 10 },
			{ 1024, 8 },
			{ 1280, 2 },
			{ 1536, 1 },
		},
		{	{ 0, 16 },
			{ 256, 16 },
			{ 512, 10 },
			{ 768, 10 },
			{ 1024, 10 },
			{ 1280, 8 },
			{ 1536, 5 },
			{ 1792, 0 },
		},
		{	{ 0, 16 },
			{ 256, 16 },
			{ 512, 10 },
			{ 768, 10 },
			{ 1024, 8 },
			{ 1280, 8 },
			{ 1536, 5 },
			{ 1792, 0 },
		},
		{	{ 0, 10 },
			{ 256, 10 },
			{ 512, 10 },
			{ 768, 8 },
			{ 1024, 8 },
			{ 1280, 2 },
			{ 1536, 0 },
			{ 1792, 0 },
		},
	},
	1,
};

ISP_GAMMA_ATTR_S stGammaAttr = {0,347,539,679,794,894,982,1062,1136,1204,1268,1329,1386,1441,1493,1543,1591,1638,1683,1726,1768,1809,1849,1888,1926,1963,1999,2034,2068,2102,2135,2168,2200,2231,2262,2292,2322,2351,2380,2408,2436,2463,2491,2517,2544,2570,2596,2621,2646,2671,2695,2719,2743,2767,2790,2814,2836,2859,2882,2904,2926,2948,2969,2990,3012,3033,3053,3074,3094,3115,3135,3155,3174,3194,3213,3233,3252,3271,3290,3308,3327,3345,3364,3382,3400,3418,3436,3453,3471,3488,3506,3523,3540,3557,3574,3591,3607,3624,3640,3657,3673,3689,3705,3721,3737,3753,3769,3785,3800,3816,3831,3846,3862,3877,3892,3907,3922,3937,3951,3966,3981,3995,4010,4024,4039,4053,4067,4081,4095};

ISP_IRIDIX_ATTR_S stIridixAttr = {
	.enOpType = OP_TYPE_MANUAL,
	.stAuto = {
		30,
		{3750000,3574729},
		2557570,
		{20,95,800,2000,8,20,7680,12800,0,40,40,7680,10240,20,0},
	},
	.stManual = {
		2, 640,
		2, 640,
		64, 512,
	},
};

ISP_CNR_ATTR_S stCnrAttr = {
	{
		{ 0, 1500 },
		{ 256, 2000 },
		{ 512, 2100 },
		{ 768, 2100 },
		{ 1024, 3500 },
		{ 1280, 4100 },
		{ 1536, 5900 },
		{ 1792, 6100 },
	},
};

ISP_SINTER_ATTR_S stSinterAttr = {
	1,
	{
		{	{ 0, 35 },
			{ 256, 43 },
			{ 512, 53 },
			{ 768, 65 },
			{ 1024, 65 },
			{ 1280, 70 },
			{ 1536, 78 },
			{ 1792, 82 },
		},
		{	{ 0, 155 },
			{ 256, 155 },
			{ 512, 125 },
			{ 768, 115 },
			{ 1024, 115 },
			{ 1280, 115 },
			{ 1536, 85 },
			{ 1792, 85 },
		},
		{	{ 0, 0 },
			{ 256, 0 },
			{ 512, 2 },
			{ 768, 3 },
			{ 1024, 4 },
			{ 1280, 4 },
			{ 1536, 5 },
		},
		{	{ 0, 0 },
			{ 256, 0 },
			{ 512, 0 },
			{ 768, 5 },
			{ 1024, 64 },
			{ 1280, 64 },
			{ 1536, 128 },
		},
		{	{ 0, 0 },
			{ 256, 0 },
			{ 512, 0 },
			{ 768, 5 },
			{ 1024, 64 },
			{ 1280, 64 },
			{ 1536, 128 },
		},
	},
	{
		16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	},
};

ISP_TEMPER_ATTR_S stTemperAttr = {
	{	{ 0, 120 },
		{ 256, 120 },
		{ 512, 120 },
		{ 768, 120 },
		{ 1024, 140 },
		{ 1280, 145 },
		{ 1536, 195 },
		{ 1792, 195 },
	},
	2,
};

ISP_CSC_ATTR_S stCSCAttr = {
	10, 1020,
	10, 1020,
	1021, 1021, 1021,
	{
		68, 68, 68, 32666, 32666, 68, 68, 32888, 32888,
		0, 0, 0,
	},
};

MESH_SHADING_ATTR_S stMeshShadingAttr = {
	1, 1, 0, 63, 63, 1024
};

RADIAL_SHADING_ATTR_S stRadialShadingAttr = {
	1,
	1920, 266, 1920, 266, 25200, 65000,
	1920, 128, 1920, 128, 3, 3,
};

ISP_SCENE_MODES_ATTR_S stSceneModesAttr = {
	0x41, 0x80, 0x81, 0x82, 0x83,
};

int sockfd;
FILE *fp;

void isp_stats_client_init()
{
        struct sockaddr_in servaddr;

        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("socket error");
                exit(1);
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(PORT);
        if(inet_pton(AF_INET, IP_ADDR, &servaddr.sin_addr) < 0) {
                perror("inet_pton error");
                exit(1);
        }

        if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                perror("connect error");
                exit(1);
        }
}

void isp_stats_client_deinit()
{
        close(sockfd);
}

void *ae_init_func(uint32_t ctx_id)
{
	pr_info("ctx id is %d", ctx_id);

	return NULL;
}

int32_t ae_proc_func(void *ae_ctx, ae_stats_data_t *stats, ae_input_data_t *input, ae_output_data_t *output)
{
	pr_info("AE Stats: fullhist_size %d, fullhist_sum %d, zone_hist_size %d",
		stats->fullhist_size, stats->fullhist_sum, stats->zone_hist_size);
	ae_acamera_input_t *ae_inputdata = (ae_acamera_input_t *)input->acamera_input;
	ae_5bin_info_t  ae5data = ae_inputdata->ae_5bin_data;

	ae_acamera_output_t *ae_acamera = (ae_acamera_output_t *)output->acamera_output;
	ae_acamera->ae_out_info.line[0] = 120;
	ae_acamera->ae_out_info.line_num = 1;
	ae_acamera->ae_out_info.sensor_again[0] = 15 << (18 - 5);
	ae_acamera->ae_out_info.sensor_again_num = 1;
	ae_acamera->ae_out_info.sensor_dgain[0] = 0;
	ae_acamera->ae_out_info.sensor_dgain_num = 1;
	ae_acamera->ae_out_info.isp_dgain = 21 << (18 - 5);

	return 0;
}

int32_t ae_deinit_func(void *ae_ctx)
{
	pr_info("done");

	return 0;
}

void *awb_init_func(uint32_t ctx_id)
{
	pr_info("ctx id is %d", ctx_id);

	return NULL;
}

int32_t awb_proc_func(void *awb_ctx, awb_stats_data_t *stats, awb_input_data_t *input, awb_output_data_t *output)
{
	pr_info("AWB Stats: zones_size %d, rg %d, bg %d, sum %d",
		stats->zones_size, stats->awb_zones[0].rg, stats->awb_zones[0].bg, stats->awb_zones[0].sum);

	return 0;
}

int32_t awb_deinit_func(void *awb_ctx)
{
	pr_info("done");

	return 0;
}

void *af_init_func(uint32_t ctx_id)
{
	pr_info("ctx id is %d", ctx_id);

	return NULL;
}

int32_t af_proc_func(void *af_ctx, af_stats_data_t *stats, af_input_data_t *input, af_output_data_t *output)
{
	pr_info("AF Stats: zones_size %d", stats->zones_size);

	return 0;
}

int32_t af_deinit_func(void *af_ctx)
{
	pr_info("done");

	return 0;
}

void print_help(char *name)
{
	printf("============================================\n");
	printf("APP: %s\n", name);
	printf("%c: AE\n", IDX_AE);
	printf("%c: AF\n", IDX_AF);
	printf("%c: AWB\n", IDX_AWB);
	printf("%c: BL\n", IDX_BL);
	printf("%c: DEMOSAIC\n", IDX_DEMOSAIC);
	printf("%c: SHARPEN\n", IDX_SHARPEN);
	printf("%c: GAMMA\n", IDX_GAMMA);
	printf("%c: IRIDIX\n", IDX_IRIDIX);
	printf("%c: CNR\n", IDX_CNR);
	printf("%c: SINTER\n", IDX_SINTER);
	printf("%c: TEMPER\n", IDX_TEMPER);
	printf("%c: SCENE_MODES\n", IDX_SCENE_MODES);
	printf("%c: FIRMWARE STATE\n", IDX_FWSTATE);
	printf("%c: MODULE CONTROL\n", IDX_MODCTRL);
	printf("%c: REGISTER\n", IDX_REGISTER);
	printf("%c: LIBREG_AE\n", IDX_LIBREG_AE);
	printf("%c: LIBREG_AWB\n", IDX_LIBREG_AWB);
	printf("%c: LIBREG_AF\n", IDX_LIBREG_AF);
	printf("%c: METERING AE(read only)\n", IDX_METERING_AE);
	printf("%c: METERING AWB(read only)\n", IDX_METERING_AWB);
	printf("%c: METERING AF(read only)\n", IDX_METERING_AF);
	printf("%c: METERING AE_5BIN(read only)\n", IDX_METERING_AE_5BIN);
	printf("%c: METERING_DATA_TIME(read only)\n", IDX_METERING_DATA_TIME);
	printf("%c: SWITCH SCENCE\n", IDX_SWITCH_SCENCE);
	printf("%c: CSC\n", IDX_CSC);
	printf("%c: MESH SHADING\n", IDX_MESH_SHADING);
	printf("%c: MESH SHADING LUT\n", IDX_MESH_SHADING_LUT);
	printf("%c: RADIAL SHADING\n", IDX_RADIAL_SHADING);
	printf("%c: RADIAL SHADING LUT\n", IDX_RADIAL_SHADING_LUT);
	printf("%c: IRIDIX STRENGTH LEVEL\n", IDX_IRIDIX_STRENGTH_LEVEL);
	printf("%c: IDX_IRQ_SYNC\n", IDX_IRQ_SYNC);
	printf("%c: IDX_AWB_ZONE\n", IDX_AWB_ZONE);
	printf("%c: IDX_AF_ZONE\n", IDX_AF_ZONE);
	printf("%c: IDX_AF_KERNEL\n", IDX_AF_KERNEL);
	printf("%c: IDX_AEROI_INFO\n", IDX_AEROI_INFO);
	printf("%c: IDX_LUMA_INFO\n", IDX_LUMA_INFO);
	printf("%c: IDX_AEPARAM_INFO\n", IDX_AEPARAM_INFO);
	printf("%c: IDX_AE5BIN_ZONE\n", IDX_AE5BIN_ZONE);
	printf("%c: IDX_AE_ZONE\n", IDX_AE_ZONE);
	printf("%c: IDX_AE_EX\n", IDX_AE_EX);
	printf("y: Help\n");
	printf("Q: Exit\n");
	printf("============================================\n");
}

void print_data(int chn, int type, int bytewidth, void *data, int size)
{
	int i;
	uint8_t *ptr8 = data;
	uint16_t *ptr16 = data;
	uint32_t *ptr32 = data;

	switch (type) {
	case IDX_AE:
		printf("===AE===\n");
		break;
	case IDX_AF:
		printf("===AF===\n");
		break;
	case IDX_AWB:
		printf("===AWB===\n");
		break;
	case IDX_BL:
		printf("===BL===\n");
		break;
	case IDX_DEMOSAIC:
		printf("===DEMOSAIC===\n");
		break;
	case IDX_SHARPEN:
		printf("===SHARPEN===\n");
		break;
	case IDX_GAMMA:
		printf("===GAMMA===\n");
		break;
	case IDX_IRIDIX:
		printf("===IRIDIX===\n");
		break;
	case IDX_CNR:
		printf("===CNR===\n");
		break;
	case IDX_SINTER:
		printf("===SINTER===\n");
		break;
	case IDX_TEMPER:
		printf("===TEMPER===\n");
		break;
	case IDX_SCENE_MODES:
		printf("===SCENE_MODES===\n");
		break;
	case IDX_CSC:
		printf("===IDX_CSC===\n");
		break;
	case IDX_MESH_SHADING:
		printf("===IDX_MESH_SHADING===\n");
		break;
	case IDX_RADIAL_SHADING:
		printf("===IDX_RADIAL_SHADING===\n");
		break;
	case IDX_MESH_SHADING_LUT:
		printf("===IDX_MESH_SHADING_LUT===\n");
		break;
	case IDX_RADIAL_SHADING_LUT:
		printf("===IDX_RADIAL_SHADING_LUT===\n");
		break;
	default:
		break;
	}

	for (i = 0; i < size / bytewidth; i++) {
		if (bytewidth == DWORD)
			printf("%-8d ", ptr32[i]);
		if (bytewidth == WORD)
			printf("%-4d ", ptr16[i]);
		if (bytewidth == BYTE)
			printf("%-4d ", ptr8[i]);
		if ((i + 1) % 2 == 0)
			printf("\n");
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int inited = 0;
	int dir = 0;
	int mode = 0;
	int ctx_idx = 0;
	char c, type;
	uint8_t irq_type = 0;
	uint8_t zone_h = 0;
	uint8_t zone_v = 0;
	uint8_t x_start = 0;
	uint8_t x_end = 0;
	uint8_t y_start = 0;
	uint8_t y_end = 0;
	uint32_t temp_data = 0;
	uint64_t timeout = 0;
	char filename[200];

	print_help(argv[0]);
start:
	printf("ISP_TEST> ");

	system("stty raw");
	c = getchar();
	system("stty -raw");
	printf("\n");
	if (c == 'Q') goto out;
	if (c == 'y') {
		print_help(argv[0]);
		goto start;
	}

	type = c;

	if (type >= IDX_MAX) {
		pr_err("invalid idx %d", type);
		exit(0);
	}

	if ((type >= IDX_AE && type <= IDX_REGISTER) ||
		(type >= IDX_CSC && type <= IDX_IRIDIX_STRENGTH_LEVEL) ||
		(type == IDX_AE_EX)) {
		printf("dir(1:r, 0:w)> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		dir = atoi(&c);
	}

	if (type == IDX_AE || type == IDX_AWB || type == IDX_BL || type == IDX_IRIDIX) {
		printf("mode(1:manual, 0:auto)> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		mode = atoi(&c);
	}

	if (type == IDX_METERING_AE || type == IDX_METERING_AWB || type == IDX_METERING_AF || type == IDX_METERING_AE_5BIN || type == IDX_LUMA_INFO ||
		type == IDX_METERING_DATA_TIME) {
		printf("ctx idx> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		ctx_idx = atoi(&c);
	}

	if (type == IDX_LIBREG_AE || type == IDX_LIBREG_AWB || type == IDX_LIBREG_AF) {
		if (inited) {
			pr_info("LIBREG test must at first time app executed");
			goto start;
		}
	}
	if (type == IDX_SWITCH_SCENCE) {
		printf("ctx idx> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		ctx_idx = atoi(&c);
		printf("please input file path & name>: \n");
		//gets(filename);
		scanf("%s", filename);
	}

	if (type == IDX_IRQ_SYNC) {
		printf("ctx idx> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		ctx_idx = atoi(&c);
		printf("mode(0:frame_start, 1:frame_end)> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		irq_type = atoi(&c);
		printf("\n");
		printf("input time> ");
		system("stty raw");
		scanf("%ld", &timeout);
		system("stty -raw");
		printf("\n");
		printf("type %d, timeout %ld\n", irq_type, timeout);
	}

	if ((type == IDX_AWB_ZONE) || (type == IDX_AF_ZONE) || (type == IDX_AE5BIN_ZONE)
			|| (type == IDX_AE_ZONE)) {
		printf("ctx idx> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		ctx_idx = atoi(&c);
		printf("input zone_h size (0:33]> ");
		system("stty raw");
		scanf("%hhd", &zone_h);
		system("stty -raw");
		printf("\n");
		printf("input zone_v size (0:33]> ");
		system("stty raw");
		scanf("%hhd", &zone_v);
		system("stty -raw");
		printf("\n");
		printf("zone_h %d, zone_v %d\n", zone_h, zone_v);
	}

	if (type == IDX_AEROI_INFO) {
		printf("ctx idx> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		ctx_idx = atoi(&c);
		printf("dir(1:r, 0:w)> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		dir = atoi(&c);
		if (dir == 1)
			goto get_end_aeroi;
		printf("\n");
		printf("input x_start size [0:255]> ");
		system("stty raw");
		scanf("%hhd", &x_start);
		system("stty -raw");
		printf("\n");
		printf("input x_end size [0:255]> ");
		system("stty raw");
		scanf("%hhd", &x_end);
		system("stty -raw");
		printf("\n");
		printf("input y_start size [0:255]> ");
		system("stty raw");
		scanf("%hhd", &y_start);
		system("stty -raw");
		printf("\n");
		printf("input y_end size [0:255]> ");
		system("stty raw");
		scanf("%hhd", &y_end);
		system("stty -raw");
		printf("\n");
		printf("x_start %d, x_end %d, y_start %d, y_end %d\n", x_start, x_end, y_start, y_end);
get_end_aeroi:
		printf("\n");
	}

	if (type == IDX_AEPARAM_INFO) {
		printf("ctx idx> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		ctx_idx = atoi(&c);
		printf("dir(1:r, 0:w)> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		dir = atoi(&c);
		if (dir == 1)
			goto get_end_aeparam;
		printf("\n");
		printf("input shutter size [0:50000]> ");
		system("stty raw");
		x_start = 0;
		scanf("%hhd", &x_start);
		system("stty -raw");
		printf("\n");
		printf("input x_end size [0:765]> ");
		system("stty raw");
		x_end = 0;
		scanf("%hhd", &x_end);
		system("stty -raw");
		printf("\n");
		printf("shutter %d, linear %d\n", x_start, x_end);
get_end_aeparam:
		printf("\n");
	}

	if (type == IDX_AF_KERNEL) {
		printf("ctx idx> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		ctx_idx = atoi(&c);

		printf("dir(1:r, 0:w)> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		dir = atoi(&c);
		if (dir == 1)
			goto get_end;
		printf("af_kernel [0:3]> ");
		system("stty raw");
		c = getchar();
		system("stty -raw");
		printf("\n");
		temp_data = atoi(&c);
get_end:
		printf("\n");
	}

	switch (type) {
	case IDX_LIBREG_AE:
	{
		ISP_AE_FUNC_S stAeFunc = {
			.init_func = ae_init_func,
			.proc_func = ae_proc_func,
			.deinit_func = ae_deinit_func,
		};
		HB_ISP_AELibRegCallback(0, "libae.so", &stAeFunc);
	}
		break;
	case IDX_LIBREG_AWB:
	{
		ISP_AWB_FUNC_S stAwbFunc = {
			.init_func = awb_init_func,
			.proc_func = awb_proc_func,
			.deinit_func = awb_deinit_func,
		};
		HB_ISP_AWBLibRegCallback(0, "libawb.so", &stAwbFunc);
	}
		break;
	case IDX_LIBREG_AF:
	{
		ISP_AF_FUNC_S stAfFunc = {
			.init_func = af_init_func,
			.proc_func = af_proc_func,
			.deinit_func = af_deinit_func,
		};
		HB_ISP_AFLibRegCallback(0, "libaf.so", &stAfFunc);
	}
		break;
	}

	if (!inited) {
		inited = 1;
		HB_ISP_GetSetInit();
	}

	if (type == IDX_LIBREG_AE || type == IDX_LIBREG_AWB || type == IDX_LIBREG_AF)
		goto start;

	switch (type) {
	case IDX_AE:
		if (!dir) {
			stAeAttr.enOpType = mode;
			HB_ISP_SetAeAttr(0, &stAeAttr);
		} else {
			memset(&stAeAttr, 0, sizeof(stAeAttr));
			stAeAttr.enOpType = mode;
			HB_ISP_GetAeAttr(0, &stAeAttr);
			print_data(0, type, DWORD, &stAeAttr, sizeof(stAeAttr));
		}
		break;
	case IDX_AF:
		if (!dir)
			pr_info("not support until now.");
		else
			pr_info("not support until now.");
		break;
	case IDX_AWB:
		if (!dir) {
			stAwbAttr.enOpType = mode;
			HB_ISP_SetAwbAttr(0, &stAwbAttr);
		} else {
			memset(&stAwbAttr, 0, sizeof(stAwbAttr));
			stAwbAttr.enOpType = mode;
			HB_ISP_GetAwbAttr(0, &stAwbAttr);
			print_data(0, type, DWORD, &stAwbAttr, sizeof(stAwbAttr));
		}
		break;
	case IDX_BL:
		if (!dir) {
			stBackLevelAttr.enOpType = mode;
			HB_ISP_SetBlackLevelAttr(0, &stBackLevelAttr);
		} else {
			memset(&stBackLevelAttr, 0, sizeof(stBackLevelAttr));
			stBackLevelAttr.enOpType = mode;
			HB_ISP_GetBlackLevelAttr(0, &stBackLevelAttr);
			print_data(0, type, DWORD, &stBackLevelAttr, sizeof(stBackLevelAttr));
		}
		break;
	case IDX_DEMOSAIC:
		if (!dir)
			HB_ISP_SetDemosaicAttr(0, &stDemosaicAttr);
		else {
			memset(&stDemosaicAttr, 0, sizeof(stDemosaicAttr));
			HB_ISP_GetDemosaicAttr(0, &stDemosaicAttr);
			print_data(0, type, DWORD, &stDemosaicAttr, sizeof(stDemosaicAttr) - sizeof(stDemosaicAttr.u16NpOffset));
			print_data(0, type, WORD, (void *)stDemosaicAttr.u16NpOffset, sizeof(stDemosaicAttr.u16NpOffset));
		}
		break;
	case IDX_SHARPEN:
		if (!dir)
			HB_ISP_SetSharpenAttr(0, &stSharpenAttr);
		else {
			memset(&stSharpenAttr, 0, sizeof(stSharpenAttr));
			HB_ISP_GetSharpenAttr(0, &stSharpenAttr);
			print_data(0, type, WORD, &stSharpenAttr, sizeof(stSharpenAttr));
		}
		break;
	case IDX_GAMMA:
		if (!dir)
			HB_ISP_SetGammaAttr(0, &stGammaAttr);
		else {
			memset(&stGammaAttr, 0, sizeof(stGammaAttr));
			HB_ISP_GetGammaAttr(0, &stGammaAttr);
			print_data(0, type, WORD, &stGammaAttr, sizeof(stGammaAttr));
		}
		break;
	case IDX_IRIDIX:
		if (!dir) {
			stIridixAttr.enOpType = mode;
			HB_ISP_SetIridixAttr(0, &stIridixAttr);
		} else {
			memset(&stIridixAttr, 0, sizeof(stIridixAttr));
			stIridixAttr.enOpType = mode;
			HB_ISP_GetIridixAttr(0, &stIridixAttr);
			print_data(0, type, BYTE, &stIridixAttr.stAuto, sizeof(stIridixAttr.stAuto.u8AvgCoef));
			print_data(0, type, DWORD, (void *)stIridixAttr.stAuto.au32EvLimNoStr, sizeof(stIridixAttr.stAuto) - sizeof(stIridixAttr.stAuto.u8AvgCoef));
			print_data(0, type, DWORD, &stIridixAttr.stManual, sizeof(stIridixAttr.stManual));
		}
		break;
	case IDX_CNR:
		if (!dir)
			HB_ISP_SetCnrAttr(0, &stCnrAttr);
		else {
			memset(&stCnrAttr, 0, sizeof(stCnrAttr));
			HB_ISP_GetCnrAttr(0, &stCnrAttr);
			print_data(0, type, WORD, &stCnrAttr, sizeof(stCnrAttr));
		}
		break;
	case IDX_SINTER:
		if (!dir)
			HB_ISP_SetSinterAttr(0, &stSinterAttr);
		else {
			memset(&stSinterAttr, 0, sizeof(stSinterAttr));
			HB_ISP_GetSinterAttr(0, &stSinterAttr);
			print_data(0, type, WORD, &stSinterAttr, sizeof(stSinterAttr));
		}
		break;
	case IDX_TEMPER:
		if (!dir)
			HB_ISP_SetTemperAttr(0, &stTemperAttr);
		else {
			memset(&stTemperAttr, 0, sizeof(stTemperAttr));
			HB_ISP_GetTemperAttr(0, &stTemperAttr);
			print_data(0, type, WORD, &stTemperAttr, sizeof(stTemperAttr));
			print_data(0, type, DWORD, &stTemperAttr.u32RecursionLimit, sizeof(stTemperAttr.u32RecursionLimit));
		}
		break;
	case IDX_SCENE_MODES:
		if (!dir)
			HB_ISP_SetSceneModesAttr(0, &stSceneModesAttr);
		else {
			memset(&stSceneModesAttr, 0, sizeof(stSceneModesAttr));
			HB_ISP_GetSceneModesAttr(0, &stSceneModesAttr);
			print_data(0, type, DWORD, &stSceneModesAttr, sizeof(stSceneModesAttr));
		}
		break;
	case IDX_FWSTATE:
		if (!dir)
			HB_ISP_SetFWState(0, ISP_FW_STATE_FREEZE);
		else {
			ISP_FW_STATE_E val;
			HB_ISP_GetFWState(0, &val);
			pr_info("fw state is %d\n", val);
		}
		break;
	case IDX_MODCTRL:
		if (!dir) {
			ISP_MODULE_CTRL_U ctrl;
			ctrl.u64Value = 0xff;
			HB_ISP_SetModuleControl(0, &ctrl);
		} else {
			ISP_MODULE_CTRL_U ctrl;
			HB_ISP_GetModuleControl(0, &ctrl);
			pr_info("module control get value 0x%lx\n", ctrl.u64Value);
		}
		break;
	case IDX_REGISTER:
		if (!dir)
			HB_ISP_SetRegister(0, 0x18e88, 0x4380780);  // 1080P
		else {
			int val;
			HB_ISP_GetRegister(0, 0x18e88, &val);
			pr_info("active w/h is 0x%x\n", val);
		}
		break;
	case IDX_METERING_AE:
	{
        int i;
        uint32_t ae[HB_ISP_FULL_HISTOGRAM_SIZE];
		ISP_ZONE_ATTR_S aeZoneInfo;

        memset(ae, 0, sizeof(ae));
		ret = HB_ISP_GetAeFullHist(ctx_idx, ae);

        printf("\n--AE--\n");
		if (!ret) {
			HB_ISP_GetAeZoneInfo(ctx_idx, &aeZoneInfo);
			for (i = 0; i < HB_ISP_MAX_AE_ZONES; i++) {
			        printf("%-8d    ", ae[i]);
			        if ((i + 1) % 8 == 0)
			              printf("\n");
				if (i >= aeZoneInfo.u8Horiz * aeZoneInfo.u8Vert) {
					printf("\n--horiz %d, vert %d--\n", aeZoneInfo.u8Horiz, aeZoneInfo.u8Vert);
					break;
				}
			}
		}
	}
		break;
	case IDX_METERING_AWB:
        {
                int i = 0;
                ISP_STATISTICS_AWB_ZONE_ATTR_S awb[HB_ISP_MAX_AWB_ZONES];
				ISP_ZONE_ATTR_S awbZoneInfo;
			for (int k = 0; k < 10; k++) {
                memset(awb, 0, sizeof(awb));
                ret = HB_ISP_GetAwbZoneHist(ctx_idx, awb);

                printf("\n--AWB--\n");
		if (!ret) {
			HB_ISP_GetAwbZoneInfo(ctx_idx, &awbZoneInfo);
        		for (i = 0; i < HB_ISP_MAX_AWB_ZONES; i++) {
        		        printf("rg %-4d, bg %-4d, sum %-4d\n", awb[i].u16Rg, awb[i].u16Bg, awb[i].u32Sum);
				if (i >= awbZoneInfo.u8Horiz * awbZoneInfo.u8Vert) {
					printf("\n--horiz %d, vert %d--\n", awbZoneInfo.u8Horiz, awbZoneInfo.u8Vert);
					break;
				}
        		}
		}
        }
		}
                break;
	case IDX_METERING_DATA_TIME:
        {
         int i = 0;
         ISP_STATISTICS_AWB_ZONE_ATTR_S awb[HB_ISP_MAX_AWB_ZONES];
		 ISP_ZONE_ATTR_S awbZoneInfo;

		 memset(awb, 0, sizeof(awb));
		 printf("\n--AWB--\n");
         ret = HB_ISP_GetMeteringData(ctx_idx, awb, ISP_AWB, 1);
		if (!ret) {
			HB_ISP_GetAwbZoneInfo(ctx_idx, &awbZoneInfo);
			for (i = 0; i < HB_ISP_MAX_AWB_ZONES; i++) {
				printf("rg %-4d, bg %-4d, sum %-4d\n", awb[i].u16Rg, awb[i].u16Bg, awb[i].u32Sum);
				if (i >= awbZoneInfo.u8Horiz * awbZoneInfo.u8Vert) {
					printf("\n--horiz %d, vert %d--\n", awbZoneInfo.u8Horiz, awbZoneInfo.u8Vert);
					break;
				}
			}
		}
		}
                break;
	case IDX_METERING_AF:
        {
                int i = 0;
		uint32_t af_data[HB_ISP_AF_ZONES_COUNT_MAX * 2];
		af_stats_data_t af;
		ISP_ZONE_ATTR_S afZoneInfo;

		memset(af_data, 0, sizeof(af_data));
		af.zones_stats = (uint32_t *)&af_data;

                ret = HB_ISP_GetAfZoneHist(ctx_idx, &af);

		printf("\n--AF--\n");
		if (!ret) {
			HB_ISP_GetAfZoneInfo(ctx_idx, &afZoneInfo);
        		for (i = 0; i < HB_ISP_AF_ZONES_COUNT_MAX; i++) {
        		        printf("zone %-4d, af %-4d\n", i, af_data[i]);
				if (i >= afZoneInfo.u8Horiz * afZoneInfo.u8Vert) {
					printf("\n--horiz %d, vert %d--\n", afZoneInfo.u8Horiz, afZoneInfo.u8Vert);
					break;
				}
        		}
		}
        }
                break;
	case IDX_METERING_AE_5BIN:
        {
                int i = 0;
		ISP_STATISTICS_AE_5BIN_ZONE_ATTR_S ae_5bin[HB_ISP_MAX_AE_5BIN_ZONES];
		ISP_ZONE_ATTR_S ae5binZoneInfo;

                memset(ae_5bin, 0, sizeof(ae_5bin));
                ret = HB_ISP_GetAe5binZoneHist(ctx_idx, ae_5bin);

		printf("\n--AE_5BIN--\n");
		if (!ret) {
			HB_ISP_GetAe5binZoneInfo(ctx_idx, &ae5binZoneInfo);
        		for (i = 0; i < HB_ISP_MAX_AE_5BIN_ZONES; i++) {
        		        printf("zone num, %-4d, bin_0 %-4d, bin_1 %-4d, bin_3 %-4d, bin_4 %-4d\n", i,
					ae_5bin[i].u16Hist0, ae_5bin[i].u16Hist1, ae_5bin[i].u16Hist3, ae_5bin[i].u16Hist4);
				if (i >= ae5binZoneInfo.u8Horiz * ae5binZoneInfo.u8Vert) {
					printf("\n--horiz %d, vert %d--\n", ae5binZoneInfo.u8Horiz, ae5binZoneInfo.u8Vert);
					break;
				}
        		}
		}
        }
                break;
	case IDX_LUMA_INFO:
        {
                int i = 0;
		ISP_STATISTICS_LUMVAR_ZONE_ATTR_S pst32Luma[512];

                memset(pst32Luma, 0, sizeof(pst32Luma));
                ret = HB_ISP_GetLumaZoneHist(ctx_idx, pst32Luma);

		printf("\n--LUMA--\n");
		if (!ret) {
        		for (i = 0; i < 512; i++) {
        		        printf("zone num, %-4d, var %-4d, mean %-4d\n", i,
					pst32Luma[i].u16Var, pst32Luma[i].u16Mean);
        		}
		}
        }
                break;
	case IDX_SWITCH_SCENCE:
	{
		int i = 0;
		int ret = 0;
		ret = HB_ISP_SwitchScence((uint8_t)ctx_idx, filename);
		if (ret < 0) {
			printf("switch failed\n");
		}
	}
		break;
	case IDX_CSC:
		if (!dir)
			HB_ISP_SetCSCAttr(0, &stCSCAttr);
		else {
			memset(&stCSCAttr, 0, sizeof(stCSCAttr));
			HB_ISP_GetCSCAttr(0, &stCSCAttr);
			print_data(0, type, DWORD, &stCSCAttr, sizeof(uint32_t) * 7);
			print_data(0, type, WORD, &stCSCAttr.aau16Coefft, sizeof(stCSCAttr.aau16Coefft));
		}
		break;
	case IDX_MESH_SHADING:
		if (!dir)
			HB_ISP_SetMeshShadingAttr(0, &stMeshShadingAttr);
		else {
			memset(&stMeshShadingAttr, 0, sizeof(stMeshShadingAttr));
			HB_ISP_GetMeshShadingAttr(0, &stMeshShadingAttr);
			print_data(0, type, DWORD, &stMeshShadingAttr, sizeof(stMeshShadingAttr));
		}
		break;
	case IDX_MESH_SHADING_LUT:
	{
		int i;
		MESH_SHADING_LUT_S *meshLUT = calloc(1, sizeof(MESH_SHADING_LUT_S));
		for (i = 0; i < 1024; i ++) {
			meshLUT->au8LsAR[i] = 32;
			meshLUT->au8LsAG[i] = 64;
			meshLUT->au8LsAB[i] = 128;
		}
		if (!dir)
			HB_ISP_SetMeshShadingLUT(0, meshLUT);
		else {
			memset(meshLUT, 0, sizeof(MESH_SHADING_LUT_S));
			HB_ISP_GetMeshShadingLUT(0, meshLUT);
			print_data(0, type, DWORD, meshLUT, sizeof(MESH_SHADING_LUT_S));
		}
		free(meshLUT);
	}
		break;
	case IDX_RADIAL_SHADING:
		if (!dir)
			HB_ISP_SetRadialShadingAttr(0, &stRadialShadingAttr);
		else {
			memset(&stRadialShadingAttr, 0, sizeof(stRadialShadingAttr));
			HB_ISP_GetRadialShadingAttr(0, &stRadialShadingAttr);
			print_data(0, type, DWORD, &stRadialShadingAttr, sizeof(stRadialShadingAttr));
		}
		break;
	case IDX_RADIAL_SHADING_LUT:
	{
		int i;
		RADIAL_SHADING_LUT_S radialLUT;
		for (i = 0; i < 129; i ++) {
			radialLUT.au16RGain[i] = 32;
			radialLUT.au16GGain[i] = 64;
			radialLUT.au16BGain[i] = 128;
		}
		if (!dir)
			HB_ISP_SetRadialShadingLUT(0, &radialLUT);
		else {
			memset(&radialLUT, 0, sizeof(radialLUT));
			HB_ISP_GetRadialShadingLUT(0, &radialLUT);
			print_data(0, type, DWORD, &radialLUT, sizeof(radialLUT));
		}
	}
		break;
	case IDX_IRIDIX_STRENGTH_LEVEL:
	{
		uint16_t level = 88;
		if (!dir)
			HB_ISP_SetIridixStrengthLevel(0, level);
		else {
			level = 0;
			HB_ISP_GetIridixStrengthLevel(0, &level);
			printf("level = %d\n", level);
		}
	}
		break;
	case IDX_IRQ_SYNC:
	{

		if (HB_ISP_GetVDTTimeOut(ctx_idx, irq_type, timeout)) {
			printf("irq sync failed, timeout!\n");
		}
	}
		break;
	case IDX_AWB_ZONE:
	{
		ISP_ZONE_ATTR_S awbZoneInfo;
		awbZoneInfo.u8Horiz = zone_h;
		awbZoneInfo.u8Vert = zone_v;
		HB_ISP_SetAwbZoneInfo(ctx_idx, awbZoneInfo);
	}
		break;
	case IDX_AF_ZONE:
	{
		ISP_ZONE_ATTR_S afZoneInfo;
		afZoneInfo.u8Horiz = zone_h;
		afZoneInfo.u8Vert = zone_v;
		HB_ISP_SetAfZoneInfo(ctx_idx, afZoneInfo);
	}
		break;
	case IDX_AE5BIN_ZONE:
	{
		ISP_ZONE_ATTR_S ae5binZoneInfo;
		ae5binZoneInfo.u8Horiz = zone_h;
		ae5binZoneInfo.u8Vert = zone_v;
		HB_ISP_SetAe5binZoneInfo(ctx_idx, ae5binZoneInfo);
	}
		break;
	case IDX_AE_ZONE:
	{
		ISP_ZONE_ATTR_S aeZoneInfo;
		aeZoneInfo.u8Horiz = zone_h;
		aeZoneInfo.u8Vert = zone_v;
		HB_ISP_SetAeZoneInfo(ctx_idx, aeZoneInfo);
	}
		break;
	case IDX_AF_KERNEL:
	{
		uint32_t af_kernel = 5;
		if (!dir) {
			af_kernel = temp_data;
			HB_ISP_SetAfKernelInfo(ctx_idx, af_kernel);
		} else {
			HB_ISP_GetAfKernelInfo(ctx_idx, &af_kernel);
			printf("af_kernel is %d\n", af_kernel);
		}
	}
		break;
	case IDX_AEROI_INFO:
	{
		ISP_AE_ROI_ATTR_S aeRoiInfo;
		if (!dir) {
			aeRoiInfo.u8XStart = x_start;
			aeRoiInfo.u8YStart = y_start;
			aeRoiInfo.u8XEnd = x_end;
			aeRoiInfo.u8YEnd = y_end;
			HB_ISP_SetAeRoiInfo(ctx_idx, aeRoiInfo);
		} else {
			HB_ISP_GetAeRoiInfo(ctx_idx, &aeRoiInfo);
			printf("x_start is 0x%x, y_start is 0x%x, x_end is 0x%x, y_end is 0x%x\n",
				aeRoiInfo.u8XStart, aeRoiInfo.u8YStart, aeRoiInfo.u8XEnd, aeRoiInfo.u8YEnd);
		}
	}
	break;
	case IDX_AEPARAM_INFO:
	{
		ISP_AE_PARAM_S aeParam;
		if (!dir) {
			aeParam.u32TotalGain = x_end;
			aeParam.GainOpType = 1;
			aeParam.u32IntegrationTime = 1200;
			aeParam.u32ExposureRatio = 1;
			aeParam.IntegrationOpType = 1;
			printf("toal gain is 0x%x, shutter is 0x%x\n", aeParam.u32TotalGain, aeParam.u32IntegrationTime);
			HB_ISP_SetAeParam(ctx_idx, &aeParam);
		} else {
			aeParam.GainOpType = 0;
			aeParam.IntegrationOpType = 0;
			HB_ISP_GetAeParam(ctx_idx, &aeParam);
			printf("toal gain is 0x%x, shutter is 0x%x\n", aeParam.u32TotalGain, aeParam.u32IntegrationTime);
		}
	}
	break;
	case IDX_AE_EX:
	{
		ISP_AE_ATTR_EX_S pstAeAttrEx = {88, 88, 88};
		if (!dir) {
			HB_ISP_SetAeAttrEx(ctx_idx, &pstAeAttrEx);
		} else {
			HB_ISP_GetAeAttrEx(ctx_idx, &pstAeAttrEx);
			printf("%d, %d, %d\n", pstAeAttrEx.u32Compensation, pstAeAttrEx.u32Speed, pstAeAttrEx.u32Tolerance);
		}
	}
	break;
	default:
		pr_err("invalid idx %d", type);
		break;
	}

	goto start;
out:
	HB_ISP_GetSetExit();

	return 0;
}
