/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2022 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>

#include "hb_vp_api.h"
#include "hb_vin_api.h"
#include "hb_vps_api.h"
#include "hb_mipi_api.h"
#include "hb_vio_interface.h"
#include "hb_sys.h"
#include "hb_common.h"
#include "hb_type.h"
#include "hb_errno.h"
#include "hb_comm_video.h"
#include "hb_venc.h"

int g_dump = 0;

int ion_open(void);
int ion_alloc_phy(int size, int *fd, char **vaddr, uint64_t * paddr);
int dumpToFile2plane(char *filename, char *srcBuf, char *srcBuf1, unsigned int size, unsigned int size1);

int prepare_user_buf(void *buf, uint32_t size_y, uint32_t size_uv)
{
	int ret;
	hb_vio_buffer_t *buffer = (hb_vio_buffer_t *)buf;

	if (buffer == NULL)
		return -1;

	buffer->img_info.fd[0] = ion_open();
	buffer->img_info.fd[1] = ion_open();

	ret = ion_alloc_phy(size_y, &buffer->img_info.fd[0], &buffer->img_addr.addr[0], &buffer->img_addr.paddr[0]);
	if (ret) {
		printf("prepare user buf error\n");
		return ret;
	}

	ret = ion_alloc_phy(size_uv, &buffer->img_info.fd[1], &buffer->img_addr.addr[1], &buffer->img_addr.paddr[1]);
	if (ret) {
		printf("prepare user buf error\n");
		return ret;
	}

	printf("buf:y: vaddr = 0x%s paddr = 0x%ld; uv: vaddr = 0x%s, paddr = 0x%ld\n",
			buffer->img_addr.addr[0], buffer->img_addr.paddr[0],
			buffer->img_addr.addr[1], buffer->img_addr.paddr[1]);

	return 0;
}

// 1920x1080 grp 0 chn 1 裁剪 1280x720
// 1920x1080 grp 0 chn 2 旋转 1088x1920
// 1920x1080 grp 0 chn 3 缩小 960x540
// 1920x1080 grp 0 chn 5 放大 2880x1620
int main()
{
	int ret;
	char file_name[64];
	uint32_t size_y, size_uv;
	VPS_GRP_ATTR_S grp_attr;
	VPS_CHN_ATTR_S chn_1_attr;
	VPS_CHN_ATTR_S chn_2_attr;
	VPS_CHN_ATTR_S chn_3_attr;
	VPS_CHN_ATTR_S chn_5_attr;
	VPS_CROP_INFO_S crop_info;
	int img_in_fd;
	int chn_1_img_out_fd;
	int chn_2_img_out_fd;
	int chn_3_img_out_fd;
	int chn_5_img_out_fd;
	hb_vio_buffer_t feedback_buf;
	hb_vio_buffer_t chn_1_out_buf;
	hb_vio_buffer_t chn_2_out_buf;
	hb_vio_buffer_t chn_3_out_buf;
	hb_vio_buffer_t chn_5_out_buf;

	memset(&grp_attr, 0, sizeof(VPS_GRP_ATTR_S));
	grp_attr.maxW = 1920;
	grp_attr.maxH = 1080;
	grp_attr.frameDepth = 8;
	HB_VPS_CreateGrp(0, &grp_attr);
	HB_SYS_SetVINVPSMode(0, VIN_OFFLINE_VPS_OFFINE);

	memset(&chn_1_attr, 0, sizeof(VPS_CHN_ATTR_S));
	chn_1_attr.enScale = 1;
	chn_1_attr.width = 1280;
	chn_1_attr.height = 720;
	chn_1_attr.frameDepth = 8;
	HB_VPS_SetChnAttr(0, 1, &chn_1_attr);

	HB_VPS_GetChnCrop(0, 1, &crop_info);
	crop_info.en = 1;
	crop_info.cropRect.x = 0;
	crop_info.cropRect.y = 0;
	crop_info.cropRect.width = 1280;
	crop_info.cropRect.height = 720;
	HB_VPS_SetChnCrop(0, 1, &crop_info);

	memset(&chn_2_attr, 0, sizeof(VPS_CHN_ATTR_S));
	chn_2_attr.enScale = 1;
	chn_2_attr.width = 1920;
	chn_2_attr.height = 1080;
	chn_2_attr.frameDepth = 8;
	HB_VPS_SetChnAttr(0, 2, &chn_2_attr);
	HB_VPS_SetChnRotate(0, 2, ROTATION_90);

	memset(&chn_3_attr, 0, sizeof(VPS_CHN_ATTR_S));
	chn_3_attr.enScale = 1;
	chn_3_attr.width = 960;
	chn_3_attr.height = 540;
	chn_3_attr.frameDepth = 8;
	HB_VPS_SetChnAttr(0, 3, &chn_3_attr);

	memset(&chn_5_attr, 0, sizeof(VPS_CHN_ATTR_S));
	chn_5_attr.enScale = 1;
	chn_5_attr.width = 2880;
	chn_5_attr.height = 1620;
	chn_5_attr.frameDepth = 8;
	HB_VPS_SetChnAttr(0, 5, &chn_5_attr);

	HB_VPS_EnableChn(0, 1);
	HB_VPS_EnableChn(0, 2);
	HB_VPS_EnableChn(0, 3);
	HB_VPS_EnableChn(0, 5);
	HB_VPS_StartGrp(0);

	memset(&feedback_buf, 0, sizeof(hb_vio_buffer_t));
	size_y = 1920 * 1080;
	size_uv = size_y / 2;
	prepare_user_buf(&feedback_buf, size_y, size_uv);
	feedback_buf.img_info.planeCount = 2;
	feedback_buf.img_info.img_format = 8;
	feedback_buf.img_addr.width = 1920;
	feedback_buf.img_addr.height = 1080;
	feedback_buf.img_addr.stride_size = 1920;

	img_in_fd = open("./19201080.yuv", O_RDWR | O_CREAT);
	if (img_in_fd < 0) {
		printf("open image failed !\n");
		return img_in_fd;
	}

	read(img_in_fd, feedback_buf.img_addr.addr[0], size_y);
	usleep(10 * 1000);
	read(img_in_fd, feedback_buf.img_addr.addr[1], size_uv);
	usleep(10 * 1000);
	close(img_in_fd);
	if (g_dump) {
		dumpToFile2plane("in_image.yuv", feedback_buf.img_addr.addr[0], feedback_buf.img_addr.addr[1], size_y, size_uv);
	}
	usleep(100 * 1000);

	HB_VPS_SendFrame(0, &feedback_buf, 1000);

	HB_VPS_GetChnFrame(0, 1, &chn_1_out_buf, 2000);
	size_y = chn_1_out_buf.img_addr.stride_size * chn_1_out_buf.img_addr.height;
	size_uv = size_y / 2;
	printf("out:stride size = %d height = %d\n", chn_1_out_buf.img_addr.stride_size, chn_1_out_buf.img_addr.height);
	snprintf(file_name, sizeof(file_name), "grp_%d_chn_%d_out_%d_%d.yuv", 0, 1, chn_1_out_buf.img_addr.width, chn_1_out_buf.img_addr.height);
	dumpToFile2plane(file_name,
			chn_1_out_buf.img_addr.addr[0],
			chn_1_out_buf.img_addr.addr[1],
			chn_1_out_buf.img_addr.width * chn_1_out_buf.img_addr.height,
			chn_1_out_buf.img_addr.width * chn_1_out_buf.img_addr.height / 2);

	HB_VPS_GetChnFrame(0, 2, &chn_2_out_buf, 2000);
	size_y = chn_2_out_buf.img_addr.stride_size * chn_2_out_buf.img_addr.height;
	size_uv = size_y / 2;
	printf("out:stride size = %d height = %d\n", chn_2_out_buf.img_addr.stride_size, chn_2_out_buf.img_addr.height);
	snprintf(file_name, sizeof(file_name), "grp_%d_chn_%d_out_%d_%d.yuv", 0, 2, chn_2_out_buf.img_addr.width, chn_2_out_buf.img_addr.height);
	dumpToFile2plane(file_name,
			chn_2_out_buf.img_addr.addr[0],
			chn_2_out_buf.img_addr.addr[1],
			chn_2_out_buf.img_addr.width * chn_2_out_buf.img_addr.height,
			chn_2_out_buf.img_addr.width * chn_2_out_buf.img_addr.height / 2);

	HB_VPS_GetChnFrame(0, 3, &chn_3_out_buf, 2000);
	size_y = chn_3_out_buf.img_addr.stride_size * chn_3_out_buf.img_addr.height;
	size_uv = size_y / 2;
	printf("out:stride size = %d height = %d\n", chn_3_out_buf.img_addr.stride_size, chn_3_out_buf.img_addr.height);
	snprintf(file_name, sizeof(file_name), "grp_%d_chn_%d_out_%d_%d.yuv", 0, 3, chn_3_out_buf.img_addr.width, chn_3_out_buf.img_addr.height);
	dumpToFile2plane(file_name,
			chn_3_out_buf.img_addr.addr[0],
			chn_3_out_buf.img_addr.addr[1],
			chn_3_out_buf.img_addr.width * chn_3_out_buf.img_addr.height,
			chn_3_out_buf.img_addr.width * chn_3_out_buf.img_addr.height / 2);

	HB_VPS_GetChnFrame(0, 5, &chn_5_out_buf, 2000);
	size_y = chn_5_out_buf.img_addr.stride_size * chn_5_out_buf.img_addr.height;
	size_uv = size_y / 2;
	printf("out:stride size = %d height = %d\n", chn_5_out_buf.img_addr.stride_size, chn_5_out_buf.img_addr.height);
	snprintf(file_name, sizeof(file_name), "grp_%d_chn_%d_out_%d_%d.yuv", 0, 5, chn_5_out_buf.img_addr.width, chn_5_out_buf.img_addr.height);
	dumpToFile2plane(file_name,
			chn_5_out_buf.img_addr.addr[0],
			chn_5_out_buf.img_addr.addr[1],
			chn_5_out_buf.img_addr.width * chn_5_out_buf.img_addr.height,
			chn_5_out_buf.img_addr.width * chn_5_out_buf.img_addr.height / 2);

	HB_VPS_ReleaseChnFrame(0, 1, &chn_1_out_buf);
	HB_VPS_ReleaseChnFrame(0, 2, &chn_2_out_buf);
	HB_VPS_ReleaseChnFrame(0, 3, &chn_3_out_buf);
	HB_VPS_ReleaseChnFrame(0, 5, &chn_5_out_buf);

	HB_VPS_DisableChn(0, 1);
	HB_VPS_DisableChn(0, 2);
	HB_VPS_DisableChn(0, 3);
	HB_VPS_DisableChn(0, 5);
	HB_VPS_StopGrp(0);
	HB_VPS_DestroyGrp(0);

	return 0;
}
