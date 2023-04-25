/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#include <cstddef>
#include "stdint.h"
#include <stdio.h>
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "hb_sys.h"
#include "hb_vp_api.h"
#ifdef __cplusplus
}
#endif
#include "vio/vio_log.h"
#include "vio_sys.h"

int hb_vp_init() {
    VP_CONFIG_S struVpConf;
    memset(&struVpConf, 0x00, sizeof(VP_CONFIG_S));
    struVpConf.u32MaxPoolCnt = 32;
    HB_VP_SetConfig(&struVpConf);

    auto ret = HB_VP_Init();
    if (!ret) {
        printf("hb_vp_init success\n");
    } else {
        printf("hb_vp_init failed, ret: %d\n", ret);
    }
    return ret;
}

int hb_vp_alloc(vp_param_t *param) {
    auto vp_param = reinterpret_cast<vp_param_t *>(param);

    for (auto i = 0; i < vp_param->mmz_cnt; i++) {
        auto ret = HB_SYS_AllocCached(&vp_param->mmz_paddr[i],
            (void **)&vp_param->mmz_vaddr[i], vp_param->mmz_size);
        if (!ret) {
            printf("mmzAlloc paddr = 0x%lx, vaddr = %p i = %d \n",
                    vp_param->mmz_paddr[i], vp_param->mmz_vaddr[i], i);
        } else {
            printf("hb_vp_alloc failed, ret: %d\n", ret);
            return -1;
        }
    }
    return 0;
}

int hb_vp_free(vp_param_t *param) {
    auto vp_param = reinterpret_cast<vp_param_t *>(param);
    for (auto i = 0; i < vp_param->mmz_cnt; i++) {
        auto ret = HB_SYS_Free(vp_param->mmz_paddr[i], vp_param->mmz_vaddr[i]);
        if (ret == 0) {
            printf("mmzFree paddr = 0x%lx, vaddr = %p i = %d \n",
                vp_param->mmz_paddr[i], vp_param->mmz_vaddr[i], i);
        }
    }
    return 0;
}

int hb_vp_deinit() {
    auto ret = HB_VP_Exit();
    if (!ret) {
        printf("hb_vp_deinit success\n");
    } else {
        printf("hb_vp_deinit failed, ret: %d\n", ret);
    }
    return ret;
}

int hb_vin_bind_vps(int vin_group, int vps_group, int vps_channel, vio_cfg_t cfg) {
	int ret = 0;
	struct HB_SYS_MOD_S src_mod, dst_mod;
	src_mod.enModId = HB_ID_VIN;
	src_mod.s32DevId = vin_group;
    auto mode = cfg.vin_vps_mode[0];
	if (mode == VIN_ONLINE_VPS_ONLINE ||
		mode == VIN_OFFLINE_VPS_ONLINE||
		mode == VIN_SIF_ONLINE_DDR_ISP_ONLINE_VPS_ONLINE||
		mode == VIN_SIF_OFFLINE_ISP_OFFLINE_VPS_ONLINE ||
		mode == VIN_FEEDBACK_ISP_ONLINE_VPS_ONLINE ||
		mode == VIN_SIF_VPS_ONLINE)
		src_mod.s32ChnId = 1;
	else
		src_mod.s32ChnId = 0;
	dst_mod.enModId = HB_ID_VPS;
	dst_mod.s32DevId = vps_group;
	dst_mod.s32ChnId = vps_channel;
	ret = HB_SYS_Bind(&src_mod, &dst_mod);
	if (ret != 0)
		pr_err("hb_vin_bind_vps failed\n");

    return ret;
}

int hb_vin_unbind_vps(int vin_group, int vps_group, vio_cfg_t cfg) {
    int ret = 0;
    struct HB_SYS_MOD_S src_mod, dst_mod;
    src_mod.enModId = HB_ID_VIN;
    src_mod.s32DevId = vin_group;
    auto mode = cfg.vin_vps_mode[0];
    if (mode == VIN_ONLINE_VPS_ONLINE ||
        mode == VIN_OFFLINE_VPS_ONLINE||
        mode == VIN_SIF_ONLINE_DDR_ISP_ONLINE_VPS_ONLINE||
        mode == VIN_SIF_OFFLINE_ISP_OFFLINE_VPS_ONLINE ||
        mode == VIN_FEEDBACK_ISP_ONLINE_VPS_ONLINE ||
        mode == VIN_SIF_VPS_ONLINE)
        src_mod.s32ChnId = 1;
    else
        src_mod.s32ChnId = 0;
    dst_mod.enModId = HB_ID_VPS;
    dst_mod.s32DevId = vps_group;
    dst_mod.s32ChnId = 0;
    ret = HB_SYS_UnBind(&src_mod, &dst_mod);
    if (ret != 0)
        pr_err("hb_vin_bind_vps failed\n");

    return ret;
}

int hb_vps_bind_venc(int vpsGrp, int vpsChn, int vencChn) {
	int ret = 0;
	struct HB_SYS_MOD_S src_mod, dst_mod;

	src_mod.enModId = HB_ID_VPS;
	src_mod.s32DevId = vpsGrp;
	src_mod.s32ChnId = vpsChn;
	dst_mod.enModId = HB_ID_VENC;
	dst_mod.s32DevId = vencChn;
	dst_mod.s32ChnId = 0;
	ret = HB_SYS_Bind(&src_mod, &dst_mod);
	if (ret != 0)
		pr_err("hb_vps_bind_venc failed\n");

	return ret;
}

int hb_vps_bind_vo(int vpsGrp, int vpsChn, int voChn) {
	int ret = 0;
	struct HB_SYS_MOD_S src_mod, dst_mod;

	src_mod.enModId = HB_ID_VPS;
	src_mod.s32DevId = vpsGrp;
	src_mod.s32ChnId = vpsChn;
	dst_mod.enModId = HB_ID_VOT;
	dst_mod.s32DevId = voChn;
	dst_mod.s32ChnId = voChn;
	ret = HB_SYS_Bind(&src_mod, &dst_mod);
	if (ret != 0)
		pr_err("hb_vps_bind_venc failed\n");

	return ret;
}

int hb_vps_bind_vps(int src_Grp, int src_Chn, int dts_Grp) {
	int ret = 0;
	struct HB_SYS_MOD_S src_mod, dst_mod;

	src_mod.enModId = HB_ID_VPS;
	src_mod.s32DevId = src_Grp;
	src_mod.s32ChnId = src_Chn;
	dst_mod.enModId = HB_ID_VPS;
	dst_mod.s32DevId = dts_Grp;
	dst_mod.s32ChnId = 0;
	ret = HB_SYS_Bind(&src_mod, &dst_mod);
	if (ret != 0)
		pr_err("hb_vps_bind_vps failed, src:%d, dts:%d\n", src_Grp, dts_Grp);

	return ret;
}

int hb_vps_unbind_vps(int src_Grp, int src_Chn, int dts_Grp) {
	int ret = 0;
	struct HB_SYS_MOD_S src_mod, dst_mod;

	src_mod.enModId = HB_ID_VPS;
	src_mod.s32DevId = src_Grp;
	src_mod.s32ChnId = src_Chn;
	dst_mod.enModId = HB_ID_VPS;
	dst_mod.s32DevId = dts_Grp;
	dst_mod.s32ChnId = 0;
	ret = HB_SYS_UnBind(&src_mod, &dst_mod);
	if (ret != 0)
		pr_err("hb_vps_unbind_vps failed, src:%d, dts:%d\n", src_Grp, dts_Grp);

	return ret;
}

int hb_vps_unbind_venc(int vpsGrp, int vpsChn, int vencChn)
{
	int ret = 0;
	struct HB_SYS_MOD_S src_mod, dst_mod;

	src_mod.enModId = HB_ID_VPS;
	src_mod.s32DevId = vpsGrp;
	src_mod.s32ChnId = vpsChn;
	dst_mod.enModId = HB_ID_VENC;
	dst_mod.s32DevId = vencChn;
	dst_mod.s32ChnId = 0;
	ret = HB_SYS_UnBind(&src_mod, &dst_mod);
	if (ret != 0)
		pr_err("HB_SYS_Bind failed\n");

	return ret;
}

int hb_vdec_bind_venc(int vdecChn, int vencChn) {
	int ret = 0;
	struct HB_SYS_MOD_S src_mod, dst_mod;

	src_mod.enModId = HB_ID_VDEC;
	src_mod.s32DevId = vdecChn;
	src_mod.s32ChnId = 0;
	dst_mod.enModId = HB_ID_VENC;
	dst_mod.s32DevId = vencChn;
	dst_mod.s32ChnId = 0;
	ret = HB_SYS_Bind(&src_mod, &dst_mod);
	if (ret)
		pr_err("hb_vdec_bind_venc failed\n");

	return ret;
}

int hb_vdec_bind_vps(int vdecChn, int vpsGrp, int vpsChn) {
	int ret = 0;
	struct HB_SYS_MOD_S src_mod, dst_mod;

	src_mod.enModId = HB_ID_VDEC;
	src_mod.s32DevId = vdecChn;
	src_mod.s32ChnId = 0;
	dst_mod.enModId = HB_ID_VPS;
	dst_mod.s32DevId = vpsGrp;
	dst_mod.s32ChnId = 0;
	ret = HB_SYS_Bind(&src_mod, &dst_mod);
	if (ret)
		pr_err("hb_vdec_bind_vps failed\n");

	return ret;
}
