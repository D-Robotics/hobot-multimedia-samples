/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_VIO_SYS_H_
#define INCLUDE_VIO_SYS_H_
#include "utils/utils.h"
#include "vio/vio_cfg.h"

int hb_vp_init();
int hb_vp_alloc(vp_param_t *param);
int hb_vp_free(vp_param_t *param);
int hb_vp_deinit();
int hb_vps_bind_venc(int vpsGrp, int vpsChn, int vencChn);
int hb_vps_unbind_venc(int vpsGrp, int vpsChn, int vencChn);
int hb_vin_bind_vps(int vin_group, int vps_group, int vps_channel,
        vio_cfg_t cfg);
int hb_vin_unbind_vps(int vin_group, int vps_group, vio_cfg_t cfg);
int hb_vps_bind_vo(int vpsGrp, int vpsChn, int voChn);
int hb_vdec_bind_venc(int vdecChn, int vencChn);
int hb_vdec_bind_vps(int vdecChn, int vpsGrp, int vpsChn);
int hb_vps_bind_vps(int src_Grp, int src_Chn, int dts_Grp);
int hb_vps_unbind_vps(int src_Grp, int src_Chn, int dts_Grp);
#endif //INCLUDE_VIO_SYS_H_