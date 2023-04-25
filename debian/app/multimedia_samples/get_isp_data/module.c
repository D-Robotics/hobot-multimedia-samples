/*
* @Author: yaqiang.li
* @Date:   2022-01-28 10:38:36
* @Last Modified by:   yaqiang.li
* @Last Modified time: 2022-01-28 11:59:47
*/

#include <stdio.h>
#include <string.h>
#include <module.h>

extern int __sensor_module_start,  __sensor_module_end;

sensor_param_t sensor_lists[16];

void sensor_menu_message(void)
{
	printf("\n");	
	printf("Horizon Robotics Sensor Test Tools V1.0\n");
	printf("\n");
}

int sensor_modules_probe(void)
{
	install_call *fn ;

	int *sensor_module_start = &__sensor_module_start;
	sensor_module_start++; // skip .inst

	memset(sensor_lists, 0, sizeof(sensor_lists));

	/* Install sensor modules */
	for (fn = (install_call *)sensor_module_start;
		fn < (install_call *)&__sensor_module_end; fn++) {
		(*fn)();
	}

	sensor_menu_message();

	return 0;
}