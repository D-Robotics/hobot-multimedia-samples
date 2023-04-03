#ifndef __SENSOR_HANDLE_H__
#define __SENSOR_HANDLE_H__

#include "hb_sys.h"
#include "hb_mipi_api.h"
#include "hb_vin_api.h"
#include "hb_vio_interface.h"
#include "hb_mode.h"

int sensor_sif_dev_init(char *cam_cfg_file, char *vio_cfg_file,int cam_index);
int yuv_dump_func(void);

#endif
