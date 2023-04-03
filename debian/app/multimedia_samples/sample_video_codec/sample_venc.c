/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2019 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sample.h"

int get_rc_params(media_codec_context_t *context,
                  mc_rate_control_params_t *rc_params)
{
    int ret = 0;
    ret = hb_mm_mc_get_rate_control_config(context, rc_params);
    if (ret)
    {
        printf("Failed to get rc params ret=0x%x\n", ret);
        return ret;
    }
    printf("mode:%d\n", rc_params->mode);
    switch (rc_params->mode)
    {
    case MC_AV_RC_MODE_H264CBR:
        rc_params->h264_cbr_params.intra_period = 30;
        rc_params->h264_cbr_params.intra_qp = 30;
        rc_params->h264_cbr_params.bit_rate = 5000;
        rc_params->h264_cbr_params.frame_rate = 30;
        rc_params->h264_cbr_params.initial_rc_qp = 20;
        rc_params->h264_cbr_params.vbv_buffer_size = 20;
        rc_params->h264_cbr_params.mb_level_rc_enalbe = 1;
        rc_params->h264_cbr_params.min_qp_I = 8;
        rc_params->h264_cbr_params.max_qp_I = 50;
        rc_params->h264_cbr_params.min_qp_P = 8;
        rc_params->h264_cbr_params.max_qp_P = 50;
        rc_params->h264_cbr_params.min_qp_B = 8;
        rc_params->h264_cbr_params.max_qp_B = 50;
        rc_params->h264_cbr_params.hvs_qp_enable = 1;
        rc_params->h264_cbr_params.hvs_qp_scale = 2;
        rc_params->h264_cbr_params.max_delta_qp = 10;
        rc_params->h264_cbr_params.qp_map_enable = 0;
        break;
    case MC_AV_RC_MODE_H264VBR:
        rc_params->h264_vbr_params.intra_qp = 20;
        rc_params->h264_vbr_params.intra_period = 30;
        rc_params->h264_vbr_params.intra_qp = 35;
        break;
    case MC_AV_RC_MODE_H264AVBR:
        rc_params->h264_avbr_params.intra_period = 15;
        rc_params->h264_avbr_params.intra_qp = 25;
        rc_params->h264_avbr_params.bit_rate = 2000;
        rc_params->h264_avbr_params.vbv_buffer_size = 3000;
        rc_params->h264_avbr_params.min_qp_I = 15;
        rc_params->h264_avbr_params.max_qp_I = 50;
        rc_params->h264_avbr_params.min_qp_P = 15;
        rc_params->h264_avbr_params.max_qp_P = 45;
        rc_params->h264_avbr_params.min_qp_B = 15;
        rc_params->h264_avbr_params.max_qp_B = 48;
        rc_params->h264_avbr_params.hvs_qp_enable = 0;
        rc_params->h264_avbr_params.hvs_qp_scale = 2;
        rc_params->h264_avbr_params.max_delta_qp = 5;
        rc_params->h264_avbr_params.qp_map_enable = 0;
        break;
    case MC_AV_RC_MODE_H264FIXQP:
        rc_params->h264_fixqp_params.force_qp_I = 23;
        rc_params->h264_fixqp_params.force_qp_P = 23;
        rc_params->h264_fixqp_params.force_qp_B = 23;
        rc_params->h264_fixqp_params.intra_period = 23;
        break;
    case MC_AV_RC_MODE_H264QPMAP:
        break;
    case MC_AV_RC_MODE_H265CBR:
        rc_params->h265_cbr_params.intra_period = 20;
        rc_params->h265_cbr_params.intra_qp = 30;
        rc_params->h265_cbr_params.bit_rate = 5000;
        rc_params->h265_cbr_params.frame_rate = 30;
        rc_params->h265_cbr_params.initial_rc_qp = 20;
        rc_params->h265_cbr_params.vbv_buffer_size = 20;
        rc_params->h265_cbr_params.ctu_level_rc_enalbe = 1;
        rc_params->h265_cbr_params.min_qp_I = 8;
        rc_params->h265_cbr_params.max_qp_I = 50;
        rc_params->h265_cbr_params.min_qp_P = 8;
        rc_params->h265_cbr_params.max_qp_P = 50;
        rc_params->h265_cbr_params.min_qp_B = 8;
        rc_params->h265_cbr_params.max_qp_B = 50;
        rc_params->h265_cbr_params.hvs_qp_enable = 1;
        rc_params->h265_cbr_params.hvs_qp_scale = 2;
        rc_params->h265_cbr_params.max_delta_qp = 10;
        rc_params->h265_cbr_params.qp_map_enable = 0;
        break;
    case MC_AV_RC_MODE_H265VBR:
        rc_params->h265_vbr_params.intra_qp = 20;
        rc_params->h265_vbr_params.intra_period = 30;
        rc_params->h265_vbr_params.intra_qp = 35;
        break;
    case MC_AV_RC_MODE_H265AVBR:
        rc_params->h265_avbr_params.intra_period = 15;
        rc_params->h265_avbr_params.intra_qp = 25;
        rc_params->h265_avbr_params.bit_rate = 2000;
        rc_params->h265_avbr_params.vbv_buffer_size = 3000;
        rc_params->h265_avbr_params.min_qp_I = 15;
        rc_params->h265_avbr_params.max_qp_I = 50;
        rc_params->h265_avbr_params.min_qp_P = 15;
        rc_params->h265_avbr_params.max_qp_P = 45;
        rc_params->h265_avbr_params.min_qp_B = 15;
        rc_params->h265_avbr_params.max_qp_B = 48;
        rc_params->h265_avbr_params.hvs_qp_enable = 0;
        rc_params->h265_avbr_params.hvs_qp_scale = 2;
        rc_params->h265_avbr_params.max_delta_qp = 5;
        rc_params->h265_avbr_params.qp_map_enable = 0;
        break;
    case MC_AV_RC_MODE_H265FIXQP:
        rc_params->h265_fixqp_params.force_qp_I = 23;
        rc_params->h265_fixqp_params.force_qp_P = 23;
        rc_params->h265_fixqp_params.force_qp_B = 23;
        rc_params->h265_fixqp_params.intra_period = 23;
        break;
    case MC_AV_RC_MODE_H265QPMAP:
        break;
    case MC_AV_RC_MODE_MJPEGFIXQP:
        rc_params->mjpeg_fixqp_params.quality_factor = 0;
        rc_params->mjpeg_fixqp_params.frame_rate = 60;
        break;
    default:
        ret = HB_MEDIA_ERR_INVALID_PARAMS;
        break;
    }
    return ret;
}

// int sample_venc(int type, int width,
//                 int height, int mode, int external_buf)
void *sample_venc(void *arg)
{
    int ERR_RET = -1;
    int NORMAL_RET = 0;
    venc_param *args = (venc_param *)arg;
    int i = 0;
    int ret;
    int size, w, h;
    ion_buffer_t ionbuf;
    media_codec_context_t *context;
    mc_video_codec_enc_params_t *params;
    FILE *fin, *fout;
    char *outputfile;

    w = args->width;
    h = args->height;

    fin = fopen(args->input, "rb");
    if (fin == NULL)
    {
        printf("fopen inputfile %s failed\n", args->input);
        pthread_exit((void *)ERR_RET);
    }
    fout = fopen(args->output, "wb");
    if (fout == NULL)
    {
        printf("fopen outputfile %s failed\n", args->output);
        pthread_exit((void *)ERR_RET);
    }

    context = (media_codec_context_t *)malloc(sizeof(media_codec_context_t));
    memset(context, 0x00, sizeof(media_codec_context_t));
    context->codec_id = args->type;
    context->encoder = HB_TRUE;
    params = &(context->video_enc_params);
    params->enable_user_pts = HB_TRUE;
    params->width = args->width;
    params->height = args->height;
    params->pix_fmt = MC_PIXEL_FORMAT_NV12;
    params->frame_buf_count = 5;
    params->external_frame_buf = args->ext_buf;
    params->bitstream_buf_count = 5;
    params->rc_params.mode = args->rc_mode;

    if (context->codec_id == MEDIA_CODEC_ID_H264 ||
        context->codec_id == MEDIA_CODEC_ID_H265)
    {
        //fixme 2023/3/6
        //Due to the bug of libmm, the frame rate modification of H264 is temporarily unavailable, and all output H264 stream is 25fps
        get_rc_params(context, &params->rc_params);
        params->gop_params.decoding_refresh_type = 2;
        params->gop_params.gop_preset_idx = 2;
        params->rot_degree = MC_CCW_0;
        params->mir_direction = MC_DIRECTION_NONE;
        params->frame_cropping_flag = HB_FALSE;
    }
    else if (context->codec_id == MEDIA_CODEC_ID_MJPEG)
    {
        get_rc_params(context, &params->rc_params);
    }

    ret = hb_mm_mc_initialize(context);
    if (ret != 0)
    {
        printf("hb_mm_mc_initialize failed, ret: %x\n", ret);
        pthread_exit((void *)ERR_RET);
    }
    ret = hb_mm_mc_configure(context);
    if (ret != 0)
    {
        printf("hb_mm_mc_configure failed, ret: %x\n", ret);
        pthread_exit((void *)ERR_RET);
    }
    mc_av_codec_startup_params_t startup_params;
    startup_params.video_enc_startup_params.receive_frame_number = 0;
    ret = hb_mm_mc_start(context, &startup_params);
    if (ret != 0)
    {
        printf("hb_mm_mc_start failed, ret: %x\n", ret);
        pthread_exit((void *)ERR_RET);
    }

    if (context->video_enc_params.external_frame_buf)
    {
        ret = allocate_ion_mem(w * h * 3 / 2, &ionbuf);
    }

    while (running)
    {
        media_codec_buffer_t inputBuffer;
        size = context->video_enc_params.width * context->video_enc_params.height;
        memset(&inputBuffer, 0x00, sizeof(media_codec_buffer_t));

        ret = hb_mm_mc_dequeue_input_buffer(context, &inputBuffer, 3000);
        if (ret != 0)
        {
            printf("hb_mm_mc_dequeue_input_buffer failed %x\n", ret);
            pthread_exit((void *)ERR_RET);
        }

        if (context->video_enc_params.external_frame_buf)
        {
            ret = fread((void *)(ionbuf.virt_addr), 1, size * 3 / 2, fin);
            if (ret != size * 3 / 2)
                break;
            inputBuffer.vframe_buf.vir_ptr[0] = (unsigned char *)ionbuf.virt_addr;
            inputBuffer.vframe_buf.vir_ptr[1] = (unsigned char *)ionbuf.virt_addr + size;

            inputBuffer.vframe_buf.phy_ptr[0] = ionbuf.phys_addr;
            inputBuffer.vframe_buf.phy_ptr[1] = ionbuf.phys_addr + size;
            inputBuffer.vframe_buf.size = size * 3 / 2;
            fseek(fin, 0, SEEK_SET);
        }
        else
        {
            ret = fread(inputBuffer.vframe_buf.vir_ptr[0], 1, size, fin);
            ret = fread(inputBuffer.vframe_buf.vir_ptr[1], 1, size / 2, fin);
            fseek(fin, 0, SEEK_SET);
        }
        ret = hb_mm_mc_queue_input_buffer(context, &inputBuffer, 100);
        if (ret != 0)
        {
            printf("hb_mm_mc_queue_input_buffer failed %x\n", ret);
            pthread_exit((void *)ERR_RET);
        }

        media_codec_buffer_t outputBuffer;
        media_codec_output_buffer_info_t info;
        memset(&outputBuffer, 0x00, sizeof(media_codec_buffer_t));
        memset(&info, 0x00, sizeof(media_codec_output_buffer_info_t));
        ret = hb_mm_mc_dequeue_output_buffer(context, &outputBuffer, &info, 3000);
        if (ret == 0)
        {
            if (fout)
            {
                fwrite(outputBuffer.vstream_buf.vir_ptr,
                       outputBuffer.vstream_buf.size, 1, fout);
            }
            ret = hb_mm_mc_queue_output_buffer(context, &outputBuffer, 100);
        }
    }

    if (context->video_enc_params.external_frame_buf)
    {
        release_ion_mem(&ionbuf);
    }

    ret = hb_mm_mc_stop(context);
    if (ret != 0)
    {
        printf("hb_mm_mc_stop failed, ret: %x\n", ret);
        pthread_exit((void *)ERR_RET);
    }
    ret = hb_mm_mc_release(context);
    if (ret != 0)
    {
        printf("hb_mm_mc_release failed, ret: %x\n", ret);
        pthread_exit((void *)ERR_RET);
    }

    if (fin)
        fclose(fin);
    if (fout)
        fclose(fout);
    pthread_exit((void *)NORMAL_RET);
}
