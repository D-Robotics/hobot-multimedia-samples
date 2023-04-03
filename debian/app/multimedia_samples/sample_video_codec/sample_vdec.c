/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2019 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>

#include "sample.h"

#define SET_BYTE(_p, _b) \
    *_p++ = (unsigned char)_b;

#define SET_BUFFER(_p, _buf, _len) \
    memcpy(_p, _buf, _len);        \
    (_p) += (_len);

static int build_dec_seq_header(uint8_t *pbHeader,
                                const media_codec_id_t codStd, const AVStream *st, int *sizelength)
{
    AVCodecParameters *avc = st->codecpar;

    uint8_t *pbMetaData = avc->extradata;
    int nMetaData = avc->extradata_size;
    uint8_t *p = pbMetaData;
    uint8_t *a = p + 4 - ((long)p & 3);
    uint8_t *t = pbHeader;
    int size;
    int sps, pps, i, nal;

    size = 0;
    *sizelength = 4; // default size length(in bytes) = 4
    if (codStd == MEDIA_CODEC_ID_H264)
    {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01)
        {
            // check mov/mo4 file format stream
            p += 4;
            *sizelength = (*p++ & 0x3) + 1;
            sps = (*p & 0x1f); // Number of sps
            p++;
            for (i = 0; i < sps; i++)
            {
                nal = (*p << 8) + *(p + 1) + 2;
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x01);
                SET_BUFFER(t, p + 2, nal - 2);
                p += nal;
                size += (nal - 2 + 4); // 4 => length of start code to be inserted
            }

            pps = *(p++); // number of pps
            for (i = 0; i < pps; i++)
            {
                nal = (*p << 8) + *(p + 1) + 2;
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x01);
                SET_BUFFER(t, p + 2, nal - 2);
                p += nal;
                size += (nal - 2 + 4); // 4 => length of start code to be inserted
            }
        }
        else if (nMetaData > 3)
        {
            size = -1; // return to meaning of invalid stream data;
            for (; p < a; p++)
            {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1)
                {
                    // find startcode
                    size = avc->extradata_size;
                    if (pbMetaData && 0x00 == pbMetaData[size - 1])
                    {
                        size -= 1;
                    }
                    if (!pbHeader || !pbMetaData)
                        return 0;
                    SET_BUFFER(pbHeader, pbMetaData, size);
                    break;
                }
            }
        }
    }
    else if (codStd == MEDIA_CODEC_ID_H265)
    {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01)
        {
            static const int8_t nalu_header[4] = {0, 0, 0, 1};
            int numOfArrays = 0;
            uint16_t numNalus = 0;
            uint16_t nalUnitLength = 0;
            uint32_t offset = 0;

            p += 21;
            *sizelength = (*p++ & 0x3) + 1;
            numOfArrays = *p++;

            while (numOfArrays--)
            {
                p++; // NAL type
                numNalus = (*p << 8) + *(p + 1);
                p += 2;
                for (i = 0; i < numNalus; i++)
                {
                    nalUnitLength = (*p << 8) + *(p + 1);
                    p += 2;
                    // if(i == 0)
                    {
                        memcpy(pbHeader + offset, nalu_header, 4);
                        offset += 4;
                        memcpy(pbHeader + offset, p, nalUnitLength);
                        offset += nalUnitLength;
                    }
                    p += nalUnitLength;
                }
            }

            size = offset;
        }
        else if (nMetaData > 3)
        {
            size = -1; // return to meaning of invalid stream data;

            for (; p < a; p++)
            {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1) // find startcode
                {
                    size = avc->extradata_size;
                    if (!pbHeader || !pbMetaData)
                        return 0;
                    SET_BUFFER(pbHeader, pbMetaData, size);
                    break;
                }
            }
        }
    }
    else
    {
        SET_BUFFER(pbHeader, pbMetaData, nMetaData);
        size = nMetaData;
    }

    return size;
}

void send_stream(void *args)
{
    int ret;
    AVFormatContext *avContext = NULL;
    AVPacket avpacket = {0};
    int videoIndex;
    uint8_t *seqHeader = NULL;
    int seqHeaderSize = 0;
    uint8_t *picHeader = NULL;
    int picHeaderSize;
    int firstPacket = 1;
    int eos = 0;
    int bufSize = 0;
    int count = 0;
    int mmz_index = 0;
    int error = 0;
    AVDictionary *option = 0;
    vdec_param *arg = (vdec_param *)args;
    media_codec_context_t *context = arg->context;

    av_dict_set(&option, "stimeout", "3000000", 0);
    av_dict_set(&option, "bufsize", "1024000", 0);
    av_dict_set(&option, "rtsp_transport", "tcp", 0);
    do
    {
        ret = avformat_open_input(&avContext, arg->input, 0, &option);
        if (ret != 0)
        {
            printf("avformat_open_input: %d, retry\n", ret);
        }
    } while ((ret != 0) && (running));
    ret = avformat_find_stream_info(avContext, 0);
    videoIndex = av_find_best_stream(avContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    av_init_packet(&avpacket);

    while (running)
    {
        media_codec_buffer_t inputBuffer;
        memset(&inputBuffer, 0x00, sizeof(media_codec_buffer_t));

        ret = hb_mm_mc_dequeue_input_buffer(context, &inputBuffer, 3000);
        if (ret != 0)
        {
            printf("hb_mm_mc_dequeue_input_buffer failed %x\n", ret);
            break;
        }
        if (!avpacket.size)
        {
            error = av_read_frame(avContext, &avpacket);
        }
        if (error < 0)
        {
            inputBuffer.vstream_buf.stream_end = HB_TRUE;
            inputBuffer.vstream_buf.size = 0;
            // break;
        }
        else
        {

            seqHeaderSize = 0;
            if (firstPacket)
            {

                AVCodecParameters *codec;
                int retSize = 0;
                codec = avContext->streams[videoIndex]->codecpar;
                seqHeader = (uint8_t *)malloc(codec->extradata_size + 1024);
                if (seqHeader == NULL)
                {
                    printf("Failed to mallock seqHeader\n");
                    error = eos = 1;
                    break;
                }
                memset((void *)seqHeader, 0x00, codec->extradata_size + 1024);

                seqHeaderSize = build_dec_seq_header(seqHeader,
                                                     context->codec_id,
                                                     avContext->streams[videoIndex], &retSize);
                if (seqHeaderSize < 0)
                {
                    printf("Failed to build seqHeader\n");
                    error = eos = 1;
                    break;
                }
                firstPacket = 0;
            }

            if ((avpacket.size <= (int)inputBuffer.vstream_buf.size) && (seqHeaderSize <= (int)inputBuffer.vstream_buf.size))
            {
                int bufSize = 0;
                if (seqHeaderSize)
                {
                    memcpy((char *)inputBuffer.vstream_buf.vir_ptr,
                           seqHeader, seqHeaderSize);
                    bufSize = seqHeaderSize;
                }
                else
                {
                    memcpy((char *)inputBuffer.vstream_buf.vir_ptr,
                           avpacket.data, avpacket.size);
                    bufSize = avpacket.size;
                    av_packet_unref(&avpacket);
                    avpacket.size = 0;
                }
                inputBuffer.vstream_buf.size = bufSize;
            }
            else
            {
                printf("The stream buffer is too small!\n");
                error = eos = 1;
                break;
            }

            if (seqHeader)
            {
                free(seqHeader);
                seqHeader = NULL;
            }
        }

        ret = hb_mm_mc_queue_input_buffer(context, &inputBuffer, 100);
        if (ret != 0)
        {
            printf("hb_mm_mc_queue_input_buffer failed %x\n", ret);
            break;
        }
        if (inputBuffer.vstream_buf.stream_end)
            break;
    }

    if (avContext)
        avformat_close_input(&avContext);
    if (seqHeader)
    {
        free(seqHeader);
        seqHeader = NULL;
    }
}

// int sample_vdec(int type, int width, int height)
void *sample_vdec(void *arg)
{
    vdec_param *args = (vdec_param *)arg;
    int ERR_RET = -1;
    int NORMAL_RET = 0;
    int type = args->type;
    int width = args->width;
    int height = args->height;
    char output_file_name[100] = {0};
    char *yuv_suffix = ".yuv";
    // fixme:using global variable to set open file name
    // g_inputfilename = args->input;
    int i = 0;
    int cnt = 0;
    int ret;
    int size, w, h;
    ion_buffer_t ionbuf;
    media_codec_context_t *context;
    mc_video_codec_dec_params_t *params;
    FILE *fout;
    pthread_t thread_id;

    context = (media_codec_context_t *)malloc(sizeof(media_codec_context_t));
    memset(context, 0x00, sizeof(media_codec_context_t));
    context->codec_id = type;
    context->encoder = HB_FALSE;
    params = &(context->video_dec_params);
    params->feed_mode = MC_FEEDING_MODE_FRAME_SIZE;
    params->pix_fmt = MC_PIXEL_FORMAT_NV12;
    params->bitstream_buf_count = 3;
    params->frame_buf_count = 3;
    params->bitstream_buf_size = ((width * height * 3 / 2) + 0x3FF) & (~0x3FF);
    params->external_bitstream_buf = HB_FALSE;
    if (context->codec_id == MEDIA_CODEC_ID_H264)
    {
        params->h264_dec_config.bandwidth_Opt = HB_TRUE;
        params->h264_dec_config.reorder_enable = HB_TRUE;
        params->h264_dec_config.skip_mode = 0;
    }
    else if (context->codec_id == MEDIA_CODEC_ID_H265)
    {
        params->h265_dec_config.bandwidth_Opt = HB_TRUE;
        params->h265_dec_config.reorder_enable = HB_TRUE;
        params->h265_dec_config.skip_mode = 0;
        params->h265_dec_config.cra_as_bla = HB_FALSE;
        params->h265_dec_config.dec_temporal_id_mode = 0;
        params->h265_dec_config.target_dec_temporal_id_plus1 = 0;
    }
    else
    {
        params->mjpeg_dec_config.rot_degree = MC_CCW_0;
        params->mjpeg_dec_config.mir_direction = MC_DIRECTION_NONE;
        params->mjpeg_dec_config.frame_crop_enable = HB_FALSE;
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
    args->context = context;
    pthread_create(&thread_id, NULL, (void *)send_stream, args);

    while (running)
    {
        media_codec_buffer_t outputBuffer;
        media_codec_output_buffer_info_t info;
        memset(&outputBuffer, 0x00, sizeof(media_codec_buffer_t));
        memset(&info, 0x00, sizeof(media_codec_output_buffer_info_t));
        ret = hb_mm_mc_dequeue_output_buffer(context, &outputBuffer, &info, 3000);
        if (ret == 0)
        {
            if (context->codec_id == MEDIA_CODEC_ID_JPEG)
                printf("w = %d, h = %d\n", info.jpeg_frame_info.display_width, info.jpeg_frame_info.display_height);
            sprintf(output_file_name, "%s_%d%s", args->prefix, cnt++, yuv_suffix);
            // printf("name : %s\n", output_file_name);
            fout = fopen(output_file_name, "wb");
            if (fout == NULL)
            {
                printf("fopen outputfile %s failed\n", output_file_name);
            }
            if (!outputBuffer.vframe_buf.frame_end && fout)
            {
                fwrite(outputBuffer.vframe_buf.vir_ptr[0],
                       outputBuffer.vframe_buf.width * outputBuffer.vframe_buf.height, 1, fout);
                fwrite(outputBuffer.vframe_buf.vir_ptr[1],
                       outputBuffer.vframe_buf.width * outputBuffer.vframe_buf.height / 2, 1, fout);
            }
            ret = hb_mm_mc_queue_output_buffer(context, &outputBuffer, 100);
            if (outputBuffer.vframe_buf.frame_end)
            {
                printf("There is no more output data!\n");
                break;
            }
        }
    }

    pthread_join(thread_id, NULL);
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

    if (fout)
        fclose(fout);
    pthread_exit((void *)NORMAL_RET);
}
