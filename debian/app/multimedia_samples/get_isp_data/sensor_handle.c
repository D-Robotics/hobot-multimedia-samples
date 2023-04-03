#include <stdio.h>
#include "stdint.h"
#include "stddef.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include "vio/hb_vio_interface.h"
#include "cam/hb_cam_interface.h"
/*#include <module.h>*/
#include <sensor_handle.h>


#define HW_TIMER 24000
#define MAX_PLANE 4

typedef struct {
	uint32_t frame_id;
	uint32_t plane_count;
	uint32_t xres[MAX_PLANE];
	uint32_t yres[MAX_PLANE];
	char *addr[MAX_PLANE];
	uint32_t size[MAX_PLANE];
} raw_t;

typedef struct {
	uint8_t ctx_id;
	raw_t raw;
} dump_info_t;

typedef struct {
	uint32_t frame_id;
	uint32_t plane_count;
	uint32_t xres;
	uint32_t yres;
	char *addr;
	uint32_t size;
} yuv_t;

typedef struct {
	uint8_t ctx_id;
	yuv_t yuv;
} dump_yuv_info_t;

static int x3_dumpToFile(char *filename, char *srcBuf, unsigned int size)
{
	FILE *yuvFd = NULL;
	char *buffer = NULL;

	yuvFd = fopen(filename, "w+");

	if (yuvFd == NULL) {
		printf("ERRopen(%s) fail", filename);
		return -1;
	}

	buffer = (char *)malloc(size);

	if (buffer == NULL) {
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

	if (yuvFd == NULL) {
		printf("open(%s) fail", filename);
		return -1;
	}

	buffer = (char *)malloc(size + size1);

	if (buffer == NULL) {
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

void x3_normal_buf_info_print(hb_vio_buffer_t * buf)
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

int time_cost_ms(struct timeval *start, struct timeval *end)
{
	int time_ms = -1;
	time_ms = ((end->tv_sec * 1000 + end->tv_usec /1000) -
			(start->tv_sec * 1000 + start->tv_usec /1000));
	printf("time cost %d ms \n", time_ms);
	return time_ms;
}



int yuv_dump_func(void)
{
	struct timeval time_now = { 0 };
	struct timeval time_next = { 0 };
	int size = -1;
	char file_name[128] = { 0 };
	dump_yuv_info_t dump_info = {0};
	int ret = 0, i;
	hb_vio_buffer_t *yuv_buf = NULL;
	int pipeId = 0;

	yuv_buf = (hb_vio_buffer_t *) malloc(sizeof(hb_vio_buffer_t));
	memset(yuv_buf, 0, sizeof(hb_vio_buffer_t));

	/* 从VIN模块获取图像帧 */
	ret = hb_vio_get_data(pipeId, HB_VIO_ISP_YUV_DATA, yuv_buf);
	if (ret < 0)
	{
		printf("hb_vio_get_data error!!!\n");
	} else {
		x3_normal_buf_info_print(yuv_buf);
		size = yuv_buf->img_addr.stride_size * yuv_buf->img_addr.height;
		printf("stride_size(%u) w x h%u x %u  size %d\n",
				yuv_buf->img_addr.stride_size,
				yuv_buf->img_addr.width, yuv_buf->img_addr.height, size);

		dump_info.ctx_id = pipeId;
		dump_info.yuv.frame_id = yuv_buf->img_info.frame_id;
		dump_info.yuv.plane_count = yuv_buf->img_info.planeCount;
		dump_info.yuv.xres = yuv_buf->img_addr.width;
		dump_info.yuv.yres = yuv_buf->img_addr.height;
		dump_info.yuv.addr = yuv_buf->img_addr.addr[0];
		dump_info.yuv.size = size;

		printf("pipe(%d)dump normal yuv frame id(%d),plane(%d)size(%d),img_format(%d)\n",
				dump_info.ctx_id, dump_info.yuv.frame_id,
				dump_info.yuv.plane_count, size,yuv_buf->img_info.img_format);
		
		sprintf(file_name, "pipe%d_%ux%u_frame_%03d.yuv",
				dump_info.ctx_id,
				dump_info.yuv.xres,
				dump_info.yuv.yres,
				dump_info.yuv.frame_id);
		gettimeofday(&time_now, NULL);
		x3_dumpToFile2plane(file_name, yuv_buf->img_addr.addr[0],
				yuv_buf->img_addr.addr[1], size, size/2);
		gettimeofday(&time_next, NULL);
		int time_cost = time_cost_ms(&time_now, &time_next);
		printf("dumpToFile yuv cost time %d ms\n", time_cost);
		
		// ret = HB_VIN_ReleaseChnFrame(pipeId, 0, yuv_buf);
		// if (ret < 0) {
		// 	printf("HB_VIN_ReleaseChnFrame error!!!\n");
		// }
	}
	free(yuv_buf);
	yuv_buf = NULL;
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
