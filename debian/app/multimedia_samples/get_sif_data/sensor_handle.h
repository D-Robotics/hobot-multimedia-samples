#ifndef __SENSOR_HANDLE_H__
#define __SENSOR_HANDLE_H__

#include "hb_sys.h"
#include "hb_mipi_api.h"
#include "hb_vin_api.h"
#include "hb_vio_interface.h"
#include "hb_mode.h"

/*定义 sensor   初始化的属性信息 */
extern MIPI_SENSOR_INFO_S snsinfo;
/*定义 mipi 初始化参数信息 */
extern MIPI_ATTR_S mipi_attr;
/*定义 dev 初始化的属性信息 */
extern VIN_DEV_ATTR_S devinfo;
/*定义 pipe 属性信息 */
extern VIN_PIPE_ATTR_S pipeinfo;

int sensor_sif_dev_init(void);
int sif_dump_func(void);

#endif
