/*
* @Author: yaqiang.li
* @Date:   2022-01-28 10:38:36
* @Last Modified by:   yaqiang.li
* @Last Modified time: 2022-01-28 10:38:36
*/

#ifndef __MODULE_H__
#define __MODULE_H__

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

typedef int (*install_call)(void);

#define SENSOR_MODULE_INSTALL(fn)	\
	static install_call  fn##call __attribute__((section(".sensor_module"))) = fn;

typedef int (*set_sensor_param_func)(void);

typedef struct {
	char sensor_tag[32];
	set_sensor_param_func func;
} sensor_param_t;

extern sensor_param_t sensor_lists[16];

int sensor_modules_probe(void);

#endif