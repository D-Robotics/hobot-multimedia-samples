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

/*#include <module.h>*/
#include <sensor_handle.h>

/*定义 sensor   初始化的属性信息 */
MIPI_SENSOR_INFO_S snsinfo;
/*定义 mipi 初始化参数信息 */
MIPI_ATTR_S mipi_attr;
/*定义 dev 初始化的属性信息 */
VIN_DEV_ATTR_S devinfo;
/*定义 pipe 属性信息 */
VIN_PIPE_ATTR_S pipeinfo;

#define HW_TIMER 24000
#define MAX_PLANE 4

extern int board_bus_num;
extern int entry_index;

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

int time_cost_ms(struct timeval *start, struct timeval *end)
{
	int time_ms = -1;
	time_ms = ((end->tv_sec * 1000 + end->tv_usec /1000) -
			(start->tv_sec * 1000 + start->tv_usec /1000));
	return time_ms;
}

static void big_to_little_end(char *srcBuf, char *dstBuf, unsigned int size)
{
	int i = 0;
	uint16_t *in = (uint16_t *)srcBuf;
	uint16_t *out = (uint16_t *)dstBuf;
	uint16_t tmp;
	for (i = 0; i < size/2; i++) {
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

	struct timeval time_now = { 0 };
	struct timeval time_next = { 0 };

	yuvFd = fopen(filename, "w+");

	if (yuvFd == NULL) {
		printf("ERRopen(%s) fail", filename);
		return -1;
	}

	temp = (char *)malloc(size);
	buffer = (char *)malloc(size);

	if (buffer == NULL) {
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


static void print_sensor_info(MIPI_SENSOR_INFO_S *snsinfo)
{
	printf("bus_num %d\n", snsinfo->sensorInfo.bus_num);
	printf("bus_type %d\n", snsinfo->sensorInfo.bus_type);
	printf("entry_index %d\n", snsinfo->sensorInfo.entry_index);
	printf("sensor_name %s\n", snsinfo->sensorInfo.sensor_name);
	printf("reg_width %d\n", snsinfo->sensorInfo.reg_width);
	printf("sensor_mode %d\n", snsinfo->sensorInfo.sensor_mode);
	printf("sensor_addr 0x%x\n", snsinfo->sensorInfo.sensor_addr);
	printf("serial_addr 0x%x\n", snsinfo->sensorInfo.serial_addr);
	printf("resolution %d\n", snsinfo->sensorInfo.resolution);

	return;
}

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

int sif_dump_func(void)
{
	struct timeval time_now = { 0 };
	struct timeval time_next = { 0 };
	int size = -1;
	char file_name[128] = { 0 };
	dump_info_t dump_info = {0};
	int ret = 0, i;
	hb_vio_buffer_t *sif_raw = NULL;
	int pipeId = 0;

	sif_raw = (hb_vio_buffer_t *) malloc(sizeof(hb_vio_buffer_t));
	memset(sif_raw, 0, sizeof(hb_vio_buffer_t));

	/* 从SIF模块获取图像帧 */
	ret = HB_VIN_GetDevFrame(pipeId, 0, sif_raw, 2000);
	if (ret < 0) {
		printf("HB_VIN_GetDevFrame error!!!\n");
	} else {
		x3_normal_buf_info_print(sif_raw);
		size = sif_raw->img_addr.stride_size * sif_raw->img_addr.height;
		printf("stride_size(%u) w x h%u x %u  size %d\n",
				sif_raw->img_addr.stride_size,
				sif_raw->img_addr.width, sif_raw->img_addr.height, size);

		if (sif_raw->img_info.planeCount == 1) {
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
		} else if (sif_raw->img_info.planeCount == 2) {
			/* 一般是 yuv 图，如果开启了dol2模式，则是两帧raw图， 根据img_format进行区分 */
			dump_info.ctx_id = pipeId;
			dump_info.raw.frame_id = sif_raw->img_info.frame_id;
			dump_info.raw.plane_count = sif_raw->img_info.planeCount;
			for (int i = 0; i < sif_raw->img_info.planeCount; i ++) {
				dump_info.raw.xres[i] = sif_raw->img_addr.width;
				dump_info.raw.yres[i] = sif_raw->img_addr.height;
				dump_info.raw.addr[i] = sif_raw->img_addr.addr[i];
				dump_info.raw.size[i] = size;
			}
			if(sif_raw->img_info.img_format == 0) {
				printf("pipe(%d)dump dol2 raw frame id(%d),plane(%d)size(%d)\n",
						dump_info.ctx_id, dump_info.raw.frame_id,
						dump_info.raw.plane_count, size);
			}
		} else if (sif_raw->img_info.planeCount == 3) {
			/* 一般是 dol3 模式下的3帧raw图 */
			dump_info.ctx_id = pipeId;
			dump_info.raw.frame_id = sif_raw->img_info.frame_id;
			dump_info.raw.plane_count = sif_raw->img_info.planeCount;
			for (int i = 0; i < sif_raw->img_info.planeCount; i ++) {
				dump_info.raw.xres[i] = sif_raw->img_addr.width;
				dump_info.raw.yres[i] = sif_raw->img_addr.height;
				dump_info.raw.addr[i] = sif_raw->img_addr.addr[i];
				dump_info.raw.size[i] = size;
			}
			printf("pipe(%d)dump dol3 raw frame id(%d),plane(%d)size(%d)\n",
					dump_info.ctx_id, dump_info.raw.frame_id,
					dump_info.raw.plane_count, size);
		} else {
			printf("pipe(%d)raw buf planeCount wrong !!!\n", pipeId);
		}

		for (int i = 0; i < dump_info.raw.plane_count; i ++) {
			if(sif_raw->img_info.img_format == 0) {
				sprintf(file_name, "pipe%d_plane%d_%ux%u_frame_%03d.raw",
						dump_info.ctx_id,
						i,
						dump_info.raw.xres[i],
						dump_info.raw.yres[i],
						dump_info.raw.frame_id);

				gettimeofday(&time_now, NULL);
				x3_dumpToFile(file_name,  dump_info.raw.addr[i], dump_info.raw.size[i]);
				gettimeofday(&time_next, NULL);
				int time_cost = time_cost_ms(&time_now, &time_next);
				printf("dumpToFile raw cost time %d ms\n", time_cost);
			}
		}
		if(sif_raw->img_info.img_format == 8) {
			sprintf(file_name, "pipe%d_%ux%u_frame_%03d.yuv",
					dump_info.ctx_id,
					dump_info.raw.xres[i],
					dump_info.raw.yres[i],
					dump_info.raw.frame_id);
			gettimeofday(&time_now, NULL);
			x3_dumpToFile2plane(file_name, sif_raw->img_addr.addr[0],
					sif_raw->img_addr.addr[1], size, size/2);
			gettimeofday(&time_next, NULL);
			int time_cost = time_cost_ms(&time_now, &time_next);
			printf("dumpToFile yuv cost time %d ms", time_cost);
		}
		ret = HB_VIN_ReleaseDevFrame(pipeId, 0, sif_raw);
		if (ret < 0) {
			printf("HB_VIN_ReleaseDevFrame error!!!\n");
		}
	}
	free(sif_raw);
	sif_raw = NULL;
}

int sensor_sif_dev_init(void)
{
	int ret = 0;
	int devId = 0;

	// 使能 mclk
	HB_MIPI_EnableSensorClock(0);
	HB_MIPI_SetSensorClock(0, 24000000);
	HB_MIPI_EnableSensorClock(1);
	HB_MIPI_SetSensorClock(1, 24000000);

	// 初始化sensor，配置sensor寄存器
	HB_MIPI_SensorBindSerdes(&snsinfo, snsinfo.sensorInfo.deserial_index, snsinfo.sensorInfo.deserial_port);
	HB_MIPI_SensorBindMipi(&snsinfo, snsinfo.sensorInfo.entry_index);
	
	snsinfo.sensorInfo.bus_num = board_bus_num;
	snsinfo.sensorInfo.entry_index = entry_index;

	print_sensor_info(&snsinfo);
	ret = HB_MIPI_InitSensor(devId, &snsinfo);
	if (ret < 0) {
		printf("hb mipi init sensor error!\n");
		return ret;
	}
	printf("hb sensor init success...\n");

	// 需要设置成offline模式，否则获取不到raw图
	ret = HB_SYS_SetVINVPSMode(0, VIN_OFFLINE_VPS_OFFINE);
	if (ret < 0) {
		printf("HB_SYS_SetVINVPSMode error!\n");
		return ret;
	}

	// 必须要初始化isp来贯通pipeline，即使isp不工作也需要，否则 HB_MIPI_SetMipiAttr 会失败
	ret = HB_VIN_CreatePipe(0, &pipeinfo);  // isp init
	if (ret < 0) {
		printf("HB_VIN_CreatePipe error!\n");
		return ret;
	}

	// 初始化mipi，设置mipi host的mipiclk、linelength、framelength、settle等参数
	ret = HB_MIPI_SetMipiAttr(snsinfo.sensorInfo.entry_index, &mipi_attr);
	if (ret < 0) {
		printf("hb mipi set mipi attr error!\n");
		return ret;
	}
	printf("hb mipi init success...\n");

	printf("devId: %d snsinfo.sensorInfo.entry_index:%d\n", devId, snsinfo.sensorInfo.entry_index);
	ret = HB_VIN_SetMipiBindDev(devId, snsinfo.sensorInfo.entry_index); /* mipi和vin(sif) dev 绑定 */
	if (ret < 0) {
		printf("HB_VIN_SetMipiBindDev error!\n");
		return ret;
	}
	printf("camera_info->mipi_attr.mipi_host_cfg.channel_num: %d\n", mipi_attr.mipi_host_cfg.channel_num);
	ret = HB_VIN_SetDevVCNumber(devId, mipi_attr.mipi_host_cfg.channel_sel[0]); /* 确定使用哪几个虚拟通道作为mipi的输入 */
	if (ret < 0) {
		printf("HB_VIN_SetDevVCNumber error!\n");
		return ret;
	}
	if(devinfo.mipiAttr.ipi_channels > 1 && mipi_attr.mipi_host_cfg.channel_num > 1)
	for(int i = 1 ; i < mipi_attr.mipi_host_cfg.channel_num; i++)
	{
		ret = HB_VIN_AddDevVCNumber(devId, mipi_attr.mipi_host_cfg.channel_sel[i]);
		if(ret < 0) {
			printf("HB_VIN_AddDevVCNumber error!\n");
			return ret;
		}
	}

	ret = HB_VIN_SetDevAttr(devId, &devinfo);  // sif init
	if (ret < 0) {
		printf("HB_VIN_SetDevAttr error!\n");
		return ret;
	}

	ret = HB_VIN_EnableDev(0); // sif start && start thread
	if (ret < 0)
	{
		printf("HB_VIN_EnableDev error!\n");
		return ret;
	}

	/* sensor出流，streamon*/
	ret = HB_MIPI_ResetSensor(devId);
	if (ret < 0)
	{
		printf("HB_MIPI_ResetSensor error!\n");
		return ret;
	}
	ret = HB_MIPI_ResetMipi(snsinfo.sensorInfo.entry_index);
	if (ret < 0)
	{
		printf("HB_MIPI_ResetMipi error, ret= %d\n", ret);
		return ret;
	}
	return ret;
}
