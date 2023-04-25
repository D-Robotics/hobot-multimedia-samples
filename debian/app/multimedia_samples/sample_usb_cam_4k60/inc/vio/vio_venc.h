/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_VIO_VENC_H_
#define INCLUDE_VIO_VENC_H_
#include "hb_comm_venc.h"
#include "hb_venc.h"

typedef struct {
    int veChn;
    PAYLOAD_TYPE_E type;
    int width;
    int height;
    int stride;
    int bitrate;
    int vpsGrp;
    int vpsChn;
		int quit;
		int mjpgQp;
    int bind_flag;
    int need_vo;
    int need_dump;
    int need_rtsp;
} vencParam_t;

int hb_venc_init(vencParam_t *vencparam);
int hb_venc_start(int channel);
int hb_venc_stop(int channel);
int hb_vps_unbind_venc(int vpsGrp, int vpsChn, int vencChn);
int hb_venc_output_release(int channel, VIDEO_STREAM_S *pstStream);
int hb_venc_get_output(int channel, VIDEO_STREAM_S *pstStream);
int hb_venc_deinit(int channel);
void venc_thread(void *param);
#endif // INCLUDE_VIO_VENC_H_
