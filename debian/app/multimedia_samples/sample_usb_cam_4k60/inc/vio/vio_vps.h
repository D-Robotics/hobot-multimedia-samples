/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_VIO_VPS_H_
#define INCLUDE_VIO_VPS_H_
#include "stdint.h"
#include "hb_vps_api.h"
#include "hb_vio_interface.h"

int hb_vps_init(uint32_t pipeId, vio_cfg_t cfg);
int hb_vps_start(uint32_t pipe_id);
void hb_vps_stop(int pipeId);
void hb_vps_deinit(int pipeId);
int hb_vps_input(uint32_t pipe_id, hb_vio_buffer_t *buffer);
int hb_vps_get_output(uint32_t pipe_id, int channel, hb_vio_buffer_t *buffer);
int hb_vps_output_release(uint32_t pipe_id, int channel,
        hb_vio_buffer_t *buffer);
int hb_vps_change_pym_us(uint32_t pipe_id, int layer, int enable);
#endif // INCLUDE_VIO_VPS_H_
