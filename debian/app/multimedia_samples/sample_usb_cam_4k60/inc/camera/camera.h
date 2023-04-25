/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_CAMERA_H_
#define INCLUDE_CAMERA_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "hb_mipi_api.h"
#include "hb_vin_api.h"

extern VIN_LDC_ATTR_S LDC_ATTR_BASE;
extern VIN_DIS_ATTR_S DIS_ATTR_BASE;

extern MIPI_SENSOR_INFO_S SENSOR_IMX415_30FPS_10BIT_LINEAR_INFO;
extern MIPI_ATTR_S MIPI_SENSOR_IMX415_30FPS_10BIT_LINEAR_ATTR;
extern MIPI_ATTR_S MIPI_SENSOR_IMX415_30FPS_10BIT_LINEAR_SENSOR_CLK_ATTR;
extern VIN_DEV_ATTR_S DEV_ATTR_IMX415_LINEAR_BASE;
extern VIN_PIPE_ATTR_S PIPE_ATTR_IMX415_LINEAR_BASE;
extern VIN_DIS_ATTR_S DIS_ATTR_IMX415_BASE;
extern VIN_LDC_ATTR_S LDC_ATTR_IMX415_BASE;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
#endif // INCLUDE_CAMERA_H_