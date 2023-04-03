/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2019 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef __SAMPLE_DAHUA_H
#define __SAMPLE_DAHUA_H

#include "libmm/hb_media_codec.h"
#include "libmm/hb_media_error.h"
#include "vio/hb_type.h"
#include <hbmem.h>
#include <stdatomic.h>
#define MAX_SENSOR_NUM 6
#define MAX_MIPIID_NUM 4
#define MAX_ID_NUM 4
#define BIT(n)  (1UL << (n))
#define TAG "[COMMON]"
extern char *g_inputfilename;
extern atomic_int running;
typedef struct {
	int loop;
	int vdecchn;
	char fname[128];
} vdecParam;

typedef struct {
    int veChn;
    int type;
    int width;
    int height;
    int bitrate;
    int vpsGrp;
    int vpsChn;
	int quit;
}vencParam;

struct arguments
{
	int mode;
	char *input1;
	char *input2;
	char *output1;
    char *output2;
	char *prefix1;
    char *prefix2;
	int height1;
    int height2;
	int width1;
	int width2;
	int type;
	int rc_mode;
	int ext_buf;
	int dual;
};

// typedef struct venc_thread1_param1_t{
//     char *input;
//     char *output;
// 	int height;
// 	int width;
// 	int type;
// 	int rc_mode;
// 	int ext_buf;
// }venc_param;

typedef struct venc_thread_param_t{
    char *input;
    char *output;
	int height;
	int width;
	int type;
	int rc_mode;
	int ext_buf;
}venc_param;

typedef struct vdec_thread_param_t{
    char *input;
    char *prefix;
	int height;
	int width;
	int type;
    media_codec_context_t* context;
}vdec_param;


typedef struct ion_buffer_t {
    int32_t size;
    unsigned long long phys_addr;
    unsigned long   virt_addr;
} ion_buffer_t;

typedef struct {
    int rgnChn;
    int x;
    int y;
    int w;
    int h;
}rgnParam;

typedef struct MediaCodecTestContext
{
    media_codec_context_t *context;
    char *inputFileName;
    char *outputFileName;
    int vlc_buf_size;
} MediaCodecTestContext;

typedef struct AsyncMediaCtx
{
    media_codec_context_t *ctx;
    FILE *inFile;
    FILE *outFile;
    int lastStream;
    uint64_t startTime;
    int32_t duration;
} AsyncMediaCtx;

void do_async_encoding(void *arg);

void venc_thread(void *vencpram);
int allocate_ion_mem(int size, ion_buffer_t *ionbuf);
int release_ion_mem(ion_buffer_t *ionbuf);
void *sample_venc(void *arg);
void *sample_vdec(void *arg);

#endif
