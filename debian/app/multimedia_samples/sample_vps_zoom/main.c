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

void dump_pym_to_files(pym_buffer_t *pvio_image, int grp, int chn, int layer) {
    static int count = 0;
    char file_name[100] = {0};
    snprintf(file_name,
        sizeof(file_name),
        "grp%d_chn%d_pym_layer_DS%d_%d_%d_%d.yuv",
        grp,
        chn, 
        layer,
        pvio_image->pym[layer].width,
        pvio_image->pym[layer].height, count);
    printf("grp%d_chn%d_pym_layer_DS%d_%d_%d_%d.yuv, strid:%d\n",
        grp,
        chn, 
        layer,
        pvio_image->pym[layer].width,
        pvio_image->pym[layer].height, count, pvio_image->pym[layer].stride_size);
    dumpToFile2plane(
        file_name,
        pvio_image->pym[layer].addr[0],
        pvio_image->pym[layer].addr[1],
        pvio_image->pym[layer].width * pvio_image->pym[layer].height,
        pvio_image->pym[layer].width * pvio_image->pym[layer].height / 2);    
    count++;
}

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
  
    printf("buf:y: vaddr = 0x%s paddr = 0x%lx; uv: vaddr = 0x%s, paddr = 0x%lx\n",
	buffer->img_addr.addr[0], buffer->img_addr.paddr[0],
	buffer->img_addr.addr[1], buffer->img_addr.paddr[1]);
  
    return 0;
}

int venc_save_h264(FILE* fpH264File, VIDEO_STREAM_S *pstStream)
{
	fwrite(pstStream->pstPack.vir_ptr, pstStream->pstPack.size, 1, fpH264File);
	fflush(fpH264File);

    return 0;
}

int venc_save_stream(PAYLOAD_TYPE_E enType, FILE *pFd, VIDEO_STREAM_S *pstStream)
{
    int ret = -1;
    if (PT_H264 == enType){
        ret = venc_save_h264(pFd, pstStream);
    }
    
    return ret;
}

int venc_save_jpeg(char *filename, char *srcBuf, unsigned int size)
{
    FILE *fd = NULL; 
    fd = fopen(filename, "w+"); 
    if (fd == NULL) {
        printf("open(%s) fail", filename);
        return -1;
    }
  
    fflush(stdout);
    fwrite(srcBuf, 1, size, fd);
    fflush(fd); 
    if (fd) {
        fclose(fd);
    }
    printf("DEBUG:save jpeg(%s, size(%d) is successed!!\n", filename, size);
  
    return 0;
}

int find_up_layer(int x, int y, int w, int h, int max_width, int max_height, int grp_id, int chn_id) 
{
    int ret = -1;

    // 判断分辨率是否符合放大要求
    if (w < max_width / 4 || h < max_height / 4 || x + w > max_width || y + h > max_height) {
        return -1;
    }

    // printf("FindUpLayer (%d, %d) (%d, %d)\n", x, y, w, h);
    if (w >= max_width * 2 / 3 && h >= max_height * 2 / 3) {
        VPS_CROP_INFO_S crop_info;
        ret = HB_VPS_GetChnCrop(grp_id, chn_id, &crop_info);
        crop_info.en = 1;
        crop_info.cropRect.x = x;
        crop_info.cropRect.y = y;
        crop_info.cropRect.width = w;
        crop_info.cropRect.height = h;
        // printf("crop info : (x %d y %d w %d h %d) grp id %d, chn id %d\n", 
        //         crop_info.cropRect.x,
        //         crop_info.cropRect.y,
        //         crop_info.cropRect.width,
        //         crop_info.cropRect.height,
        //         grp_id, chn_id);
        ret = HB_VPS_SetChnCrop(grp_id, chn_id, &crop_info);
        return 100;
    }

    // 寻找放大区间
    int up_factor[6] = {50, 40, 32, 25, 20, 16};
    int layer = -1;
    for (int i = 0; i < 6; i++) {
        if (w >= ((max_width * up_factor[i] / 64) & (~0x1))) {
            layer = i - 1;
            break;
        }
    }

    if (layer < 0) {
        return -1;
    }

    // 将该区域放大到金字塔对应的区域大小，计算新的要放大区域大小
    int ipu_roi_width = w * 64 / up_factor[layer];
    ipu_roi_width &= ~0x1;
    int ipu_roi_height = h * 64 / up_factor[layer];
    ipu_roi_height &= ~0x1;

    if (ipu_roi_width > max_width) {
        ipu_roi_width = max_width;
    }
    ipu_roi_height = ipu_roi_height > max_height ? max_height : ipu_roi_height;

    int roi_x = x + w / 2 - ipu_roi_width / 2;
    int roi_y = y + h / 2 - ipu_roi_height / 2;
    int roi_w = ipu_roi_width;
    int roi_h = ipu_roi_height;
    // printf("roi info (%d, %d) (%d, %d)\n", ipu_roi_width, ipu_roi_height, roi_x, roi_y);

    if (roi_x < 0)
        roi_x = 0;
    if (roi_y < 0)
        roi_y = 0;
    if (roi_x + roi_w > max_width)
        roi_x = max_width - roi_w;
    if (roi_y + roi_h > max_height)
        roi_y = max_height - roi_h;

    roi_x &= ~0x1;
    roi_y &= ~0x1;
    // printf("crop info (%d, %d) (%d, %d)\n", roi_x, roi_y, roi_w, roi_h);

    VPS_CROP_INFO_S crop_info;
    ret = HB_VPS_GetChnCrop(grp_id, chn_id, &crop_info);
    crop_info.en = 1;
    crop_info.cropRect.x = roi_x;
    crop_info.cropRect.y = roi_y;
    crop_info.cropRect.width = roi_w;
    crop_info.cropRect.height = roi_h;
    ret = HB_VPS_SetChnCrop(grp_id, chn_id, &crop_info);

    VPS_PYM_CHN_ATTR_S pym_chn_attr;
    ret = HB_VPS_GetPymChnAttr(grp_id, chn_id, &pym_chn_attr);
    int pym_roi_x = max_width * (x - roi_x) / ipu_roi_width - 1;
    int pym_roi_y = max_height * (y - roi_y) / ipu_roi_height - 3;
    if (pym_roi_x < 0)
        pym_roi_x = 0;
    if (pym_roi_y < 0)
        pym_roi_y = 0;

    pym_roi_x &= ~0x1;
    pym_roi_y &= ~0x1;

    pym_chn_attr.us_info[layer].roi_x = pym_roi_x;
    pym_chn_attr.us_info[layer].roi_y = pym_roi_y;
    // printf("pym roi (%d, %d) (%d, %d)\n", pym_chn_attr.us_info[layer].roi_x,
    //        pym_chn_attr.us_info[layer].roi_y,
    //        pym_chn_attr.us_info[layer].roi_width,
    //        pym_chn_attr.us_info[layer].roi_height);

    ret = HB_VPS_SetPymChnAttr(grp_id, chn_id, &pym_chn_attr);
    for (int i = 0; i < 6; i++) {
        ret = HB_VPS_ChangePymUs(grp_id, i, layer == i);
        if (ret != 0) {
            printf("HB_VPS_ReleaseChnFrame Failed. ret = %d\n", ret);
        }
    }

    return layer;
}

int main()
{
    VPS_GRP_ATTR_S stVpsGrpAttr_0;
    VPS_GRP_ATTR_S stVpsGrpAttr_1;
    VPS_CHN_ATTR_S stVpsChnAttr_0_1;
    VPS_CHN_ATTR_S stVpsChnAttr_1_5;
    VPS_PYM_CHN_ATTR_S stVpsPymChnAttr_1_5;
    VENC_CHN_ATTR_S stVencChnAttr_0;
    VENC_RC_ATTR_S *pstRcParam;
    PIXEL_FORMAT_E pixFmt = HB_PIXEL_FORMAT_NV12;
    VENC_RECV_PIC_PARAM_S stVencRecvPicParam;
    hb_vio_buffer_t feedback_buf;
    pym_buffer_t grp_1_chn_5_pym_out_buf;
    int ret = 0;
    uint32_t size_y, size_uv;
    int img_in_fd, img_out_fd;
    time_t now, start_time, end_time;
    char file_name[128];
    int frame_index = 0;
    VIDEO_FRAME_S pstFrame;
    VIDEO_STREAM_S pstStream;
    FILE *pFile[3];
    struct HB_SYS_MOD_S src_mod, dst_mod;
    int max_width = 1920;
    int max_height = 1080;
    int test_w = 1920;
    int test_h = 1080;
    int x = 0, y = 0, w = 0, h = 0;
    int i = 0;
    int up_layer;

    memset(&pstFrame, 0, sizeof(VIDEO_FRAME_S));
    memset(&pstStream, 0, sizeof(VIDEO_STREAM_S));

    memset(&stVpsGrpAttr_0, 0, sizeof(VPS_GRP_ATTR_S));
    stVpsGrpAttr_0.maxW = max_width;
    stVpsGrpAttr_0.maxH = max_height;
    HB_VPS_CreateGrp(0, &stVpsGrpAttr_0);

    HB_SYS_SetVINVPSMode(0, VIN_OFFLINE_VPS_OFFINE);

    memset(&stVpsChnAttr_0_1, 0, sizeof(VPS_CHN_ATTR_S));
    stVpsChnAttr_0_1.enScale = 1;
    stVpsChnAttr_0_1.width = max_width;
    stVpsChnAttr_0_1.height = max_height;
    stVpsChnAttr_0_1.frameDepth = 8;
    HB_VPS_SetChnAttr(0, 1, &stVpsChnAttr_0_1);

    memset(&stVpsGrpAttr_1, 0, sizeof(VPS_GRP_ATTR_S));
    stVpsGrpAttr_1.maxW = max_width;
    stVpsGrpAttr_1.maxH = max_height;
    HB_VPS_CreateGrp(1, &stVpsGrpAttr_1);

    HB_SYS_SetVINVPSMode(1, VIN_OFFLINE_VPS_OFFINE);

    memset(&stVpsChnAttr_1_5, 0, sizeof(VPS_CHN_ATTR_S));
    stVpsChnAttr_1_5.enScale = 1;
    stVpsChnAttr_1_5.width = max_width;
    stVpsChnAttr_1_5.height = max_height;
    stVpsChnAttr_1_5.frameDepth = 8;
    HB_VPS_SetChnAttr(1, 5, &stVpsChnAttr_1_5);

    memset(&stVpsPymChnAttr_1_5, 0, sizeof(VPS_PYM_CHN_ATTR_S));
    stVpsPymChnAttr_1_5.timeout = 2000;
    stVpsPymChnAttr_1_5.ds_layer_en = 4;
    stVpsPymChnAttr_1_5.us_layer_en = 6;
    stVpsPymChnAttr_1_5.frame_id = 0;
    stVpsPymChnAttr_1_5.frameDepth = 1;
    stVpsPymChnAttr_1_5.us_info[0].factor = 50; // 1.28
    stVpsPymChnAttr_1_5.us_info[0].roi_width = (max_width * 50 / 64 + 2) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[0].roi_height = (max_height * 50 / 64 + 3) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[1].factor = 40; // 1.6
    stVpsPymChnAttr_1_5.us_info[1].roi_width = (max_width * 40 / 64 + 2) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[1].roi_height = (max_height * 40 / 64 + 3) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[2].factor = 32; // 2
    stVpsPymChnAttr_1_5.us_info[2].roi_width = (max_width * 32 / 64 + 2) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[2].roi_height = (max_height * 32 / 64 + 2) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[3].factor = 25; // 2.56
    stVpsPymChnAttr_1_5.us_info[3].roi_width = (max_width * 25 / 64 + 2) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[3].roi_height = ((max_height + 2) * 25 / 64 + 3) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[4].factor = 20; // 3.2
    stVpsPymChnAttr_1_5.us_info[4].roi_width = (max_width * 20 / 64 + 2) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[4].roi_height = ((max_height + 2) * 20 / 64 + 3) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[5].factor = 16; // 4
    stVpsPymChnAttr_1_5.us_info[5].roi_width = (max_width * 16 / 64 + 2) & (~0x1);
    stVpsPymChnAttr_1_5.us_info[5].roi_height = (max_height * 16 / 64 + 2) & (~0x1);
    HB_VPS_SetPymChnAttr(1, 5, &stVpsPymChnAttr_1_5);
    for (i = 0; i < 6; i++) {
    ret = HB_VPS_ChangePymUs(1, i, 0);
    if (ret != 0) {
        printf("HB_VPS_ChangePymUs Failed. ret = %d\n", ret);
        }
    }

    src_mod.enModId = HB_ID_VPS;
    src_mod.s32DevId = 0;  // grp 0
    src_mod.s32ChnId = 1;
    dst_mod.enModId = HB_ID_VPS;
    dst_mod.s32DevId = 1;  // grp 1
    dst_mod.s32ChnId = 0;
    HB_SYS_Bind(&src_mod, &dst_mod);

    HB_VENC_Module_Init();

    memset(&stVencChnAttr_0, 0, sizeof(VENC_CHN_ATTR_S));
    stVencChnAttr_0.stVencAttr.enType = PT_H264;
    stVencChnAttr_0.stVencAttr.u32PicWidth = max_width;
    stVencChnAttr_0.stVencAttr.u32PicHeight = max_height;
    stVencChnAttr_0.stVencAttr.enMirrorFlip = DIRECTION_NONE;
    stVencChnAttr_0.stVencAttr.enRotation = CODEC_ROTATION_0;
    stVencChnAttr_0.stVencAttr.stCropCfg.bEnable = HB_FALSE;
    stVencChnAttr_0.stVencAttr.bEnableUserPts = HB_TRUE;
    stVencChnAttr_0.stVencAttr.s32BufJoint = 0;
    stVencChnAttr_0.stVencAttr.s32BufJointSize = 8000000;
    stVencChnAttr_0.stVencAttr.enPixelFormat = pixFmt;
    stVencChnAttr_0.stVencAttr.u32BitStreamBufferCount = 3;
    stVencChnAttr_0.stVencAttr.u32FrameBufferCount = 3;
    stVencChnAttr_0.stGopAttr.u32GopPresetIdx = 6;
    stVencChnAttr_0.stGopAttr.s32DecodingRefreshType = 2;
    stVencChnAttr_0.stVencAttr.vlc_buf_size = 4*1024*1024;
    stVencChnAttr_0.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
    stVencChnAttr_0.stVencAttr.bExternalFreamBuffer = HB_TRUE;
    stVencChnAttr_0.stRcAttr.stH264Vbr.bQpMapEnable = HB_TRUE;
    stVencChnAttr_0.stRcAttr.stH264Vbr.u32IntraQp = 20;
    stVencChnAttr_0.stRcAttr.stH264Vbr.u32IntraPeriod = 20;
    stVencChnAttr_0.stRcAttr.stH264Vbr.u32FrameRate = 25;
    stVencChnAttr_0.stVencAttr.stAttrH264.h264_profile = HB_H264_PROFILE_MP;
    stVencChnAttr_0.stVencAttr.stAttrH264.h264_level = HB_H264_LEVEL1;
    HB_VENC_CreateChn(0, &stVencChnAttr_0);

    pstRcParam = &(stVencChnAttr_0.stRcAttr);
    HB_VENC_GetRcParam(0, pstRcParam);
    pstRcParam->stH264Vbr.bQpMapEnable = HB_FALSE;
    pstRcParam->stH264Vbr.u32FrameRate = 30;
    pstRcParam->stH264Vbr.u32IntraPeriod = 30;
    pstRcParam->stH264Vbr.u32IntraQp = 35;
    HB_VENC_SetChnAttr(0, &stVencChnAttr_0);

    HB_VPS_EnableChn(0, 1);
    HB_VPS_StartGrp(0);

    HB_VPS_EnableChn(1, 5);
    HB_VPS_StartGrp(1);

    stVencRecvPicParam.s32RecvPicNum = 0;
    HB_VENC_StartRecvFrame(0, &stVencRecvPicParam);

    memset(&feedback_buf, 0, sizeof(hb_vio_buffer_t));
    size_y = max_width * max_height;
    size_uv = size_y / 2;
    prepare_user_buf(&feedback_buf, size_y, size_uv);
    feedback_buf.img_info.planeCount = 2;
    feedback_buf.img_info.img_format = 8;
    feedback_buf.img_addr.width = max_width;
    feedback_buf.img_addr.height = max_height;
    feedback_buf.img_addr.stride_size = max_width;
  
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
        dumpToFile2plane("in_image.yuv", feedback_buf.img_addr.addr[0], 
        feedback_buf.img_addr.addr[1], size_y, size_uv);
    }
    usleep(100 * 1000);

    pFile[0] = fopen("vps0_chn1_vps1_chn5_venc0.h264", "wb");

    start_time = time(NULL);
    end_time = start_time + 20;

    while (1) {
        if (w <= 1920 && h <= 1080) {
            x = 0;
            y = 0;
            w = max_width / 4 + i * 2;
            h = w * 9 / 16 + 1;
            h &= ~0x1;
            i = i + 10;
        } else {
            w = max_width / 4;
            h = w * 9 / 16 + 1;
            i = 0;
        }

        up_layer = find_up_layer(x, y, w, h, max_width, max_height, 1, 5);
        if (up_layer < 0) {
            continue;
        }

        // send frame to vps grp 0
        printf("send frame %d to vps grp 0\n", frame_index++);
        HB_VPS_SendFrame(0, &feedback_buf, 1000);
        // printf("feedback 0x%x 0x%x\n", 
        //     feedback_buf.img_addr.addr[0],
        //     feedback_buf.img_addr.addr[1]);

        HB_VPS_GetChnFrame_Cond(1, 5, &grp_1_chn_5_pym_out_buf, 2000, 0);
        
        if (up_layer > 5) {
            if (g_dump) {
                dump_pym_to_files(&grp_1_chn_5_pym_out_buf, 1, 5, 0);
            }
            pstFrame.stVFrame.phy_ptr[0] = grp_1_chn_5_pym_out_buf.pym[0].paddr[0];
            pstFrame.stVFrame.phy_ptr[1] = grp_1_chn_5_pym_out_buf.pym[0].paddr[1];
            pstFrame.stVFrame.vir_ptr[0] = grp_1_chn_5_pym_out_buf.pym[0].addr[0];
            pstFrame.stVFrame.vir_ptr[1] = grp_1_chn_5_pym_out_buf.pym[0].addr[1];
            // printf("0x%x 0x%x 0x%x 0x%x\n",
            //     pstFrame.stVFrame.phy_ptr[0],
            //     pstFrame.stVFrame.phy_ptr[1],
            //     pstFrame.stVFrame.vir_ptr[0],
            //     pstFrame.stVFrame.vir_ptr[1]);
        } else {
            pstFrame.stVFrame.phy_ptr[0] = grp_1_chn_5_pym_out_buf.us[up_layer].paddr[0];
            pstFrame.stVFrame.phy_ptr[1] = grp_1_chn_5_pym_out_buf.us[up_layer].paddr[1];
            pstFrame.stVFrame.vir_ptr[0] = grp_1_chn_5_pym_out_buf.us[up_layer].addr[0];
            pstFrame.stVFrame.vir_ptr[1] = grp_1_chn_5_pym_out_buf.us[up_layer].addr[1];
            // printf("0x%x 0x%x 0x%x 0x%x\n",
            //     pstFrame.stVFrame.phy_ptr[0],
            //     pstFrame.stVFrame.phy_ptr[1],
            //     pstFrame.stVFrame.vir_ptr[0],
            //     pstFrame.stVFrame.vir_ptr[1]);
        }

        HB_VENC_SendFrame(0, &pstFrame, 0);
        HB_VENC_GetStream(0, &pstStream, 2000);
        venc_save_stream(PT_H264, pFile[0], &pstStream);
        HB_VENC_ReleaseStream(0, &pstStream);
        HB_VPS_ReleaseChnFrame(1, 5, &grp_1_chn_5_pym_out_buf);
        
        now = time(NULL);
        if (now > end_time) {
            break;
        }
    }

    fclose(pFile[0]);

    HB_SYS_UnBind(&src_mod, &dst_mod);
    HB_VPS_DisableChn(0, 1);
    HB_VPS_DisableChn(1, 5);
    HB_VPS_StopGrp(1);
    HB_VPS_StopGrp(0);
    HB_VPS_DestroyGrp(1);
    HB_VPS_DestroyGrp(0);
    
    HB_VENC_StopRecvFrame(0);
    HB_VENC_DestroyChn(0);
    HB_VENC_Module_Uninit();

    return 0;
}