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

void sample_vot_sendframe(){
	char *framebuf[4];
	int framesize[4];
	VOT_FRAME_INFO_S stFrame = {};
	framesize[0] = get_file("./1920_1080yuv8.yuv", &framebuf[0]);
	if (framesize[0] == 0) {
		printf("read file 1920_1080yuv8.yuv failed\n");
		return;
	}
	printf("framesize:%d\n", framesize[0]);
	stFrame.addr = framebuf[0];
	stFrame.size = framesize[0];
	int ret = HB_VOT_SendFrame(0, 0, &stFrame, -1);
}

static int sample_vot_init(char* option)
{
	int ret = 0;
	VOT_PUB_ATTR_S stPubAttr = {};
	VOT_VIDEO_LAYER_ATTR_S stLayerAttr;
	VOT_CSC_S stCsc;
	VOT_UPSCALE_ATTR_S stUpScale;
	VOT_CHN_ATTR_S stChnAttr;
	VOT_CROP_INFO_S stCrop;
	POINT_S stPoint;
	VOT_CHN_ATTR_EX_S stChnAttrEx;
	ret = HB_VOT_GetPubAttr(0, &stPubAttr);
	if (ret) {
		printf("HB_VOT_GetPubAttr failed.\n");
		// break;
	}
	stPubAttr.enOutputMode = HB_VOT_OUTPUT_BT1120;
	stPubAttr.u32BgColor = 0xFF7F88;
	if(!strncmp(option, "1080P60",strlen("1080P60"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P60;
	}else if(!strncmp(option, "1080P59.94",strlen("1080P59.94"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P59_94;
	}else if(!strncmp(option, "1080P50",strlen("1080P50"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P50;
	}else if(!strncmp(option, "1080P30",strlen("1080P30"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P30;
	}else if(!strncmp(option, "1080P29.97",strlen("1080P29.97"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P29_97;
	}else if(!strncmp(option, "1080P25",strlen("1080P25"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P25;
	}else if(!strncmp(option, "1080I60",strlen("1080I60"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080I60;
	}else if(!strncmp(option, "1080I59.94",strlen("1080I59.94"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080I59_94;
	}else if(!strncmp(option, "1080I50",strlen("1080I50"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080I50;
	}else if(!strncmp(option, "720P60",strlen("720P60"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_720P60;
	}else if(!strncmp(option, "720P59.94",strlen("720P59.94"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_720P59_94;
	}else if(!strncmp(option, "720P50",strlen("720P50"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_720P50;
	}else if(!strncmp(option, "720P29.97",strlen("720P29.97"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_720P29_97;
	}

	ret = HB_VOT_SetPubAttr(0, &stPubAttr);
	if (ret) {
		printf("HB_VOT_SetPubAttr failed.\n");
		// break;
	}
	ret = HB_VOT_Enable(0);
	if (ret) {
		printf("HB_VOT_Enable failed.\n");
	}

	ret = HB_VOT_GetVideoLayerAttr(0, &stLayerAttr);
	if (ret) {
		printf("HB_VOT_GetVideoLayerAttr failed.\n");
	}
	printf("stLayer width:%d\n", stLayerAttr.stImageSize.u32Width);
	printf("stLayer height:%d\n", stLayerAttr.stImageSize.u32Height);
	stLayerAttr.stImageSize.u32Width = 1920;
	stLayerAttr.stImageSize.u32Height = 1080;
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
	stLayerAttr.user_control_disp = 1;/*如果开启HB_VOT_SendFrame就需要设置1*/
	stLayerAttr.user_control_disp_layer1 = 0;
	stLayerAttr.big_endian = 0;
	ret = HB_VOT_SetVideoLayerAttr(0, &stLayerAttr);
	if (ret) {
		printf("HB_VOT_SetVideoLayerAttr failed.\n");
	}

	ret = HB_VOT_EnableVideoLayer(0);
	if (ret) {
		printf("HB_VOT_EnableVideoLayer failed,ret: %d\n", ret);
	}
	ret = HB_VOT_GetChnAttr(0, 0, &stChnAttr);
	if (ret) {
		printf("HB_VOT_GetChnAttr failed.\n");
	}
	printf("stChnAttr priority :%d\n", stChnAttr.u32Priority);
	printf("stChnAttr src width :%d\n", stChnAttr.u32SrcWidth);
	printf("stChnAttr src height :%d\n", stChnAttr.u32SrcHeight);
	printf("stChnAttr s32X :%d\n", stChnAttr.s32X);
	printf("stChnAttr s32Y :%d\n", stChnAttr.s32Y);
	printf("stChnAttr u32DstWidth :%d\n", stChnAttr.u32DstWidth);
	printf("stChnAttr u32DstHeight :%d\n", stChnAttr.u32DstHeight);
	stChnAttr.u32Priority = 2;
	stChnAttr.u32SrcWidth = 1920;
	stChnAttr.u32SrcHeight = 1080;
	stChnAttr.s32X = 0;
	stChnAttr.s32Y = 0;
	stChnAttr.u32DstWidth = 1920;
	stChnAttr.u32DstHeight = 1080;
	ret = HB_VOT_SetChnAttr(0, 0, &stChnAttr);
	if (ret) {
		printf("HB_VOT_SetChnAttr failed.\n");
		// break;
	}

	ret = HB_VOT_GetChnCrop(0, 0, &stCrop);
	if (ret) {
		printf("HB_VOT_GetChnCrop failed.\n");
	}
	printf("stCrop width :%d\n", stCrop.u32Width);
	printf("stCrop height :%d\n", stCrop.u32Height);
	stCrop.u32Width = 1920;
	stCrop.u32Height = 1080;
	ret = HB_VOT_SetChnCrop(0, 0, &stCrop);
	if (ret) {
		printf("HB_VOT_SetChnCrop failed.\n");
	}

	ret = HB_VOT_EnableChn(0, 0);
	if (ret) {
		printf("HB_VOT_EnableChn failed.\n");
	}
#if 0
	ret = HB_VOT_EnableChn(0, 0);
	if (ret) {
		printf("HB_VOT_EnableChn failed.\n");
	}
#endif
	sample_vot_sendframe();
}

static int sample_vot_init_720p(char* option)
{
	int ret = 0;
	char *framebuf[4];
	int framesize[4];
	VOT_FRAME_INFO_S stFrame = {};
	VOT_PUB_ATTR_S stPubAttr = {};
	VOT_VIDEO_LAYER_ATTR_S stLayerAttr;
	VOT_CSC_S stCsc;
	VOT_UPSCALE_ATTR_S stUpScale;
	VOT_CHN_ATTR_S stChnAttr;
	VOT_CROP_INFO_S stCrop;
	POINT_S stPoint;
	VOT_CHN_ATTR_EX_S stChnAttrEx;
	ret = HB_VOT_GetPubAttr(0, &stPubAttr);
	if (ret) {
		printf("HB_VOT_GetPubAttr failed.\n");
		// break;
	}
	stPubAttr.enOutputMode = HB_VOT_OUTPUT_BT1120;
	stPubAttr.u32BgColor = 0xFF7F88;
	if(!strncmp(option, "1080P60",strlen("1080P60"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P60;
	}else if(!strncmp(option, "1080P59.94",strlen("1080P59.94"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P59_94;
	}else if(!strncmp(option, "1080P50",strlen("1080P50"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P50;
	}else if(!strncmp(option, "1080P30",strlen("1080P30"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P30;
	}else if(!strncmp(option, "1080P29.97",strlen("1080P29.97"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P29_97;
	}else if(!strncmp(option, "1080P25",strlen("1080P25"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080P25;
	}else if(!strncmp(option, "1080I60",strlen("1080I60"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080I60;
	}else if(!strncmp(option, "1080I59.94",strlen("1080I59.94"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080I59_94;
	}else if(!strncmp(option, "1080I50",strlen("1080I50"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_1080I50;
	}else if(!strncmp(option, "720P60",strlen("720P60"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_720P60;
	}else if(!strncmp(option, "720P59.94",strlen("720P59.94"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_720P59_94;
	}else if(!strncmp(option, "720P50",strlen("720P50"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_720P50;
	}else if(!strncmp(option, "720P29.97",strlen("720P29.97"))){
		stPubAttr.enIntfSync = VOT_OUTPUT_720P29_97;
	}

	ret = HB_VOT_SetPubAttr(0, &stPubAttr);
	if (ret) {
		printf("HB_VOT_SetPubAttr failed.\n");
		// break;
	}
	ret = HB_VOT_Enable(0);
	if (ret) {
		printf("HB_VOT_Enable failed.\n");
	}

	ret = HB_VOT_GetVideoLayerAttr(0, &stLayerAttr);
	if (ret) {
		printf("HB_VOT_GetVideoLayerAttr failed.\n");
	}
	printf("stLayer width:%d\n", stLayerAttr.stImageSize.u32Width);
	printf("stLayer height:%d\n", stLayerAttr.stImageSize.u32Height);
	stLayerAttr.stImageSize.u32Width = 1280;
	stLayerAttr.stImageSize.u32Height = 720;
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
	stLayerAttr.user_control_disp = 1;/*如果开启HB_VOT_SendFrame就需要设置1*/
	stLayerAttr.user_control_disp_layer1 = 0;
	stLayerAttr.big_endian = 0;
	ret = HB_VOT_SetVideoLayerAttr(0, &stLayerAttr);
	if (ret) {
		printf("HB_VOT_SetVideoLayerAttr failed.\n");
	}

	ret = HB_VOT_EnableVideoLayer(0);
	if (ret) {
		printf("HB_VOT_EnableVideoLayer failed,ret: %d\n", ret);
	}
	ret = HB_VOT_GetChnAttr(0, 0, &stChnAttr);
	if (ret) {
		printf("HB_VOT_GetChnAttr failed.\n");
	}
	printf("stChnAttr priority :%d\n", stChnAttr.u32Priority);
	printf("stChnAttr src width :%d\n", stChnAttr.u32SrcWidth);
	printf("stChnAttr src height :%d\n", stChnAttr.u32SrcHeight);
	printf("stChnAttr s32X :%d\n", stChnAttr.s32X);
	printf("stChnAttr s32Y :%d\n", stChnAttr.s32Y);
	printf("stChnAttr u32DstWidth :%d\n", stChnAttr.u32DstWidth);
	printf("stChnAttr u32DstHeight :%d\n", stChnAttr.u32DstHeight);
	stChnAttr.u32Priority = 2;
	stChnAttr.u32SrcWidth = 1280;
	stChnAttr.u32SrcHeight = 720;
	stChnAttr.s32X = 0;
	stChnAttr.s32Y = 0;
	stChnAttr.u32DstWidth = 1280;
	stChnAttr.u32DstHeight = 720;
	ret = HB_VOT_SetChnAttr(0, 0, &stChnAttr);
	if (ret) {
		printf("HB_VOT_SetChnAttr failed.\n");
		// break;
	}

	ret = HB_VOT_GetChnCrop(0, 0, &stCrop);
	if (ret) {
		printf("HB_VOT_GetChnCrop failed.\n");
	}
	printf("stCrop width :%d\n", stCrop.u32Width);
	printf("stCrop height :%d\n", stCrop.u32Height);
	stCrop.u32Width = 1280;
	stCrop.u32Height = 720;
	ret = HB_VOT_SetChnCrop(0, 0, &stCrop);
	if (ret) {
		printf("HB_VOT_SetChnCrop failed.\n");
	}

	ret = HB_VOT_EnableChn(0, 0);
	if (ret) {
		printf("HB_VOT_EnableChn failed.\n");
	}
#if 0
	ret = HB_VOT_EnableChn(0, 0);
	if (ret) {
		printf("HB_VOT_EnableChn failed.\n");
	}
#endif
	framesize[0] = get_file("./1280_720yuv8.yuv", &framebuf[0]);
	if (framesize[0] == 0) {
		printf("read file 1280_720yuv8.yuv failed\n");
		return -1;
	}
	printf("framesize:%d\n", framesize[0]);
	stFrame.addr = framebuf[0];
	stFrame.size = framesize[0];
	ret = HB_VOT_SendFrame(0, 0, &stFrame, -1);
}


static int sample_vot_deinit()
{
	int ret = 0;
	ret = HB_VOT_DisableChn(0, 0);
	if (ret)
	{
		printf("HB_VOT_DisableChn failed.\n");
	}
	ret = HB_VOT_DisableVideoLayer(0);
	if (ret)
	{
		printf("HB_VOT_DisableVideoLayer failed.\n");
	}
	ret = HB_VOT_Disable(0);
	if (ret)
	{
		printf("HB_VOT_Disable failed.\n");
	}
	return 0;
}

int main(int argc, char **argv)
{
	int main_loop = 1;
	char option[32] = "1080P60";
	if(argc > 1) {
		memset(option, 0, sizeof(option));
		strcpy(option, argv[1]);
	}
	/*初始化vot输出，根据bin文件后跟的参数来初始化，目前vot支持输出的参数支持如下：
	  1080P60
	  1080P59.94
	  1080P50
	  1080P30
	  1080P29.97
	  1080P25
	  1080I60
	  1080I59.94
	  1080I50
	  720P60
	  720P59.94
	  720P50
	  720P29.97
	  注意：使用sdb的板子用的bt1120转换hdmi芯片是lt8618，只支持1080p30输出，客户可以使用
	  sil902x的bt1120转hdmi芯片可以支持如上列的所有格式
	  */
	if(!strncmp(option, "1080P60",strlen("1080P60"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "1080P59.94",strlen("1080P59.94"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "1080P50",strlen("1080P50"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "1080P30",strlen("1080P30"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "1080P29.97",strlen("1080P29.97"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "1080P25",strlen("1080P25"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "1080I60",strlen("1080I60"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "1080I59.94",strlen("1080I59.94"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "1080I50",strlen("1080I50"))){
		sample_vot_init(option);
	}else if(!strncmp(option, "720P60",strlen("720P60"))){
		sample_vot_init_720p(option);
	}else if(!strncmp(option, "720P59.94",strlen("720P59.94"))){
		sample_vot_init_720p(option);
	}else if(!strncmp(option, "720P50",strlen("720P50"))){
		sample_vot_init_720p(option);
	}else if(!strncmp(option, "720P29.97",strlen("720P29.97"))){
		sample_vot_init_720p(option);
	}
	while(main_loop)
	{
		char cc = getchar();
		printf("main getchar = %d\n", cc);
		if(cc == 'x' || cc == 'X')
		{
			main_loop = 0;
			break;
		}
	}
	sample_vot_deinit();

	return 0;
}
