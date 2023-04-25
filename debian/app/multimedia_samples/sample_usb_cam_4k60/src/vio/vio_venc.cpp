/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "hb_vio_interface.h"
#include "hb_vps_api.h"
#include "utils/utils.h"
#include "vio_venc.h"
int g_intra_qp = 12;
//int g_intra_qp = 15;
int g_inter_qp = 32;
extern int running;

int hb_venc_init(vencParam_t *vencparam)
{
    int ret = 0;
    PIXEL_FORMAT_E pixFmt = HB_PIXEL_FORMAT_NV12;
    VENC_RC_ATTR_S *pstRcParam;
    VENC_CHN_ATTR_S vencChnAttr;

    memset(&vencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    vencChnAttr.stVencAttr.enType = vencparam->type;
    vencChnAttr.stVencAttr.u32PicWidth = vencparam->width;
    vencChnAttr.stVencAttr.u32PicHeight = vencparam->height;
    vencChnAttr.stVencAttr.enMirrorFlip = DIRECTION_NONE;
    vencChnAttr.stVencAttr.enRotation = CODEC_ROTATION_0;
    vencChnAttr.stVencAttr.stCropCfg.bEnable = HB_FALSE;
    vencChnAttr.stVencAttr.bEnableUserPts = HB_TRUE;
    vencChnAttr.stVencAttr.s32BufJoint = 0;
    vencChnAttr.stVencAttr.s32BufJointSize = 8000000;
    vencChnAttr.stVencAttr.enPixelFormat = pixFmt;
    vencChnAttr.stVencAttr.u32BitStreamBufferCount = 3;
    vencChnAttr.stVencAttr.u32FrameBufferCount = 3;
    vencChnAttr.stGopAttr.u32GopPresetIdx = 6;
    vencChnAttr.stGopAttr.s32DecodingRefreshType = 2;

    auto size = vencparam->width * vencparam->height;
    auto streambuf = size & 0xfffff000;
    if (size > 2688 * 1522) {
        vencChnAttr.stVencAttr.vlc_buf_size = 7900*1024;
    } else if (size > 1920 * 1080) {
        vencChnAttr.stVencAttr.vlc_buf_size = 4*1024*1024;
    } else if (size > 1280 * 720) {
        vencChnAttr.stVencAttr.vlc_buf_size = 2100*1024;
    } else if (size > 704 * 576) {
        vencChnAttr.stVencAttr.vlc_buf_size = 2100*1024;
    } else {
        vencChnAttr.stVencAttr.vlc_buf_size = 2048*1024;
    }

    if (vencparam->type == PT_MJPEG) {
        if (vencparam->stride) {
            vencChnAttr.stVencAttr.u32PicWidth = vencparam->stride;
        }
        vencChnAttr.stVencAttr.bExternalFreamBuffer = HB_TRUE;
        vencChnAttr.stVencAttr.stAttrJpeg.dcf_enable = HB_FALSE;
        vencChnAttr.stVencAttr.stAttrJpeg.quality_factor = 0;
        vencChnAttr.stVencAttr.stAttrJpeg.restart_interval = 0;
        vencChnAttr.stVencAttr.u32BitStreamBufSize = streambuf;
        vencChnAttr.stVencAttr.stCropCfg.bEnable = HB_TRUE;
        vencChnAttr.stVencAttr.stCropCfg.stRect.s32X = 0;
        vencChnAttr.stVencAttr.stCropCfg.stRect.s32Y = 0;
        vencChnAttr.stVencAttr.stCropCfg.stRect.u32Width = vencparam->width;
        vencChnAttr.stVencAttr.stCropCfg.stRect.u32Height = vencparam->height;
    } else if (vencparam->type == PT_H264) {
        vencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
				vencChnAttr.stVencAttr.bExternalFreamBuffer = HB_TRUE;
        vencChnAttr.stRcAttr.stH264Vbr.bQpMapEnable = HB_TRUE;
        vencChnAttr.stRcAttr.stH264Vbr.u32IntraQp = 20;
        vencChnAttr.stRcAttr.stH264Vbr.u32IntraPeriod = 20;
        vencChnAttr.stRcAttr.stH264Vbr.u32FrameRate = 60;
        //vencChnAttr.stVencAttr.stAttrH264.h264_profile = HB_H264_PROFILE_MP;
        vencChnAttr.stVencAttr.stAttrH264.h264_profile = HB_H264_PROFILE_UNSPECIFIED;
        //vencChnAttr.stVencAttr.stAttrH264.h264_level = HB_H264_LEVEL1;
        vencChnAttr.stVencAttr.stAttrH264.h264_level = HB_H264_LEVEL_UNSPECIFIED;
    } else if (vencparam->type == PT_H265) {
        vencChnAttr.stVencAttr.bExternalFreamBuffer = HB_TRUE;
        vencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
        vencChnAttr.stRcAttr.stH265Vbr.bQpMapEnable = HB_TRUE;
        vencChnAttr.stRcAttr.stH265Vbr.u32IntraQp = 20;
        vencChnAttr.stRcAttr.stH265Vbr.u32IntraPeriod = 20;
        vencChnAttr.stRcAttr.stH265Vbr.u32FrameRate = 25;
    }

    ret = HB_VENC_CreateChn(vencparam->veChn, &vencChnAttr);
    if (ret != 0) {
        printf("HB_VENC_CreateChn %d failed, %d.\n", 0, ret);
        return -1;
    }

   pstRcParam = &(vencChnAttr.stRcAttr);
   if (vencparam->type == PT_MJPEG)	{
		vencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGFIXQP;
		ret = HB_VENC_GetRcParam(vencparam->veChn, pstRcParam);
		if (ret != 0) {
			printf("HB_VENC_GetRcParam failed.\n");
			return -1;
		}
        pstRcParam->stMjpegFixQp.u32FrameRate = 60;
        pstRcParam->stMjpegFixQp.u32QualityFactort = vencparam->mjpgQp;
        ret = HB_VENC_SetRcParam(vencparam->veChn, pstRcParam);
        if (ret) {
            printf("MJPG HB_VENC_SetRcParam vencparam->veChn = %d, ret = %d",vencparam->veChn,ret);
            return ret;
        }
    } else if (vencparam->type == PT_H264) {
        vencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
        ret = HB_VENC_GetRcParam(vencparam->veChn, pstRcParam);
        if (ret != 0) {
            printf("HB_VENC_GetRcParam failed.\n");
            return -1;
        }

        switch (vencChnAttr.stRcAttr.enRcMode) {
        case VENC_RC_MODE_H264CBR:
        {
            pstRcParam->stH264Cbr.u32BitRate = vencparam->bitrate;
            pstRcParam->stH264Cbr.u32FrameRate = 60;
            pstRcParam->stH264Cbr.u32IntraPeriod = 60;
            pstRcParam->stH264Cbr.u32VbvBufferSize = 3000;
            pstRcParam->stH264Cbr.u32IntraQp = g_intra_qp;
            //pstRcParam->stH264Cbr.u32InitialRcQp = g_intra_qp;
            pstRcParam->stH264Cbr.u32InitialRcQp = g_intra_qp;
            //pstRcParam->stH264Cbr.bMbLevelRcEnable = HB_FALSE;
            pstRcParam->stH264Cbr.bMbLevelRcEnable = HB_TRUE;
            pstRcParam->stH264Cbr.u32MaxIQp = 51;
            pstRcParam->stH264Cbr.u32MinIQp = 10;
            pstRcParam->stH264Cbr.u32MaxPQp = 51;
            pstRcParam->stH264Cbr.u32MinPQp = 10;
            pstRcParam->stH264Cbr.u32MaxBQp = 51;
            pstRcParam->stH264Cbr.u32MinBQp = 10;
            pstRcParam->stH264Cbr.bHvsQpEnable = HB_FALSE;
            pstRcParam->stH264Cbr.s32HvsQpScale = 2;
            pstRcParam->stH264Cbr.u32MaxDeltaQp = 3;
            pstRcParam->stH264Cbr.bQpMapEnable = HB_FALSE;
            break;
        }
        case VENC_RC_MODE_H264VBR:
        {
            pstRcParam->stH264Vbr.bQpMapEnable = HB_FALSE;
            pstRcParam->stH264Vbr.u32FrameRate = 60;
            pstRcParam->stH264Vbr.u32IntraPeriod = 60;
            pstRcParam->stH264Vbr.u32IntraQp = 25;
            break;
        }
        case VENC_RC_MODE_H264AVBR:
        {
            pstRcParam->stH264AVbr.u32BitRate = vencparam->bitrate;
            pstRcParam->stH264AVbr.u32FrameRate = 65;
            pstRcParam->stH264AVbr.u32IntraPeriod = 50;
            pstRcParam->stH264AVbr.u32VbvBufferSize = 3000;
            pstRcParam->stH264AVbr.u32IntraQp = 30;
            pstRcParam->stH264AVbr.u32InitialRcQp = 45;
            pstRcParam->stH264AVbr.bMbLevelRcEnable = HB_FALSE;
            pstRcParam->stH264AVbr.u32MaxIQp = 51;
            pstRcParam->stH264AVbr.u32MinIQp = 28;
            pstRcParam->stH264AVbr.u32MaxPQp = 51;
            pstRcParam->stH264AVbr.u32MinPQp = 28;
            pstRcParam->stH264AVbr.u32MaxBQp = 51;
            pstRcParam->stH264AVbr.u32MinBQp = 28;
            pstRcParam->stH264AVbr.bHvsQpEnable = HB_TRUE;
            pstRcParam->stH264AVbr.s32HvsQpScale = 2;
            pstRcParam->stH264AVbr.u32MaxDeltaQp = 3;
            pstRcParam->stH264AVbr.bQpMapEnable = HB_FALSE;
            break;
        }
        case VENC_RC_MODE_H264FIXQP:
        {
            pstRcParam->stH264FixQp.u32FrameRate = 60;
            pstRcParam->stH264FixQp.u32IntraPeriod = 60;
            pstRcParam->stH264FixQp.u32IQp = 35;
            pstRcParam->stH264FixQp.u32PQp = 35;
            pstRcParam->stH264FixQp.u32BQp = 35;
            break;
        }
        case VENC_RC_MODE_H264QPMAP:
        {
            auto width = vencChnAttr.stVencAttr.u32PicWidth;
            auto height = vencChnAttr.stVencAttr.u32PicHeight;
            pstRcParam->stH264QpMap.u32FrameRate = 60;
            pstRcParam->stH264QpMap.u32IntraPeriod = 60;
            pstRcParam->stH264QpMap.u32QpMapArrayCount =
                ((width+0x0f)&~0x0f)/16 * ((height+0x0f)&~0x0f)/16;
            pstRcParam->stH264QpMap.u32QpMapArray =
                reinterpret_cast<unsigned char *>(malloc(pstRcParam->stH264QpMap.u32QpMapArrayCount));
            for(int i = 0; i < pstRcParam->stH264QpMap.u32QpMapArrayCount; i++) {
                pstRcParam->stH264QpMap.u32QpMapArray[i] = 30;
            }
            break;
        }
        default:
            break;
        }
    } else if (vencparam->type == PT_H265) {
        vencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
        ret = HB_VENC_GetRcParam(vencparam->veChn, pstRcParam);
        if (ret != 0) {
            printf("HB_VENC_GetRcParam failed.\n");
            return -1;
        }
        switch (vencChnAttr.stRcAttr.enRcMode) {
        case VENC_RC_MODE_H265CBR:
        {
            VENC_H265_CBR_S stH265Cbr;
            memset(&stH265Cbr, 0, sizeof(stH265Cbr));
            pstRcParam->stH265Cbr.u32IntraPeriod = 30;
            pstRcParam->stH265Cbr.u32IntraQp = 30;
            pstRcParam->stH265Cbr.u32BitRate = vencparam->bitrate;
            pstRcParam->stH265Cbr.u32FrameRate = 30;
            pstRcParam->stH265Cbr.u32InitialRcQp = 45;
            pstRcParam->stH265Cbr.u32VbvBufferSize = 3000;
            pstRcParam->stH265Cbr.bCtuLevelRcEnable = HB_FALSE;
            pstRcParam->stH265Cbr.u32MaxIQp = 45;
            pstRcParam->stH265Cbr.u32MinIQp = 22;
            pstRcParam->stH265Cbr.u32MaxPQp = 45;
            pstRcParam->stH265Cbr.u32MinPQp = 22;
            pstRcParam->stH265Cbr.u32MaxBQp = 45;
            pstRcParam->stH265Cbr.u32MinBQp = 22;
            pstRcParam->stH265Cbr.bHvsQpEnable = HB_FALSE;
            pstRcParam->stH265Cbr.s32HvsQpScale = 2;
            pstRcParam->stH265Cbr.u32MaxDeltaQp = 10;
            pstRcParam->stH265Cbr.bQpMapEnable = HB_FALSE;
            break;
        }
        case VENC_RC_MODE_H265VBR:
        {
            VENC_H265_VBR_S stH265Vbr;
            memset(&stH265Vbr, 0, sizeof(stH265Vbr));
            pstRcParam->stH265Vbr.u32IntraPeriod = 30;
            pstRcParam->stH265Vbr.u32FrameRate = 30;
            pstRcParam->stH265Vbr.bQpMapEnable = HB_FALSE;
            pstRcParam->stH265Vbr.u32IntraQp = 32;
            break;
        }
        case VENC_RC_MODE_H265AVBR:
        {
            VENC_H265_AVBR_S stH265Avbr;
            memset(&stH265Avbr, 0, sizeof(stH265Avbr));
            pstRcParam->stH265AVbr.u32IntraPeriod = 30;
            pstRcParam->stH265AVbr.u32IntraQp = 30;
            pstRcParam->stH265AVbr.u32BitRate = vencparam->bitrate;
            pstRcParam->stH265AVbr.u32FrameRate = 30;
            pstRcParam->stH265AVbr.u32InitialRcQp = 45;
            pstRcParam->stH265AVbr.u32VbvBufferSize = 3000;
            pstRcParam->stH265AVbr.bCtuLevelRcEnable = HB_FALSE;
            pstRcParam->stH265AVbr.u32MaxIQp = 45;
            pstRcParam->stH265AVbr.u32MinIQp = 22;
            pstRcParam->stH265AVbr.u32MaxPQp = 45;
            pstRcParam->stH265AVbr.u32MinPQp = 22;
            pstRcParam->stH265AVbr.u32MaxBQp = 45;
            pstRcParam->stH265AVbr.u32MinBQp = 22;
            pstRcParam->stH265AVbr.bHvsQpEnable = HB_TRUE;
            pstRcParam->stH265AVbr.s32HvsQpScale = 2;
            pstRcParam->stH265AVbr.u32MaxDeltaQp = 10;
            pstRcParam->stH265AVbr.bQpMapEnable = HB_FALSE;
            break;
        }
        case VENC_RC_MODE_H265FIXQP:
        {
            VENC_H265_FIXQP_S stH265FixQp;
            memset(&stH265FixQp, 0, sizeof(stH265FixQp));
            pstRcParam->stH265FixQp.u32FrameRate = 30;
            pstRcParam->stH265FixQp.u32IntraPeriod = 30;
            pstRcParam->stH265FixQp.u32IQp = 35;
            pstRcParam->stH265FixQp.u32PQp = 35;
            pstRcParam->stH265FixQp.u32BQp = 35;
            break;
        }
        case VENC_RC_MODE_H265QPMAP:
        {
            VENC_H265_QPMAP_S stH265QpMap;
            memset(&stH265QpMap, 0, sizeof(stH265QpMap));
            pstRcParam->stH265QpMap.u32FrameRate = 30;
            pstRcParam->stH265QpMap.u32IntraPeriod = 30;
            pstRcParam->stH265QpMap.u32QpMapArrayCount =
                ((vencparam->width + 0x0f)&~0x0f) / 16
                 * ((vencparam->height + 0x0f)&~0x0f) / 16;
            pstRcParam->stH265QpMap.u32QpMapArray = 
                reinterpret_cast<unsigned char *>(malloc(pstRcParam->stH265QpMap.u32QpMapArrayCount));
            break;
        }
        default:
            break;
        }
    }

    ret = HB_VENC_SetRcParam(vencparam->veChn, pstRcParam);
    if (ret != 0) {
        printf("HB_VENC_SetRcParam failed.\n");
        return -1;
    }

    ret = HB_VENC_SetChnAttr(vencparam->veChn, &vencChnAttr); // config
    if (ret != 0) {
        printf("HB_VENC_SetChnAttr failed\n");
        return -1;
    }
    return ret;
}

int hb_venc_start(int channel)
{
	int ret = 0;
	VENC_RECV_PIC_PARAM_S pstRecvParam;
	pstRecvParam.s32RecvPicNum = 0; // unchangable

	ret = HB_VENC_StartRecvFrame(channel, &pstRecvParam);
	if (ret != 0)
	{
		printf("HB_VENC_StartRecvFrame failed\n");
		return -1;
	}
	return ret;
}

int hb_venc_input(int channel, VIDEO_FRAME_S *pstFrame)
{
	int ret = 0;
	ret = HB_VENC_SendFrame(channel, pstFrame, 3000);
	if (ret < 0) {
		printf("HB_VENC_SendFrame error!!!\n");
	}
	return ret;
}

int hb_venc_get_output(int channel, VIDEO_STREAM_S *pstStream)
{
	int ret = 0;
	ret = HB_VENC_GetStream(channel, pstStream, 2000);
	if (ret < 0) {
		printf("HB_VENC_GetStream error!!!\n");
	}
	return ret;
}

int hb_venc_output_release(int channel, VIDEO_STREAM_S *pstStream)
{
	int ret = 0;
	ret = HB_VENC_ReleaseStream(channel, pstStream);
	if (ret != 0)
	{
		printf("HB_VENC_ReleaseStream failed\n");
		return ret;
	}
	return ret;
}

int hb_venc_stop(int channel)
{
	int ret = 0;
	ret = HB_VENC_StopRecvFrame(channel);
	if (ret != 0)
	{
		printf("HB_VENC_StopRecvFrame failed\n");
		return ret;
	}
	return ret;
}

int hb_venc_deinit(int channel)
{
	int ret = 0;
	ret = HB_VENC_DestroyChn(channel);
	if (ret != 0)
	{
		printf("HB_VENC_DestroyChn failed\n");
		return -1;
	}
	return ret;
}

void venc_thread(void *param) {
    int ret;
    hb_vio_buffer_t out_buf;
    struct timeval ts0, ts1;
    int pts = 0, idx = 0;
    uint32_t last_frame_id = -1;
    time_t start, last = time(NULL);
    VIDEO_FRAME_S pstFrame;
    VIDEO_STREAM_S pstStream;
    memset(&pstFrame, 0, sizeof(VIDEO_FRAME_S));
    memset(&pstStream, 0, sizeof(VIDEO_STREAM_S));
    vencParam_t *vencparam = reinterpret_cast<vencParam_t *>(param);
    pstFrame.stVFrame.width = vencparam->width;
    pstFrame.stVFrame.height = vencparam->height;
    pstFrame.stVFrame.size = vencparam->width * vencparam->height * 3 / 2;
    pstFrame.stVFrame.pix_format = HB_PIXEL_FORMAT_NV12;
    auto *pvio_image = reinterpret_cast<pym_buffer_t *>(
        std::calloc(1, sizeof(pym_buffer_t)));
    if (pvio_image == nullptr) {
        printf("malloc failed at chn%d\n", vencparam->veChn);
        return;
    }

    printf("venc_thread%d entered,vencparam->bind_flag =%d\n", vencparam->veChn,vencparam->bind_flag);
	while (running) {
        memset(pvio_image, 0, sizeof(pym_buffer_t));
        if (!vencparam->bind_flag) {
            ret = HB_VPS_GetChnFrame(vencparam->vpsGrp, vencparam->vpsChn,
                reinterpret_cast<void *>(pvio_image), 2000);
            if (ret != 0) {
                usleep(10000);
                printf("HB_VPS_GetChnFrame failed 11 \n");
                continue;
            }
            pstFrame.stVFrame.phy_ptr[0] = pvio_image->pym[0].paddr[0];
            pstFrame.stVFrame.phy_ptr[1] = pvio_image->pym[0].paddr[1];
            pstFrame.stVFrame.vir_ptr[0] = pvio_image->pym[0].addr[0];
            pstFrame.stVFrame.vir_ptr[1] = pvio_image->pym[0].addr[1];
            pstFrame.stVFrame.pts = pts;
            if (vencparam->need_dump) {
                dump_pym_to_files(pvio_image, vencparam->veChn, 0);
            }
            HB_VENC_SendFrame(vencparam->veChn, &pstFrame, 0);
            ret = HB_VENC_GetStream(vencparam->veChn, &pstStream, 2000);
            gettimeofday(&ts1, NULL);
            if (ret < 0) {
                printf("HB_VENC_GetStream error!!!\n");
            } else {
                start = time(NULL);
                idx++;
                if (start > last) {
                    gettimeofday(&ts1, NULL);
                    printf("[%ld.%06ld]venc no-bind chn %d fps %d\n",
                        ts1.tv_sec, ts1.tv_usec, vencparam->veChn, idx);
                    last = start;
                    idx = 0;
                }
                //if (vencparam->need_rtsp) {
                //    hb_h264_data_push(0, pstStream.pstPack.src_idx,
                //        reinterpret_cast<unsigned char*>(pstStream.pstPack.vir_ptr),
                //        pstStream.pstPack.size);
                //}
                HB_VENC_ReleaseStream(vencparam->veChn, &pstStream);
            }
            HB_VPS_ReleaseChnFrame(vencparam->vpsGrp, vencparam->vpsChn,
                reinterpret_cast<void *>(pvio_image));
        } else {
            ret = HB_VENC_GetStream(vencparam->veChn, &pstStream, 2000);
            gettimeofday(&ts1, NULL);
            if (ret < 0) {
                printf("HB_VENC_GetStream error!!!\n");
            } else {
                start = time(NULL);
                idx++;
                if (start > last) {
                    gettimeofday(&ts1, NULL);
                    printf("[%ld.%06ld]venc bind chn %d fps %d\n",
                                ts1.tv_sec, ts1.tv_usec, vencparam->veChn, idx);
                    last = start;
                    idx = 0;
                }
				HB_VENC_ReleaseStream(vencparam->veChn, &pstStream);
            }
        }
        pts++;
	}
	printf("venc_thread exit\n");
}
