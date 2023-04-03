#include <stdio.h>
#include "stdint.h"
#include "stddef.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "vio/hb_vio_interface.h"
#include "cam/hb_cam_interface.h"
/*#include <module.h>*/
#include <sensor_handle.h>

#define HW_TIMER 24000
#define MAX_PLANE 4

typedef struct
{
	uint32_t frame_id;
	uint32_t plane_count;
	uint32_t xres[MAX_PLANE];
	uint32_t yres[MAX_PLANE];
	char *addr[MAX_PLANE];
	uint32_t size[MAX_PLANE];
} raw_t;

typedef struct
{
	uint8_t ctx_id;
	raw_t raw;
} dump_info_t;

int time_cost_ms(struct timeval *start, struct timeval *end)
{
	int time_ms = -1;
	time_ms = ((end->tv_sec * 1000 + end->tv_usec / 1000) -
			   (start->tv_sec * 1000 + start->tv_usec / 1000));
	return time_ms;
}

static void big_to_little_end(char *srcBuf, char *dstBuf, unsigned int size)
{
	int i = 0;
	uint16_t *in = (uint16_t *)srcBuf;
	uint16_t *out = (uint16_t *)dstBuf;
	uint16_t tmp;
	for (i = 0; i < size / 2; i++)
	{
		tmp = in[i];
		out[i] = ((tmp << 8) | (tmp >> 8)) >> 6;
	}
}

/*
 * 支持从sif直接获取arm raw格式的数据
 * 仅针对数据不需要使用isp的时候才用这种模式
 * 支持本功能需要修改VIN_DEV_ATTR_S结构的以下字段：
 *    stSize.pix_length 为4，代表每个像素数据占用16bit，不足16bit的数进行补0
 *    outDdrAttr.stride 设置为 width 的 2倍
 */
static int x3_dumpToFile_arm_raw(char *filename, char *srcBuf, unsigned int size)
{
	FILE *yuvFd = NULL;
	char *buffer = NULL;
	char *temp = NULL;

	struct timeval time_now = {0};
	struct timeval time_next = {0};

	yuvFd = fopen(filename, "w+");

	if (yuvFd == NULL)
	{
		printf("ERRopen(%s) fail", filename);
		return -1;
	}

	temp = (char *)malloc(size);
	buffer = (char *)malloc(size);

	if (buffer == NULL)
	{
		printf(":malloc file");
		fclose(yuvFd);
		return -1;
	}

	memcpy(temp, srcBuf, size);

	gettimeofday(&time_now, NULL);
	big_to_little_end(temp, buffer, size);
	gettimeofday(&time_next, NULL);
	int time_cost = time_cost_ms(&time_now, &time_next);
	printf("big_to_little_end raw cost time %d ms\n", time_cost);

	/*memcpy(buffer, srcBuf, size);*/

	fflush(stdout);

	fwrite(buffer, 1, size, yuvFd);

	fflush(yuvFd);

	if (yuvFd)
		fclose(yuvFd);
	if (buffer)
		free(buffer);

	printf("filedump(%s, size(%d) is successed\n", filename, size);

	return 0;
}

static int x3_dumpToFile(char *filename, char *srcBuf, unsigned int size)
{
	FILE *yuvFd = NULL;
	char *buffer = NULL;

	yuvFd = fopen(filename, "w+");

	if (yuvFd == NULL)
	{
		printf("ERRopen(%s) fail", filename);
		return -1;
	}

	buffer = (char *)malloc(size);

	if (buffer == NULL)
	{
		printf(":malloc file");
		fclose(yuvFd);
		return -1;
	}

	memcpy(buffer, srcBuf, size);

	fflush(stdout);

	fwrite(buffer, 1, size, yuvFd);

	fflush(yuvFd);

	if (yuvFd)
		fclose(yuvFd);
	if (buffer)
		free(buffer);

	printf("filedump(%s, size(%d) is successed\n", filename, size);

	return 0;
}

static int x3_dumpToFile2plane(char *filename, char *srcBuf, char *srcBuf1,
							   unsigned int size, unsigned int size1)
{

	FILE *yuvFd = NULL;
	char *buffer = NULL;

	yuvFd = fopen(filename, "w+");

	if (yuvFd == NULL)
	{
		printf("open(%s) fail", filename);
		return -1;
	}

	buffer = (char *)malloc(size + size1);

	if (buffer == NULL)
	{
		printf("ERR:malloc file");
		fclose(yuvFd);
		return -1;
	}

	memcpy(buffer, srcBuf, size);
	memcpy(buffer + size, srcBuf1, size1);

	fflush(stdout);

	fwrite(buffer, 1, size + size1, yuvFd);

	fflush(yuvFd);

	if (yuvFd)
		fclose(yuvFd);
	if (buffer)
		free(buffer);

	printf("DEBUG:filedump(%s, size(%d) is successed!!", filename, size);

	return 0;
}

// static void print_sensor_info(MIPI_SENSOR_INFO_S *snsinfo)
// {
// 	printf("bus_num %d\n", snsinfo->sensorInfo.bus_num);
// 	printf("bus_type %d\n", snsinfo->sensorInfo.bus_type);
// 	printf("sensor_name %s\n", snsinfo->sensorInfo.sensor_name);
// 	printf("reg_width %d\n", snsinfo->sensorInfo.reg_width);
// 	printf("sensor_mode %d\n", snsinfo->sensorInfo.sensor_mode);
// 	printf("sensor_addr 0x%x\n", snsinfo->sensorInfo.sensor_addr);
// 	printf("serial_addr 0x%x\n", snsinfo->sensorInfo.serial_addr);
// 	printf("resolution %d\n", snsinfo->sensorInfo.resolution);

// 	return;
// }

void x3_normal_buf_info_print(hb_vio_buffer_t *buf)
{
	printf("normal pipe_id (%d)type(%d)frame_id(%d)buf_index(%d)w x h(%dx%d) data_type %d img_format %d\n",
		   buf->img_info.pipeline_id,
		   buf->img_info.data_type,
		   buf->img_info.frame_id,
		   buf->img_info.buf_index,
		   buf->img_addr.width,
		   buf->img_addr.height,
		   buf->img_info.data_type,
		   buf->img_info.img_format);
}

int sif_dump_func(void)
{
	struct timeval time_now = {0};
	struct timeval time_next = {0};
	int size = -1;
	char file_name[128] = {0};
	dump_info_t dump_info = {0};
	int ret = 0, i;
	hb_vio_buffer_t *sif_raw = NULL;
	int pipeId = 0;

	sif_raw = (hb_vio_buffer_t *)malloc(sizeof(hb_vio_buffer_t));
	memset(sif_raw, 0, sizeof(hb_vio_buffer_t));

	/* 从SIF模块获取图像帧 */
	ret = hb_vio_get_data(pipeId, HB_VIO_SIF_RAW_DATA, sif_raw);
	if (ret < 0)
	{
		printf("hb_vio_get_data error!!!\n");
	}
	else
	{
		x3_normal_buf_info_print(sif_raw);
		size = sif_raw->img_addr.stride_size * sif_raw->img_addr.height;
		printf("stride_size(%u) w x h%u x %u  size %d\n",
			   sif_raw->img_addr.stride_size,
			   sif_raw->img_addr.width, sif_raw->img_addr.height, size);

		if (sif_raw->img_info.planeCount == 1)
		{
			/* 一般是 raw 图 */
			dump_info.ctx_id = pipeId;
			dump_info.raw.frame_id = sif_raw->img_info.frame_id;
			dump_info.raw.plane_count = sif_raw->img_info.planeCount;
			dump_info.raw.xres[0] = sif_raw->img_addr.width;
			dump_info.raw.yres[0] = sif_raw->img_addr.height;
			dump_info.raw.addr[0] = sif_raw->img_addr.addr[0];
			dump_info.raw.size[0] = size;

			printf("pipe(%d)dump normal raw frame id(%d),plane(%d)size(%d)\n",
				   dump_info.ctx_id, dump_info.raw.frame_id,
				   dump_info.raw.plane_count, size);
		}
		else if (sif_raw->img_info.planeCount == 2)
		{
			/* 一般是 yuv 图，如果开启了dol2模式，则是两帧raw图， 根据img_format进行区分 */
			dump_info.ctx_id = pipeId;
			dump_info.raw.frame_id = sif_raw->img_info.frame_id;
			dump_info.raw.plane_count = sif_raw->img_info.planeCount;
			for (int i = 0; i < sif_raw->img_info.planeCount; i++)
			{
				dump_info.raw.xres[i] = sif_raw->img_addr.width;
				dump_info.raw.yres[i] = sif_raw->img_addr.height;
				dump_info.raw.addr[i] = sif_raw->img_addr.addr[i];
				dump_info.raw.size[i] = size;
			}
			if (sif_raw->img_info.img_format == 0)
			{
				printf("pipe(%d)dump dol2 raw frame id(%d),plane(%d)size(%d)\n",
					   dump_info.ctx_id, dump_info.raw.frame_id,
					   dump_info.raw.plane_count, size);
			}
		}
		else if (sif_raw->img_info.planeCount == 3)
		{
			/* 一般是 dol3 模式下的3帧raw图 */
			dump_info.ctx_id = pipeId;
			dump_info.raw.frame_id = sif_raw->img_info.frame_id;
			dump_info.raw.plane_count = sif_raw->img_info.planeCount;
			for (int i = 0; i < sif_raw->img_info.planeCount; i++)
			{
				dump_info.raw.xres[i] = sif_raw->img_addr.width;
				dump_info.raw.yres[i] = sif_raw->img_addr.height;
				dump_info.raw.addr[i] = sif_raw->img_addr.addr[i];
				dump_info.raw.size[i] = size;
			}
			printf("pipe(%d)dump dol3 raw frame id(%d),plane(%d)size(%d)\n",
				   dump_info.ctx_id, dump_info.raw.frame_id,
				   dump_info.raw.plane_count, size);
		}
		else
		{
			printf("pipe(%d)raw buf planeCount wrong !!!\n", pipeId);
		}

		for (int i = 0; i < dump_info.raw.plane_count; i++)
		{
			if (sif_raw->img_info.img_format == 0)
			{
				sprintf(file_name, "pipe%d_plane%d_%ux%u_frame_%03d.raw",
						dump_info.ctx_id,
						i,
						dump_info.raw.xres[i],
						dump_info.raw.yres[i],
						dump_info.raw.frame_id);

				gettimeofday(&time_now, NULL);
				x3_dumpToFile(file_name, dump_info.raw.addr[i], dump_info.raw.size[i]);
				gettimeofday(&time_next, NULL);
				int time_cost = time_cost_ms(&time_now, &time_next);
				printf("dumpToFile raw cost time %d ms\n", time_cost);
			}
		}
		if (sif_raw->img_info.img_format == 8)
		{
			sprintf(file_name, "pipe%d_%ux%u_frame_%03d.yuv",
					dump_info.ctx_id,
					dump_info.raw.xres[i],
					dump_info.raw.yres[i],
					dump_info.raw.frame_id);
			gettimeofday(&time_now, NULL);
			x3_dumpToFile2plane(file_name, sif_raw->img_addr.addr[0],
								sif_raw->img_addr.addr[1], size, size / 2);
			gettimeofday(&time_next, NULL);
			int time_cost = time_cost_ms(&time_now, &time_next);
			printf("dumpToFile yuv cost time %d ms", time_cost);
		}
		ret = hb_vio_free_sifbuf(pipeId, sif_raw);
		if (ret < 0)
		{
			printf("hb_vio_free_sifbuf error!!!\n");
		}
	}
	free(sif_raw);
	sif_raw = NULL;
}

int sensor_sif_dev_init(char *cam_cfg_file, char *vio_cfg_file, int cam_index)
{
	int ret = 0, port = 0;
	// int cam_index = 0;
	// char *cam_cfg_file = "./hb_x3player.json";
	// char *vio_cfg_file = "./sdb_imx219_raw_10bit_1920x1080_offline_Pipeline.json";
	ret = hb_cam_init(cam_index, cam_cfg_file);
	if (ret < 0)
	{
		printf("cam init fail\n");
		return -1;
	}
	ret = hb_vio_init(vio_cfg_file);
	if (ret < 0)
	{
		printf("vio init fail\n");
		return -1;
	}
	ret = hb_vio_start_pipeline(0);
	if (ret < 0)
	{
		printf("vio start fail, do cam&vio deinit.\n");
		hb_cam_deinit(0);
		hb_vio_deinit();
		return -1;
	}

	ret = hb_cam_power_on(port);
	if (ret < 0)
	{
		printf("hb_cam_power_on fail, do cam deinit\n");
		hb_cam_stop(port);
		hb_cam_deinit(cam_index);
		return -1;
	}
	ret = hb_cam_start(port);
	if (ret < 0)
	{
		printf("cam start fail, do cam deinit\n");
		hb_cam_deinit(cam_index);
		return -1;
	}
	return ret;
}
