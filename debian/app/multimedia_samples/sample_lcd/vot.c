#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <asm/types.h>
#include <dlfcn.h>
#include <linux/fb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "hb_vot.h"
//#include "iar_interface.h"
#include "hb_vio_interface.h"

uint32_t get_file(const char *path, char **buff) {
  FILE *file = NULL;
  struct stat statbuf;

  file = fopen(path, "r");
  if (NULL == file) {
    printf("file %s open failed", path);
    return 0;
  }
  stat(path, &statbuf);
  if (0 == statbuf.st_size) {
    printf("read file size error");
    fclose(file);
    return 0;
  }
  *buff = (char *)malloc(statbuf.st_size);
  if (NULL == *buff) {
    printf("file buff malloc failed");
    fclose(file);
    return 0;
  }
  fread(*buff, statbuf.st_size, 1, file);
  fclose(file);
  return statbuf.st_size;
}

int main(void)
{
	char *framebuf[4];
	int framesize[4];
	VOT_FRAME_INFO_S stFrame = {};
	VOT_VIDEO_LAYER_ATTR_S stLayerAttr;
	VOT_CHN_ATTR_S stChnAttr;
	VOT_WB_ATTR_S stWbAttr;
	VOT_CROP_INFO_S cropAttrs;
	hb_vio_buffer_t iar_buf = {0};
	hb_vio_buffer_t iar_buf1 = {0};
	VOT_PUB_ATTR_S devAttr;
	POINT_S display_point = {0};
	int ret = 0;
	int i = 0;

	devAttr.enIntfSync = VO_OUTPUT_USER;
	devAttr.u32BgColor = 0x108080;
	devAttr.enOutputMode = HB_VOT_OUTPUT_MIPI;
	devAttr.stSyncInfo.pixel_clk = 64000000;
	devAttr.stSyncInfo.hbp = 40;
	devAttr.stSyncInfo.hfp = 40;
	devAttr.stSyncInfo.hs = 10;
	devAttr.stSyncInfo.vbp = 11;
	devAttr.stSyncInfo.vfp = 16;
	devAttr.stSyncInfo.vs = 3;
	devAttr.stSyncInfo.width = 720;
	devAttr.stSyncInfo.height = 1280;

	ret = HB_VOT_SetPubAttr(0, &devAttr);
	if (ret) {
		printf("HB_VOT_SetPubAttr failed\n");
		return -1;
	}
	ret = HB_VOT_Enable(0);
	if (ret)
		printf("HB_VOT_Enable failed.\n");
	memset(&stLayerAttr, 0, sizeof(stLayerAttr));
	stLayerAttr.stImageSize.u32Width  = 720;
	stLayerAttr.stImageSize.u32Height = 1280;

	stLayerAttr.panel_type = 0;
	stLayerAttr.rotate = 0;
	stLayerAttr.dithering_flag = 0;
	stLayerAttr.dithering_en = 0;
	stLayerAttr.gamma_en = 0;
	stLayerAttr.hue_en = 0;
	stLayerAttr.sat_en = 0;
	stLayerAttr.con_en = 0;
	stLayerAttr.bright_en = 0;
	stLayerAttr.theta_sign = 0;
	stLayerAttr.contrast = 0;
	stLayerAttr.theta_abs = 0;
	stLayerAttr.saturation = 0;
	stLayerAttr.off_contrast = 0;
	stLayerAttr.off_bright = 0;
	stLayerAttr.user_control_disp = 1;
	stLayerAttr.big_endian = 0;

	ret = HB_VOT_SetVideoLayerAttr(0, &stLayerAttr);
	if (ret)
		printf("HB_VOT_SetVideoLayerAttr failed.\n");

	ret = HB_VOT_EnableVideoLayer(0);
	if (ret)
		printf("HB_VOT_EnableVideoLayer failed.\n");

	stChnAttr.u32Priority = 2;
	stChnAttr.s32X = 0;
	stChnAttr.s32Y = 0;
	stChnAttr.u32SrcWidth = 720;
	stChnAttr.u32SrcHeight = 1280;
	stChnAttr.u32DstWidth = 720;
	stChnAttr.u32DstHeight = 1280;
	ret = HB_VOT_SetChnAttr(0, 0, &stChnAttr);
	printf("HB_VOT_SetChnAttr 0: %d\n", ret);

	cropAttrs.u32Width = stChnAttr.u32DstWidth;
	cropAttrs.u32Height = stChnAttr.u32DstHeight;
	ret = HB_VOT_SetChnCrop(0, 0, &cropAttrs);
	printf("HB_VOT_EnableChn: %d\n", ret);

	ret = HB_VOT_EnableChn(0, 0);
	printf("HB_VOT_EnableChn: %d\n", ret);

	framesize[0] = get_file("./720x1280.yuv", &framebuf[0]);
	if (framesize[0] == 0) {
		printf("read file 1920_1080yuv8.yuv failed\n");
		return -1;
	}
	printf("framesize:%d\n", framesize[0]);
	stFrame.addr = framebuf[0];
	stFrame.size = framesize[0];
	ret = HB_VOT_SendFrame(0, 0, &stFrame, -1);

	sleep(10000);
	ret = HB_VOT_DisableChn(0, 0);
	if (ret)
		printf("HB_VOT_DisableChn failed.\n");

	ret = HB_VOT_DisableVideoLayer(0);
	if (ret)
		printf("HB_VOT_DisableVideoLayer failed.\n");

	ret = HB_VOT_Disable(0);
	if (ret)
		printf("HB_VOT_Disable failed.\n");
	printf("================test end====================\n");
	return 0;
}