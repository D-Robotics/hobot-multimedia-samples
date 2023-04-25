/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_VIO_VIDEO_H_
#define INCLUDE_VIO_VIDEO_H_
#include <stdio.h>
#include <stdint.h>
typedef enum {
	VIDEO_FMT_INVALID = 0,
	VIDEO_FMT_YUY2,
	VIDEO_FMT_NV12,
	VIDEO_FMT_H264,
	VIDEO_FMT_H265,
	VIDEO_FMT_MJPEG,
} video_format;

typedef struct video_context {
	video_format format;
	uint32_t width;
	uint32_t height;
	uint32_t req_width;
	uint32_t req_height;
	uint8_t has_venc;
	int quit;
	// helper temp buffer
	void *tmp_buffer;
	int buffer_size;
} video_context;

/* hb video api functions */
video_context *hb_video_alloc_context();
void hb_video_free_context(video_context *video_ctx);
video_format usb_fcc_to_video_format(unsigned int fcc);
#endif // INCLUDE_VIO_VIDEO_H_