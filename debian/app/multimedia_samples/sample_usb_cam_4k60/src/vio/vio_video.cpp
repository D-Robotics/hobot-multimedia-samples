#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <linux/videodev2.h>
#include "vio_video.h"

/* below h265 definition in kernel include/uapi/linux/videodev2.h,
 * but user app use toolchain's include file
 */
#define V4L2_PIX_FMT_H265     v4l2_fourcc('H', '2', '6', '5') /* H265 with start codes */

video_context *hb_video_alloc_context()
{
	video_context *video_ctx = (video_context *)calloc(1, sizeof(video_context));
	/* default video params (12M test pattern, 1080p, mjpeg)*/
	video_ctx->format = VIDEO_FMT_MJPEG;
	video_ctx->width = 1920;
	video_ctx->height = 1080;
	video_ctx->req_width = 1920;
	video_ctx->req_height = 1080;
	video_ctx->tmp_buffer = NULL;
	video_ctx->buffer_size = 0;
	video_ctx->quit = 0;
	video_ctx->has_venc = 0;		/* don't need venc as default */
	return video_ctx;
}

void hb_video_free_context(video_context *video_ctx){
	if(video_ctx){
		if(video_ctx->tmp_buffer){
			free(video_ctx->tmp_buffer);
			video_ctx->tmp_buffer = NULL;
		}
		free(video_ctx);
	}
}

video_format usb_fcc_to_video_format(unsigned int fcc)
{
	video_format format = VIDEO_FMT_INVALID;

	switch (fcc) {
	case V4L2_PIX_FMT_YUYV:
		format = VIDEO_FMT_YUY2;
		break;
	case V4L2_PIX_FMT_NV12:
		format = VIDEO_FMT_NV12;
		break;
	case V4L2_PIX_FMT_MJPEG:
		format = VIDEO_FMT_MJPEG;
		break;
	case V4L2_PIX_FMT_H264:
		format = VIDEO_FMT_H264;
		break;
	case V4L2_PIX_FMT_H265:
		format = VIDEO_FMT_H265;
		break;
	default:
		break;
	}

	return format;
}