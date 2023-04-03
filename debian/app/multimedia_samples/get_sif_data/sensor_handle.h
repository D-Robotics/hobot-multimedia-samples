#ifndef __SENSOR_HANDLE_H__
#define __SENSOR_HANDLE_H__


int sensor_sif_dev_init(char *cam_cfg_file, char *vio_cfg_file,int cam_index);
int sif_dump_func(void);

#endif
