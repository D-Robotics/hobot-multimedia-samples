/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_VIO_CFG_TYPE_H_
#define INCLUDE_VIO_CFG_TYPE_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "hb_vps_api.h"

#define MAX_CAM_NUM (6)
#define MAX_MIPIID_NUM (4)
#define MAX_GRP_NUM (MAX_CAM_NUM)
#define MAX_CHN_NUM (7)

typedef struct vio_cfg_s {
    /* 1. sensor config */
     int need_cam;
     int cam_num;
     int vps_grp_num;
     int vps_dump;
     int need_vo;
     int need_rtsp;
     int testpattern_fps;
     int Again_ispDgain;
     int sensorAgain;
     int ispDgain;
     int sleepTime;
     int AEM1;
	 int tailWeight;
	 int autoTimeCount;
	 int manualTimeCount;
	 int manualAeM1;
	 int mjpgQp;
     int sensor_id[MAX_CAM_NUM];
     int sensor_port[MAX_CAM_NUM];
     int mipi_idx[MAX_MIPIID_NUM];
     int i2c_bus[MAX_CAM_NUM];
     int serdes_index[MAX_CAM_NUM];
     int serdes_port[MAX_CAM_NUM];
     int temper_mode[MAX_CAM_NUM];
     int isp_out_buf_num[MAX_CAM_NUM];
     int gpio_num[8];
     int gpio_pin[8];
     int gpio_level[8];
    /* 2. vps config */
     int fb_width[MAX_GRP_NUM];
     int fb_height[MAX_GRP_NUM];
     int fb_buf_num[MAX_GRP_NUM];
     int vin_fd[MAX_GRP_NUM];
     int vin_vps_mode[MAX_GRP_NUM];
     int need_clk[MAX_GRP_NUM];
     int need_md[MAX_GRP_NUM];
     int need_chnfd[MAX_GRP_NUM];
     int need_dis[MAX_GRP_NUM];
     int grp_gdc_en[MAX_GRP_NUM];
     int grp_rotate[MAX_GRP_NUM];
     int grp_width[MAX_GRP_NUM];
     int grp_height[MAX_GRP_NUM];
     int dol2_vc_num[MAX_GRP_NUM];
     int grp_frame_depth[MAX_GRP_NUM];
     int chn_num[MAX_GRP_NUM];
    /* 3. chn config */
     int chn_index[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_pym_en[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_scale_en[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_width[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_height[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_crop_en[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_crop_width[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_crop_height[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_crop_x[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_crop_y[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_frame_depth[MAX_GRP_NUM][MAX_CHN_NUM];
     int chn_gdc_en[MAX_GRP_NUM][MAX_CHN_NUM];
     VPS_PYM_CHN_ATTR_S chn_pym_cfg[MAX_GRP_NUM][MAX_CHN_NUM];
} vio_cfg_t;

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  // INCLUDE_IOT_CFG_TYPE_H_
