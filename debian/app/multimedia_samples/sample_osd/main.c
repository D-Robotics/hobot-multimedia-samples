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
#include "hb_rgn.h"

typedef struct {
    int rgnChn;
    int x;
    int y;
    int w;
    int h;
} RGN_PARAM_S;

int g_dump = 0;
int g_run = 1;
int g_osd_en = 1;
RGN_PARAM_S rgn_param[6]={0};
pthread_t g_rgn_thid[6];

int ion_open(void);
int ion_alloc_phy(int size, int *fd, char **vaddr, uint64_t * paddr);
int dumpToFile2plane(char *filename, char *srcBuf, char *srcBuf1, unsigned int size, unsigned int size1);
int dumpToFile2planeStride(char *filename, char *srcBuf, char *srcBuf1, unsigned int size, unsigned int size1, int width, int height, int stride);

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

void* sample_rgn_thread(void *rgnparams)
{
    int ret = 0;
    RGN_CHN_S chn;
    RGN_ATTR_S pstRegion;
    RGN_CANVAS_S pstCanvasInfo;
    RGN_BITMAP_S bitmapAttr;
    RGN_DRAW_WORD_S drawWord;
    RGN_DRAW_LINE_S drawLine;
    int flag;
    RGN_CHN_ATTR_S chn_attr;
    int w, h, x, y, rgnChnTime, rgnChnPlace;
    time_t tt,tt_last=0;
    RGN_PARAM_S *rgnparam = (RGN_PARAM_S*)rgnparams;

    rgnChnTime = rgnparam->rgnChn;
    rgnChnPlace = rgnparam->rgnChn + 6;
    x = rgnparam->x;
    y = rgnparam->y;
    w = rgnparam->w;
    h = rgnparam->h;

    chn.s32PipelineId = 0;
    chn.enChnId = rgnparam->rgnChn;

    pstRegion.enType = OVERLAY_RGN;
    pstRegion.stOverlayAttr.stSize.u32Width = w;
    pstRegion.stOverlayAttr.stSize.u32Height = h;
    pstRegion.stOverlayAttr.enPixelFmt = PIXEL_FORMAT_VGA_4;
    pstRegion.stOverlayAttr.enBgColor  = 16;

    chn_attr.bShow = true;
    chn_attr.bInvertEn = false;
    // chn_attr.enType = OVERLAY_RGN;
    chn_attr.unChnAttr.stOverlayChn.stPoint.u32X = x;
    chn_attr.unChnAttr.stOverlayChn.stPoint.u32Y = y;
    // chn_attr.unChnAttr.stOverlayChn.u32Layer = 3;

    bitmapAttr.enPixelFormat = PIXEL_FORMAT_VGA_4;
    bitmapAttr.stSize = pstRegion.stOverlayAttr.stSize;
    bitmapAttr.pAddr = malloc(w * h / 2);
    memset(bitmapAttr.pAddr, 0xff, w * h / 2);

    ret = HB_RGN_Create(rgnChnTime, &pstRegion);
    if (ret) {
        printf("HB_RGN_Create failed\n");
        return NULL;
    }
    ret = HB_RGN_Create(rgnChnPlace, &pstRegion);
    if (ret) {
        printf("HB_RGN_Create failed\n");
        return NULL;
    }

    ret = HB_RGN_AttachToChn(rgnChnTime, &chn, &chn_attr);
    if (ret) {
        printf("HB_RGN_AttachToChn failed\n");
        return NULL;
    }
    chn_attr.unChnAttr.stOverlayChn.stPoint.u32Y += 100;
    ret = HB_RGN_AttachToChn(rgnChnPlace, &chn, &chn_attr);
    if (ret) {
        printf("HB_RGN_AttachToChn failed\n");
        return NULL;
    }

    // drawWord.bInvertEn = 1;
    drawWord.enFontSize = FONT_SIZE_MEDIUM;
    drawWord.enFontColor = FONT_COLOR_WHITE;
    drawWord.stPoint.u32X = 0;
    drawWord.stPoint.u32Y = 0;
    drawWord.bFlushEn = false;
    // unsigned char str[10] = {0xce, 0xd2, 0xce, 0xd2, 0xce, 0xd2, 0xce, 0xd2, 0xce, 0xd2};

    char str[32] = {0xb5, 0xd8, 0xb5, 0xe3, 0xca, 0xbe, 0xc0, 0xfd};
    drawWord.pu8Str = str;
    drawWord.pAddr = bitmapAttr.pAddr;
    drawWord.stSize = bitmapAttr.stSize;

    ret = HB_RGN_DrawWord(rgnChnPlace, &drawWord);
    if (ret) {
        printf("HB_RGN_DrawWord failed\n");
        return NULL;
    }

    ret = HB_RGN_SetBitMap(rgnChnPlace, &bitmapAttr);
    if (ret) {
        printf("HB_RGN_SetBitMap failed\n");
        return NULL;
    }

	while(g_run) {
        tt = time(0);
        if (tt>tt_last) {
            char str[32];
            strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", localtime(&tt));
            drawWord.pu8Str = str;
            // drawWord.u32LumThresh = 120;
            drawWord.pAddr = bitmapAttr.pAddr;
            drawWord.stSize = bitmapAttr.stSize;
            // printf("================ osd: %s\n", str);
            ret = HB_RGN_DrawWord(rgnChnTime, &drawWord);
            if (ret) {
                printf("HB_RGN_DrawWord failed\n");
                return NULL;
            }
            ret = HB_RGN_SetBitMap(rgnChnTime, &bitmapAttr);
            if (ret) {
                printf("HB_RGN_SetBitMap failed\n");
                return NULL;
            }
            tt_last = tt;
        }
        usleep(100000);
    }
	
    ret = HB_RGN_DetachFromChn(rgnChnPlace, &chn);
    if (ret) {
        printf("HB_RGN_DetachFromChn failed\n");
        return NULL;
    }
    ret = HB_RGN_DetachFromChn(rgnChnTime, &chn);
    if (ret) {
        printf("HB_RGN_DetachFromChn failed\n");
        return NULL;
    }

	ret = HB_RGN_Destroy(rgnChnTime);
    if (ret) {
        printf("HB_RGN_Destroy failed\n");
        return NULL;
    }
    ret = HB_RGN_Destroy(rgnChnPlace);
    if (ret) {
        printf("HB_RGN_Destroy failed\n");
        return NULL;
    }

    free(bitmapAttr.pAddr);
    printf("========== quit osd thread %d\n", chn.enChnId);
    return NULL;
}

void rgn_init_thread(RGN_CHN_S chn, int mode)
{
	rgn_param[chn.enChnId].rgnChn = chn.enChnId;
	rgn_param[chn.enChnId].x = 50;
	rgn_param[chn.enChnId].y = 50;
	rgn_param[chn.enChnId].w = 640;
	rgn_param[chn.enChnId].h = 100;
	pthread_create(&g_rgn_thid[chn.enChnId], NULL, (void *)sample_rgn_thread, &(rgn_param[chn.enChnId]));
}

void rgn_exit_thread(RGN_CHN_S chn, int mode)
{
	pthread_join(g_rgn_thid[chn.enChnId], NULL);
}

int main()
{
	int ret;
	char file_name[64];
	uint32_t size_y, size_uv;
	VPS_GRP_ATTR_S grp_attr;
  	VPS_CHN_ATTR_S chn_2_attr;
	int vps_chn_index = 2;
	int img_in_fd;
	hb_vio_buffer_t feedback_buf;
	hb_vio_buffer_t chn_2_out_buf;
	RGN_CHN_S rgn_chn;
	
	memset(&grp_attr, 0, sizeof(VPS_GRP_ATTR_S));
	grp_attr.maxW = 1280;
	grp_attr.maxH = 720;
	grp_attr.frameDepth = 8;
	HB_VPS_CreateGrp(0, &grp_attr);
	HB_SYS_SetVINVPSMode(0, VIN_OFFLINE_VPS_OFFINE);
	
	memset(&chn_2_attr, 0, sizeof(VPS_CHN_ATTR_S));
    chn_2_attr.enScale = 1;
    chn_2_attr.width = 1280;
    chn_2_attr.height = 720;
    chn_2_attr.frameDepth = 8;
    HB_VPS_SetChnAttr(0, vps_chn_index, &chn_2_attr);
	
	HB_VPS_EnableChn(0, vps_chn_index);
    HB_VPS_StartGrp(0);
	
	memset(&feedback_buf, 0, sizeof(hb_vio_buffer_t));
    size_y = 1280 * 720;
    size_uv = size_y / 2;
    prepare_user_buf(&feedback_buf, size_y, size_uv);
    feedback_buf.img_info.planeCount = 2;
    feedback_buf.img_info.img_format = 8;
    feedback_buf.img_addr.width = 1280;
    feedback_buf.img_addr.height = 720;
    feedback_buf.img_addr.stride_size = 1280;
	
	img_in_fd = open("./1280720.yuv", O_RDWR | O_CREAT);
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
	
	if (g_osd_en) {
		rgn_chn.s32PipelineId = 0;
		rgn_chn.enChnId = CHN_DS2;
		rgn_init_thread(rgn_chn, 0);
	}
	
	for (int i = 0; i < 10; i++) {
		HB_VPS_SendFrame(0, &feedback_buf, 1000);
		HB_VPS_GetChnFrame(0, vps_chn_index, &chn_2_out_buf, 2000);
		size_y = chn_2_out_buf.img_addr.stride_size * chn_2_out_buf.img_addr.height;
		size_uv = size_y / 2;
		printf("out:stride size = %d height = %d\n", chn_2_out_buf.img_addr.stride_size, chn_2_out_buf.img_addr.height);
		snprintf(file_name, sizeof(file_name), "grp_%d_chn_%d_out_%d_%d_%d.yuv", 0, 1, chn_2_out_buf.img_addr.width, chn_2_out_buf.img_addr.height, i);
		dumpToFile2plane(file_name, 
						chn_2_out_buf.img_addr.addr[0], 
						chn_2_out_buf.img_addr.addr[1], 
						chn_2_out_buf.img_addr.width * chn_2_out_buf.img_addr.height, 
						chn_2_out_buf.img_addr.width * chn_2_out_buf.img_addr.height / 2);
		
		HB_VPS_ReleaseChnFrame(0, vps_chn_index, &chn_2_out_buf);
	}

	if (g_osd_en) {
		g_run = 0;
		rgn_chn.s32PipelineId = 0;
		rgn_chn.enChnId = CHN_DS2;
		rgn_exit_thread(rgn_chn, 0);
	}

	HB_VPS_DisableChn(0, vps_chn_index);
	HB_VPS_StopGrp(0);
	HB_VPS_DestroyGrp(0);
	
	return 0;
}