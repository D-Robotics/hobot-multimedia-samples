/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#include <memory>
#include <stdio.h>
#include <sys/stat.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "hb_sys.h"
#ifdef __cplusplus
}
#endif
#include "vio/vio_log.h"
#include "vio_vin.h"

int SAMPLE_MIPI_GetSnsAttrBySns(MIPI_SNS_TYPE_E enSnsType,
        MIPI_SENSOR_INFO_S *pstSnsAttr)
{
    switch (enSnsType) {
    case IMX415_30FPS_2160P_RAW10_LINEAR:
        memcpy(pstSnsAttr, &SENSOR_IMX415_30FPS_10BIT_LINEAR_INFO,
                sizeof(MIPI_SENSOR_INFO_S));
        break;
    default:
        pr_err("not surpport sensor type enSnsType %d\n", enSnsType);
        return -1;
    }
    pr_info("SAMPLE_MIPI_GetSnsAttrBySns success\n");
    return 0;
}

int SAMPLE_MIPI_GetMipiAttrBySns(MIPI_SNS_TYPE_E enSnsType,
        MIPI_ATTR_S *pstMipiAttr, vio_cfg_t cfg, int index)
{
    switch (enSnsType) {
    case IMX415_30FPS_2160P_RAW10_LINEAR:
        if (cfg.need_clk[index] == 1) {
            memcpy(pstMipiAttr,
                    &MIPI_SENSOR_IMX415_30FPS_10BIT_LINEAR_SENSOR_CLK_ATTR,
                    sizeof(MIPI_ATTR_S));
        } else {
            memcpy(pstMipiAttr, &MIPI_SENSOR_IMX415_30FPS_10BIT_LINEAR_ATTR,
                    sizeof(MIPI_ATTR_S));
        }
        break;
    default:
        pr_err("not surpport sensor type\n");
        return -1;
    }
    return 0;
}

int SAMPLE_VIN_GetDevAttrBySns(MIPI_SNS_TYPE_E enSnsType,
        VIN_DEV_ATTR_S *pstDevAttr)
{
    switch (enSnsType) {
    case IMX415_30FPS_2160P_RAW10_LINEAR:
        memcpy(pstDevAttr, &DEV_ATTR_IMX415_LINEAR_BASE,
                sizeof(VIN_DEV_ATTR_S));
        break;
    default:
        pr_err("not surpport sensor type\n");
        return -1;
    }
    pr_info("SAMPLE_VIN_GetDevAttrBySns success\n");

    return 0;
}

int SAMPLE_VIN_GetDevAttrExBySns(MIPI_SNS_TYPE_E enSnsType,
        VIN_DEV_ATTR_EX_S *pstDevAttrEx)
{
    switch (enSnsType) {
    default:
        pr_warn("not surpport sensor type\n");
        return -1;
    }
    pr_info("SAMPLE_VIN_GetDevAttrBySns success\n");
    return 0;
}

int SAMPLE_VIN_GetPipeAttrBySns(MIPI_SNS_TYPE_E enSnsType,
        VIN_PIPE_ATTR_S *pstPipeAttr) {
    switch (enSnsType) {
    case IMX415_30FPS_2160P_RAW10_LINEAR:
        memcpy(pstPipeAttr, &PIPE_ATTR_IMX415_LINEAR_BASE,
                sizeof(VIN_PIPE_ATTR_S));
        break;
    default:
        pr_err("not surpport sensor type\n");
        return -1;
    }
    pr_info("SAMPLE_VIN_GetPipeAttrBySns success\n");

    return 0;
}

int SAMPLE_VIN_GetDisAttrBySns(MIPI_SNS_TYPE_E enSnsType,
        VIN_DIS_ATTR_S *pstDisAttr)
{
    switch (enSnsType) {
    case IMX415_30FPS_2160P_RAW10_LINEAR:
        memcpy(pstDisAttr, &DIS_ATTR_IMX415_BASE, sizeof(VIN_DIS_ATTR_S));
        break;
    default:
        pr_err("not surpport sensor type\n");
        return -1;
    }
    pr_info("SAMPLE_VIN_GetDisAttrBySns success\n");

    return 0;
}

int SAMPLE_VIN_GetLdcAttrBySns(MIPI_SNS_TYPE_E enSnsType,
        VIN_LDC_ATTR_S *pstLdcAttr)
{
    switch (enSnsType) {
    case IMX415_30FPS_2160P_RAW10_LINEAR:
        memcpy(pstLdcAttr, &LDC_ATTR_IMX415_BASE, sizeof(VIN_LDC_ATTR_S));
        break;
    default:
        pr_err("not surpport sensor type\n");
        return -1;
    }
    pr_info("SAMPLE_VIN_GetLdcAttrBySns success\n");

    return 0;
}

int hb_sensor_init(int devId, vio_cfg_t cfg, IOT_VIN_ATTR_S *vin_attr)
{
    int ret = 0;

    vin_attr->snsinfo =
        static_cast<MIPI_SENSOR_INFO_S*>(malloc(sizeof(MIPI_SENSOR_INFO_S)));
    if (vin_attr->snsinfo == NULL) {
        pr_err("snsinfo malloc error\n");
        return -1;
    }
    memset(vin_attr->snsinfo, 0, sizeof(MIPI_SENSOR_INFO_S));
    SAMPLE_MIPI_GetSnsAttrBySns(static_cast<MIPI_SNS_TYPE_E>(cfg.sensor_id[devId]),
            vin_attr->snsinfo);

    if (devId == 1 && cfg.sensor_id[devId] == 2) {
        vin_attr->snsinfo->sensorInfo.sensor_addr = 0x10;
        printf("dual cam, change i2c addr to 0x10\n");
    }
    HB_MIPI_SetBus(vin_attr->snsinfo, cfg.i2c_bus[devId]);
    HB_MIPI_SetPort(vin_attr->snsinfo, cfg.sensor_port[devId]);
    HB_MIPI_SensorBindSerdes(vin_attr->snsinfo, cfg.serdes_index[devId], cfg.serdes_port[devId]);
    HB_MIPI_SensorBindMipi(vin_attr->snsinfo, cfg.mipi_idx[devId]);

    vin_attr->snsinfo->sensorInfo.gpio_num = cfg.gpio_num[devId];
    vin_attr->snsinfo->sensorInfo.gpio_pin[0] = cfg.gpio_pin[devId];
    vin_attr->snsinfo->sensorInfo.gpio_level[0] = cfg.gpio_level[devId];
    ret = HB_MIPI_InitSensor(devId, vin_attr->snsinfo);
    if (ret < 0) {
        pr_err("hb mipi init sensor error!\n");
        return ret;
    }
    pr_info("hb sensor init success...\n");

    return 0;
}

int hb_mipi_init(int mipiIdx, vio_cfg_t cfg, IOT_VIN_ATTR_S *vin_attr)
{
    int ret = 0;

    ret = HB_MIPI_SetMipiAttr(cfg.mipi_idx[mipiIdx], vin_attr->mipi_attr);
    if (ret < 0) {
        pr_err("hb mipi set mipi attr error!\n");
        return ret;
    }
    pr_info("hb mipi init success...\n");

    return 0;
}

static int hb_sensor_start(int devId)
{
	int ret = 0;

	ret = HB_MIPI_ResetSensor(devId);
	if (ret < 0)
	{
		printf("HB_MIPI_ResetSensor error!\n");
		return ret;
	}
	return ret;
}

static int hb_mipi_start(int mipiIdx)
{
	int ret = 0;

	ret = HB_MIPI_ResetMipi(mipiIdx);
	if (ret < 0)
	{
		printf("HB_MIPI_ResetMipi error!\n");
		return ret;
	}

	return ret;
}

static int hb_sensor_stop(int devId) {
	int ret = 0;

	ret = HB_MIPI_UnresetSensor(devId);
	if (ret < 0) {
		printf("HB_MIPI_UnresetSensor error!\n");
		return ret;
	} else {
	    printf("hb_sensor_stop chn:%d\n", devId);
    }
	return ret;
}

static int hb_mipi_stop(int mipiIdx) {
	int ret = 0;

	ret = HB_MIPI_UnresetMipi(mipiIdx);
	if (ret < 0) {
		printf("HB_MIPI_UnresetMipi error!\n");
		return ret;
	} else {
	    printf("hb_mipi_stop chn:%d\n", mipiIdx);
    }
	return ret;
}

static int hb_sensor_deinit(int devId) {
	int ret = 0;

	ret = HB_MIPI_DeinitSensor(devId);
	if (ret < 0) 	{
		printf("HB_MIPI_DeinitSensor error!\n");
		return ret;
	} else {
	    printf("hb_sensor_deinit chn:%d\n", devId);
    }
	return ret;
}

static int hb_mipi_deinit(int mipiIdx)
{
	int ret = 0;

	printf("hb_sensor_deinit end==mipiIdx %d====\n", mipiIdx);
	ret = HB_MIPI_Clear(mipiIdx);
	if (ret < 0)
	{
		printf("HB_MIPI_Clear error!\n");
		return ret;
	}
	printf("hb_mipi_deinit end======\n");
	return ret;
}

void dis_crop_set(uint32_t pipe_id, uint32_t event, VIN_DIS_MV_INFO_S *data,
        void *userdata)
{
    pr_debug("dis_crop_set callback come in\n");
    pr_debug("data gmvX %d\n", data->gmvX);
    pr_debug("data gmvY %d\n", data->gmvY);
    pr_debug("data xUpdate %d\n", data->xUpdate);
    pr_debug("data yUpdate %d\n", data->yUpdate);
    return;
}

int hb_vin_init(uint32_t pipeId, uint32_t deseri_port,
    vio_cfg_t cfg, IOT_VIN_ATTR_S *vin_attr)
{
    int ret = 0;
    int ipu_ds2_en = 0;
    int ipu_ds2_crop_en = 0;

    VIN_DIS_CALLBACK_S pstDISCallback;
    pstDISCallback.VIN_DIS_DATA_CB = dis_crop_set;

    vin_attr->devinfo = static_cast<VIN_DEV_ATTR_S*>(malloc(sizeof(VIN_DEV_ATTR_S)));
    if (vin_attr->devinfo == NULL) {
        pr_err("devinfo malloc error\n");
        return -1;
    }
    vin_attr->devexinfo =
        static_cast<VIN_DEV_ATTR_EX_S*>(malloc(sizeof(VIN_DEV_ATTR_EX_S)));
    if (vin_attr->devinfo == NULL) {
        pr_err("devexinfo malloc error\n");
        return -1;
    }
    vin_attr->pipeinfo = static_cast<VIN_PIPE_ATTR_S*>(malloc(sizeof(VIN_PIPE_ATTR_S)));
    if (vin_attr->pipeinfo == NULL) {
        pr_err("pipeinfo malloc error\n");
        return -1;
    }
    vin_attr->disinfo = static_cast<VIN_DIS_ATTR_S*>(malloc(sizeof(VIN_DIS_ATTR_S)));
    if (vin_attr->disinfo == NULL) {
        pr_err("disinfo malloc error\n");
        return -1;
    }
    vin_attr->ldcinfo = static_cast<VIN_LDC_ATTR_S*>(malloc(sizeof(VIN_LDC_ATTR_S)));
    if (vin_attr->ldcinfo == NULL) {
        pr_err("ldcinfo malloc error\n");
        return -1;
    }
    vin_attr->mipi_attr = static_cast<MIPI_ATTR_S*>(malloc(sizeof(MIPI_ATTR_S)));
    if (vin_attr->mipi_attr == NULL) {
        pr_err("mipi_attr malloc error\n");
        return -1;
    }

    memset(vin_attr->devinfo, 0, sizeof(VIN_DEV_ATTR_S));
    memset(vin_attr->pipeinfo, 0, sizeof(VIN_PIPE_ATTR_S));
    memset(vin_attr->disinfo, 0, sizeof(VIN_DIS_ATTR_S));
    memset(vin_attr->ldcinfo, 0, sizeof(VIN_LDC_ATTR_S));
    memset(vin_attr->mipi_attr, 0, sizeof(MIPI_ATTR_S));

    auto sensor_id = cfg.sensor_id[pipeId];
    auto testpattern_fps = cfg.testpattern_fps; 
    SAMPLE_VIN_GetDevAttrBySns(static_cast<MIPI_SNS_TYPE_E>(sensor_id),
      vin_attr->devinfo);
    SAMPLE_VIN_GetPipeAttrBySns(static_cast<MIPI_SNS_TYPE_E>(sensor_id),
      vin_attr->pipeinfo);
    SAMPLE_VIN_GetDisAttrBySns(static_cast<MIPI_SNS_TYPE_E>(sensor_id),
      vin_attr->disinfo);
    SAMPLE_VIN_GetLdcAttrBySns(static_cast<MIPI_SNS_TYPE_E>(sensor_id),
      vin_attr->ldcinfo);
    SAMPLE_VIN_GetDevAttrExBySns(static_cast<MIPI_SNS_TYPE_E>(sensor_id),
      vin_attr->devexinfo);
	SAMPLE_MIPI_GetMipiAttrBySns(static_cast<MIPI_SNS_TYPE_E>(sensor_id),
		vin_attr->mipi_attr, cfg, pipeId);

    ret = HB_SYS_SetVINVPSMode(pipeId,
      static_cast<SYS_VIN_VPS_MODE_E>(cfg.vin_vps_mode[pipeId]));
    if (ret < 0) {
        pr_err("HB_SYS_SetVINVPSMode%d error!\n", cfg.vin_vps_mode[pipeId]);
        return ret;
    }
    ret = HB_VIN_CreatePipe(pipeId, vin_attr->pipeinfo);  // isp init
    if (ret < 0) {
        pr_err("HB_VIN_CreatePipe error!\n");
        return ret;
    }
    ret = HB_VIN_SetMipiBindDev(pipeId, cfg.mipi_idx[pipeId]);
    if (ret < 0) {
        pr_err("HB_VIN_SetMipiBindDev error!\n");
        return ret;
    }
    ret = HB_VIN_SetDevVCNumber(pipeId, deseri_port);
    if (ret < 0) {
        pr_err("HB_VIN_SetDevVCNumber error!\n");
        return ret;
    }
    ret = HB_VIN_SetDevAttr(pipeId, vin_attr->devinfo);  // sif init
    if (ret < 0) {
        pr_err("HB_VIN_SetDevAttr error!\n");
        return ret;
    }
    if (cfg.need_md[pipeId]) {
        ret = HB_VIN_SetDevAttrEx(pipeId, vin_attr->devexinfo);
        if (ret < 0) {
            pr_err("HB_VIN_SetDevAttrEx error!\n");
            return ret;
        }
    }
    ret = HB_VIN_SetPipeAttr(pipeId, vin_attr->pipeinfo);  // isp init
    if (ret < 0) {
        pr_err("HB_VIN_SetPipeAttr error!\n");
        goto pipe_err;
    }
    ret = HB_VIN_SetChnDISAttr(pipeId, 1, vin_attr->disinfo);  //  dis init
    if (ret < 0) {
        pr_err("HB_VIN_SetChnDISAttr error!\n");
        goto pipe_err;
    }
    if (cfg.need_dis[pipeId]) {
        HB_VIN_RegisterDisCallback(pipeId, &pstDISCallback);
    }
    ret = HB_VIN_SetChnLDCAttr(pipeId, 1, vin_attr->ldcinfo);  //  ldc init
    if (ret < 0) {
        pr_err("HB_VIN_SetChnLDCAttr error!\n");
        goto pipe_err;
    }
    ret = HB_VIN_SetChnAttr(pipeId, 1);  //  dwe init
    if (ret < 0) {
        pr_err("HB_VIN_SetChnAttr error!\n");
        goto pipe_err;
    }
    ret = HB_VIN_SetDevBindPipe(pipeId, pipeId);  //  bind init
    if (ret < 0) {
        pr_err("HB_VIN_SetDevBindPipe error!\n");
        goto chn_err;
    }
#if 1
		HB_VIN_CtrlPipeMirror(pipeId, 1);
		if (ret < 0) {
        pr_err("HB_VIN_CtrlPipeMirror error!\n");
        goto chn_err;
    }
#endif
    ret = hb_sensor_init(pipeId, cfg, vin_attr);
    if (ret < 0) {
        pr_err("hb_sensor_init error!\n");
        goto chn_err;
    }
	ret = HB_MIPI_SetMipiAttr(cfg.mipi_idx[pipeId], vin_attr->mipi_attr);
	if (ret < 0) {
		pr_err("HB_MIPI_SetMipiAttr error!\n");
		return ret;
	}
    return ret;

chn_err:
    HB_VIN_DestroyPipe(pipeId);  // isp && dwe deinit
pipe_err:
    HB_VIN_DestroyDev(pipeId);   // sif deinit
    pr_err("iot_vin_init failed\n");
    return ret;
}

int hb_vin_start(uint32_t pipe_id, vio_cfg_t cfg) {
	int ret = 0;

	ret = HB_VIN_EnableChn(pipe_id, 0); // dwe start
	if (ret < 0)
	{
		printf("HB_VIN_EnableChn error!\n");
		return ret;
	}

	ret = HB_VIN_StartPipe(pipe_id); // isp start
	if (ret < 0)
	{
		printf("HB_VIN_StartPipe error!\n");
		return ret;
	}
	ret = HB_VIN_EnableDev(pipe_id); // sif start && start thread
	if (ret < 0)
	{
		printf("HB_VIN_EnableDev error!\n");
		return ret;
	}
	auto sensor_id = cfg.sensor_id[pipe_id];
	if (cfg.vin_vps_mode[pipe_id] != VIN_FEEDBACK_ISP_ONLINE_VPS_ONLINE) {
		hb_sensor_start(pipe_id);
		hb_mipi_start(cfg.mipi_idx[pipe_id]);
	}
	return ret;
}

void hb_vin_stop(int pipeId, vio_cfg_t cfg) {
	if (cfg.vin_vps_mode[pipeId] != VIN_FEEDBACK_ISP_ONLINE_VPS_ONLINE) {
		hb_sensor_stop(pipeId);
		hb_mipi_stop(cfg.mipi_idx[pipeId]);
	}
	HB_VIN_DisableDev(pipeId);	  // thread stop && sif stop
	HB_VIN_StopPipe(pipeId);	  // isp stop
	HB_VIN_DisableChn(pipeId, 1); // dwe stop

}

void hb_vin_deinit(int pipeId, vio_cfg_t cfg) {
	if (cfg.vin_vps_mode[pipeId] != VIN_FEEDBACK_ISP_ONLINE_VPS_ONLINE) {
		hb_mipi_deinit(cfg.mipi_idx[pipeId]);
		hb_sensor_deinit(pipeId);
	}
	HB_VIN_DestroyDev(pipeId);	  // sif deinit && destroy
	HB_VIN_DestroyChn(pipeId, 1); // dwe deinit
	HB_VIN_DestroyPipe(pipeId); // isp deinit && destroy
}

void print_sensor_dev_info(VIN_DEV_ATTR_S *devinfo) {
	printf("devinfo->stSize.format %d\n", devinfo->stSize.format);
	printf("devinfo->stSize.height %d\n", devinfo->stSize.height);
	printf("devinfo->stSize.width %d\n", devinfo->stSize.width);
	printf("devinfo->stSize.pix_length %d\n", devinfo->stSize.pix_length);
	printf("devinfo->mipiAttr.enable_frame_id %d\n", devinfo->mipiAttr.enable_frame_id);
	printf("devinfo->mipiAttr.enable_mux_out %d\n", devinfo->mipiAttr.enable_mux_out);
	printf("devinfo->mipiAttr.set_init_frame_id %d\n", devinfo->mipiAttr.set_init_frame_id);
	printf("devinfo->mipiAttr.ipi_channels %d\n", devinfo->mipiAttr.ipi_channels);
	printf("devinfo->mipiAttr.enable_line_shift %d\n", devinfo->mipiAttr.enable_line_shift);
	printf("devinfo->mipiAttr.enable_id_decoder %d\n", devinfo->mipiAttr.enable_id_decoder);
	printf("devinfo->mipiAttr.set_bypass_channels %d\n", devinfo->mipiAttr.set_bypass_channels);
	printf("devinfo->mipiAttr.enable_bypass %d\n", devinfo->mipiAttr.enable_bypass);
	printf("devinfo->mipiAttr.set_line_shift_count %d\n", devinfo->mipiAttr.set_line_shift_count);
	printf("devinfo->mipiAttr.enable_pattern %d\n", devinfo->mipiAttr.enable_pattern);

	printf("devinfo->outDdrAttr.stride %d\n", devinfo->outDdrAttr.stride);
	printf("devinfo->outDdrAttr.buffer_num %d\n", devinfo->outDdrAttr.buffer_num);
	return;
}

void print_sensor_pipe_info(VIN_PIPE_ATTR_S *pipeinfo) {
	printf("isp_out ddr_out_buf_num %d\n", pipeinfo->ddrOutBufNum);
	printf("isp_out width %d\n", pipeinfo->stSize.width);
	printf("isp_out height %d\n", pipeinfo->stSize.height);
	printf("isp_out sensor_mode %d\n", pipeinfo->snsMode);
	printf("isp_out format %d\n", pipeinfo->stSize.format);
	return;
}

void print_sensor_info(MIPI_SENSOR_INFO_S *snsinfo) {
	printf("bus_num %d\n", snsinfo->sensorInfo.bus_num);
	printf("bus_type %d\n", snsinfo->sensorInfo.bus_type);
	printf("sensor_name %s\n", snsinfo->sensorInfo.sensor_name);
	printf("reg_width %d\n", snsinfo->sensorInfo.reg_width);
	printf("sensor_mode %d\n", snsinfo->sensorInfo.sensor_mode);
	printf("sensor_addr 0x%x\n", snsinfo->sensorInfo.sensor_addr);
	printf("serial_addr 0x%x\n", snsinfo->sensorInfo.serial_addr);
	printf("resolution %d\n", snsinfo->sensorInfo.resolution);

	return;
}

int hb_vin_feedback(int pipeId, hb_vio_buffer_t *feedback_buf) {
	int ret = 0;
	ret = HB_VIN_SendPipeRaw(pipeId, feedback_buf, 1000);
	return ret;
}

int hb_vin_get_ouput(int pipeId, hb_vio_buffer_t *buffer) {
	int ret = 0;
	ret = HB_VIN_GetChnFrame(pipeId, 0, buffer, 2000);
	return ret;
}

int hb_vin_output_release(int pipeId, hb_vio_buffer_t *buffer) {
	int ret = 0;
	printf("relase\n");
	ret = HB_VIN_ReleaseChnFrame(pipeId, 0, buffer);
	if (ret < 0) {
		printf("HB_VIN_ReleaseChnFrame error!!!\n");
	}
	return ret;
}