/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_VIO_VIN_H_
#define INCLUDE_VIO_VIN_H_

#include "stdint.h"
#include "stddef.h"
#include "camera/camera.h"
#include "hb_vio_interface.h"
#include "hb_mode.h"
#include "vio/vio_cfg.h"

typedef struct IOT_VIN_ATTR {
    VIN_DEV_ATTR_S *devinfo;
    VIN_PIPE_ATTR_S *pipeinfo;
    VIN_DIS_ATTR_S *disinfo;
    VIN_LDC_ATTR_S *ldcinfo;
    VIN_DEV_ATTR_EX_S *devexinfo;
    MIPI_SENSOR_INFO_S *snsinfo;
    MIPI_ATTR_S *mipi_attr;
} IOT_VIN_ATTR_S;

typedef enum HB_MIPI_SNS_TYPE_E {
    IMX415_30FPS_2160P_RAW10_LINEAR = 7,
    SAMPLE_SENOSR_ID_MAX,
} MIPI_SNS_TYPE_E;

typedef struct IOT_VIN_INFO_S {
	int dev_id;
	MIPI_SENSOR_INFO_S mipi_sensor_info;
	MIPI_ATTR_S mipi_attr;
	VIN_PIPE_ATTR_S vin_pipe_attr;
	VIN_DEV_ATTR_S vin_dev_attr;
	VIN_LDC_ATTR_S vin_ldc_attr;
	VIN_DIS_ATTR_S vin_dis_attr;
} IOT_VIN_INFO;

int hb_vin_init(uint32_t pipeId, uint32_t deseri_port,
        vio_cfg_t cfg, IOT_VIN_ATTR_S *vin_attr);
int hb_vin_start(uint32_t pipe_id, vio_cfg_t cfg);
void hb_vin_stop(int pipeId, vio_cfg_t cfg);
void hb_vin_deinit(int pipeId, vio_cfg_t cfg);
int hb_vin_feedback(int pipeId, hb_vio_buffer_t *feedback_buf);
int hb_vin_get_ouput(int pipeId, hb_vio_buffer_t *buffer);
int hb_vin_output_release(int pipeId, hb_vio_buffer_t *buffer);
#endif // INCLUDE_VIO_VIN_H_
