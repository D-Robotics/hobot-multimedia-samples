/***************************************************************************
* COPYRIGHT NOTICE
* Copyright 2019 Horizon Robotics, Inc.
* All rights reserved.
***************************************************************************/
#include <thread>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
extern "C" {
#include "hb_vin_api.h"
#include "hb_vps_api.h"
#include "hb_mipi_api.h"
#include "hb_vio_interface.h"
#include "hb_sys.h"
#include "hb_isp_api.h"
}
#include "vio_venc.h"
#include "vio_vin.h"
#include "vio_vps.h"
#include "vio_sys.h"
#include "vio_cfg.h"
#include "vio_video.h"
#include "utils/yuv2yuv.h"
#include "utils/utils.h"
#include "guvc/uvc_gadget_api.h"

/******************************************************************************
 * Notice: 
 *      PTZ function used 2 vps groups to realize roi(360p) zoom in
 *      to 4K/1080p/720p final resolution.
 *  
 *      Because pym is not exeactly follow the us ratio, so we need to set input
 *      size bigger than real size. The function is as below:
 *      xxxxxxxxxxxx
 *  
 *      Workflow: VPS0(IPU2 ds)--->VPS1(IPU5 us)--->VPS1(PYM us)--->Crop Output
 *                672x378--------->968x544(1.5)---->3864x2184(4)--->3840x2160
 *                672x378--------->968x544(1.5)---->1932x1084(2)--->1920x1080
 *                640x360--------->648x364(1)------>1292x724(2)---->1280x720
 * ***************************************************************************/

int pym_width = 3840;
int pym_height = 2160;
int image_stride = 0;
vio_cfg_t vio_cfg;
VPS_PYM_CHN_ATTR_S vps1_us_2160p_attr;
VPS_PYM_CHN_ATTR_S vps1_us_1080p_attr;
VPS_PYM_CHN_ATTR_S vps1_us_720p_attr;

extern int running;
static int uvc_stream_on = 0;
static int need_encoder = -1;
static std::mutex mutex_;
static vencParam_t vencparam;

void signal_handler(int signal_number) {
	running = 0;
}

static bool IsUvcStreamOn() {
std::lock_guard<std::mutex> lg(mutex_);
    if (uvc_stream_on == 0) {
        return false;
    } else {
        return true;
    }
}

static PAYLOAD_TYPE_E fcc_to_video_format(unsigned int fcc) {
    PAYLOAD_TYPE_E format;
    switch (fcc) {
        case V4L2_PIX_FMT_NV12:
            format = PT_NV;
            break;
        case V4L2_PIX_FMT_MJPEG:
            format = PT_MJPEG;
            printf("format is MJPEG\n");
            break;
        case V4L2_PIX_FMT_H265:
            format = PT_H265;
            break;
        case V4L2_PIX_FMT_H264:
        default:
            format = PT_H264;
            printf("format is H264\n");
    }

	return format;
}

int us_pym_attr_2160p[2] = {968, 544};
int us_pym_attr_1080p[2] = {968, 544};
int us_pym_attr_720p[2] = {648, 364};
int us_pym_attr_init(VPS_PYM_CHN_ATTR_S *attr, int us_pym_attr[2]) {
    memset(attr, 0x0, sizeof(VPS_PYM_CHN_ATTR_S));
    attr->frame_id = 0;
	attr->ds_uv_bypass = 0;
	attr->ds_layer_en = 5;
	attr->us_layer_en = 6;
	attr->us_uv_bypass = 0;
	attr->timeout = 2000;
	attr->frameDepth = 1;

    for (auto i = 0; i < 6; i++) {
       attr->us_info[i].factor = vio_cfg.chn_pym_cfg[1][0].us_info[i].factor;
       attr->us_info[i].roi_x = 0;
       attr->us_info[i].roi_y = 0;
       attr->us_info[i].roi_width = us_pym_attr[0];
       attr->us_info[i].roi_height = us_pym_attr[1];
       printf("vps1 us%d factor:%d x:%d y:%d w:%d h:%d\n", i, attr->us_info[i].factor,
       attr->us_info[i].roi_x, attr->us_info[i].roi_y, 
       attr->us_info[i].roi_width, attr->us_info[i].roi_height);
    }
	return 0;
}

int camera_init(vio_cfg_t vio_cfg, IOT_VIN_ATTR_S *vin_attr) {
    int ret;
	ret = hb_vin_init(0, 0, vio_cfg, vin_attr);
    if (ret) {
        printf("hb_vin_init failed, %d\n", ret);
        return -1;
    }
	ret = hb_vps_init(0, vio_cfg);
    if (ret) {
        hb_vin_deinit(0, vio_cfg);
        printf("hb_vps_init failed, %d\n", ret);
        return -1;
    }

	ret = hb_vin_bind_vps(0, 0, 0, vio_cfg);
    if (ret) {
        hb_vin_deinit(0, vio_cfg);
        printf("hb_vin_bind_vps failed, %d\n", ret);
        return -1;
    }

	ret = hb_vps_start(0);
    if (ret) {
        hb_vin_deinit(0, vio_cfg);
        printf("hb_vps_start failed, %d\n", ret);
        return -1;
    }

	ret = hb_vin_start(0, vio_cfg);
    if (ret) {
        printf("hb_vin_start failed, %d\n", ret);
        hb_vps_stop(0);
        hb_vin_deinit(0, vio_cfg);
        return -1;
    }

    us_pym_attr_init(&vps1_us_2160p_attr, us_pym_attr_2160p);
    us_pym_attr_init(&vps1_us_1080p_attr, us_pym_attr_1080p);
    us_pym_attr_init(&vps1_us_720p_attr, us_pym_attr_720p);

    return 0;
}

int camera_deinit(vio_cfg_t vio_cfg) {
	hb_vin_stop(0, vio_cfg);
	hb_vps_stop(0);
	hb_venc_stop(0);
	hb_venc_deinit(0);
    HB_VENC_Module_Uninit();
	hb_vps_deinit(0);
	hb_vin_deinit(0, vio_cfg);
	return 0;
}

int VencInit(vencParam_t *param) {
    vencParam_t *vencparam = reinterpret_cast<vencParam_t *>(param);
	auto ret = hb_venc_init(param);
    if (ret) {
        printf("hb_venc_init failed, %d\n", ret);
        return -1;
    }

	ret = hb_venc_start(0);
    if (ret) {
        printf("hb_venc_start failed, %d\n", ret);
        hb_venc_deinit(0);
        return -1;
    }

    return 0;
}

int VencDeinit(vencParam_t *param) {
    vencParam_t *vencparam = reinterpret_cast<vencParam_t *>(param);
	hb_venc_stop(0);
	hb_venc_deinit(0);
    return 0;
}

static void uvc_streamon_off(struct uvc_context *ctx, int is_on,
                                 void *userdata) {
    struct uvc_device *dev;
    unsigned int fcc = 0;
	video_context *video_ctx = (video_context *)userdata;
	dev = ctx->udev;
	fcc = dev->fcc;

    //printf("uvc_stream on/off value:%d\n", is_on);
    if (is_on) {
		video_ctx->format = usb_fcc_to_video_format(fcc);
		video_ctx->width = dev->width;
		video_ctx->height = dev->height;
		video_ctx->req_width = dev->width;
		video_ctx->req_height = dev->height;
        if (dev->height >= 320) {
            pym_width = dev->width;
            pym_height = dev->height;
        } else {
            printf("uvc_stream on/off fatal error,\
                unsupport pixel solution: %d\n", dev->height);
            return;
        }

		if(vio_cfg.chn_pym_en[0][0] == 0){
			vio_cfg.chn_width[0][0] = pym_width;
			vio_cfg.chn_height[0][0] = pym_height;
		}
        hb_vin_unbind_vps(0, 0, vio_cfg);
        hb_vps_stop(0);
        hb_vps_deinit(0);

        hb_vps_init(0, vio_cfg);
        hb_vin_bind_vps(0, 0, 0, vio_cfg);
        hb_vps_start(0);
		if (video_ctx->tmp_buffer) {
			free(video_ctx->tmp_buffer);
			video_ctx->tmp_buffer = NULL;
		}
		//printf("uvc_streamon_off fcc = %d,V4L2_PIX_FMT_YUYV = %d\n",fcc,V4L2_PIX_FMT_YUYV);
        if (fcc == V4L2_PIX_FMT_NV12 || fcc == V4L2_PIX_FMT_YUYV) {
            std::lock_guard<std::mutex> lg(mutex_);
			/* prepare tmp buffer for video convert (eg. nv12 to yuy2) */
			if (video_ctx->format == VIDEO_FMT_YUY2) {
				video_ctx->buffer_size = pym_width * pym_height * 2;
				video_ctx->tmp_buffer = calloc(1, video_ctx->buffer_size);
				printf("%s function. calloc tmp buffer for yuy2,video_ctx->buffer_size = %d\n", __func__,video_ctx->buffer_size);
				if (!video_ctx->tmp_buffer) {
					printf("calloc for tmp_buffer failed\n");
				}
			}
            vencparam.type = PT_NV;
            need_encoder = 0;
            uvc_stream_on = 1;
            printf("NV12 or YUY2 format!!!\n");
        } else {
            vencparam.width = pym_width;
            vencparam.height = pym_height;
            vencparam.veChn = 0;
            //vencparam.bitrate = 100000;
						vencparam.bitrate = 12000;
            // vencparam.bind_flag = 1;
            vencparam.vpsGrp = 0;
            vencparam.vpsChn = 2;
            vencparam.stride = 0;
            vencparam.mjpgQp = vio_cfg.mjpgQp;
            vencparam.type = fcc_to_video_format(fcc);
            VencInit(&vencparam);
            std::lock_guard<std::mutex> lg(mutex_);
            need_encoder = 1;
            //printf("uvc_streamon_off before sleep\n");
            //sleep(3);
            //printf("uvc_streamon_off after sleep 1 secs\n");
            uvc_stream_on = 1;
        }
    } else {
        {
            std::lock_guard<std::mutex> lg(mutex_);
            uvc_stream_on = 0;
        }

        while(need_encoder != -1) {
            usleep(10000);
            printf("wait for encoder stop\n");
        }
        if (vencparam.type != PT_NV) {
            VencDeinit(&vencparam);
        }
        RingQueue<VIDEO_STREAM_S>::Instance().Clear();
        /* release tmp buffer not in use */
		if (video_ctx->tmp_buffer) {
			free(video_ctx->tmp_buffer);
			video_ctx->tmp_buffer = NULL;
		}
    }
}

static FILE *h264_file = NULL;
static struct timeval h264_start,h264_end;
static int done = 0;
static int uvc_save_h264_file(char *buf, int len){
	if(done == 0){
		if(!h264_file){
			h264_file = fopen("/userdata/h264","w+");
			if(!h264_file){
				printf("uvc_save_h264_file fopen failed\n");
				return -1;
			}else{
				gettimeofday(&h264_start, NULL);
			}
		}
		gettimeofday(&h264_end, NULL);
		if(((h264_end.tv_sec - h264_start.tv_sec)*1000 + (h264_end.tv_usec - h264_start.tv_usec)/1000) < 20000){
			if(h264_file){
				fwrite(buf, len, 1, h264_file);
			}
		}else{
			if(h264_file){
				fclose(h264_file);
				done = 1;
				h264_file = NULL;
			}
		}
	}
	return 0;
}

static int uvc_get_frame(struct uvc_context *ctx, void **buf_to,
            int *buf_len, void **entity, void *userdata) {
    video_context *video_ctx = (video_context *)userdata;        	
    if (!ctx || !ctx->udev || !video_ctx) {
        printf("uvc_get_frame: input params is null\n");
        return -EINVAL;
    }
    struct uvc_device *dev = ctx->udev;
    if (IsUvcStreamOn() == 0) {
        printf("uvc_get_frame: stream is off\n");
        return -EFAULT;
    }

    VIDEO_STREAM_S *one_video =
        (VIDEO_STREAM_S *)calloc(1, sizeof(VIDEO_STREAM_S));
    if (!one_video) {
        return -EINVAL;
    }

    if(!RingQueue<VIDEO_STREAM_S>::Instance().Pop(*one_video)) {
        return -EINVAL;
    }
	if (dev->fcc == V4L2_PIX_FMT_YUYV){
		if (!video_ctx->tmp_buffer) {
			printf("uvc_get_frame failed!! video_ctx->tmp_buffer is NULL\n");
			return -EINVAL;
		}
		struct timeval ts0, ts1;
		gettimeofday(&ts0, NULL);
		hb_yuv2yuv_nv12toyuy2_neon((uint8_t *)one_video->pstPack.vir_ptr,
				(uint8_t *)(one_video->pstPack.vir_ptr + video_ctx->width * video_ctx->height),
				NULL,
				(uint8_t *)video_ctx->tmp_buffer,
				video_ctx->width,
				video_ctx->height,
				1);
		gettimeofday(&ts1, NULL);
		printf("hb_yuv2yuv_nv12toyuy2_neon time is %ld\n",(ts1.tv_sec - ts0.tv_sec)*1000 + (ts1.tv_usec - ts0.tv_usec)/1000);
		*buf_to = video_ctx->tmp_buffer;
		*buf_len = video_ctx->buffer_size;
		*entity = one_video;
		return 0;
	}
    *buf_to = one_video->pstPack.vir_ptr;
    *buf_len = one_video->pstPack.size;
    *entity = one_video;
    uvc_save_h264_file((char*)*buf_to,*buf_len);
    return 0;
}

static void uvc_release_frame(struct uvc_context *ctx, void **entity,
            void *userdata) {

    if (!ctx || !entity || !(*entity)) return;
    auto video_buffer = static_cast<VIDEO_STREAM_S *>(*entity);
    if (video_buffer->pstPack.vir_ptr) {
        free(video_buffer->pstPack.vir_ptr);
    }

    free(video_buffer);
    *entity = nullptr;
}

int uvc_init(uvc_context *uvc_ctx, video_context *video_ctx) {
  struct uvc_params params;
  char *uvc_devname = NULL;  /* uvc Null, lib will find one */
  char *v4l2_devname = NULL; /* v4l2 Null is Null... */
  int ret;

  /* init uvc user params, just use default params and overlay
   * some specify params.
   */
  uvc_gadget_user_params_init(&params);
#if 1
  // bulk mode
  params.bulk_mode = 1;
  params.h264_quirk = 0;
  params.burst = 9;
#else
// isoc
  params.bulk_mode = 0; /* use isoc mode  */
  params.h264_quirk = 0;
  params.burst = 10;/*3.0线要改成10,2.0不设置或者设置成0*/
  params.mult = 2;
#endif

  ret = uvc_gadget_init(&uvc_ctx, uvc_devname, v4l2_devname, &params);
  if (ret < 0) {
    printf("uvc_gadget_init error!\n");
    return ret;
  }
  uvc_set_streamon_handler(uvc_ctx, uvc_streamon_off, video_ctx);
  /* prepare/release buffer with video queue */
  uvc_set_prepare_data_handler(uvc_ctx, uvc_get_frame, video_ctx);
  uvc_set_release_data_handler(uvc_ctx, uvc_release_frame, video_ctx);

  ret = uvc_gadget_start(uvc_ctx);
  if (ret < 0) {
    printf("uvc_gadget_start error!\n");
  }

  return ret;
}

int crop_region_calc() {
    static int hold_time = 0, jump_count = 0;
    static int step = 0;
	pym_buffer_t out_pym_buf;
	hb_vio_buffer_t out_buf;
	int ret;

    if(hold_time == 0) {
        VPS_CROP_INFO_S crop_info;
        if (++jump_count % 2) {
            return 0;
        }
        memset(&crop_info, 0, sizeof(VPS_CROP_INFO_S));
        crop_info.en = 1;
        crop_info.cropRect.width = 3840 - 16 * step;
        crop_info.cropRect.height = 2160 - 9 * step;

        step += 6;
        if (crop_info.cropRect.width < 672) {
            crop_info.cropRect.width = 672;
            crop_info.cropRect.height = 378;
            hold_time = 300;
            step = 0;
        }
        ret = HB_VPS_SetChnCrop(0, 2, &crop_info);
        if (ret) {
            printf("HB_VPS_SetChnCrop error!!!\n");
            return -1;
        }
    } else {
        hold_time -= 1;
        jump_count = 0;
    }
	return 0;
}

void get_frame_thread(void *param) {
    VIDEO_FRAME_S pstFrame;
    VIDEO_STREAM_S pstStream;
    VPS_PYM_CHN_ATTR_S pym_chn_attr;
    struct timeval ts0, ts1, ts2, ts3;
    int pts = 0, idx = 0, ret = 0;
    time_t start, last = time(NULL);
    vencParam_t *vencparam = reinterpret_cast<vencParam_t *>(param);
		uint32_t last_frame_id = -1;
		static int test = 0;
    auto *pvio_image = reinterpret_cast<pym_buffer_t *>(
        std::calloc(1, sizeof(pym_buffer_t)));
    while (running) {
        if (!vencparam->bind_flag) {
            memset(&pstFrame, 0, sizeof(VIDEO_FRAME_S));
            memset(&pstStream, 0, sizeof(VIDEO_STREAM_S));
            if (!uvc_stream_on) {
                usleep(10000);
                std::lock_guard<std::mutex> lg(mutex_);
                if (!uvc_stream_on) {
                    need_encoder = -1;
                    continue;
                }
            }
			//gettimeofday(&ts0, NULL);
            ret = HB_VPS_GetChnFrame(0, 2,
                reinterpret_cast<void *>(pvio_image), 2000);

            /*skip the last ipu frame when ipu to pym*/
            if (ret != 0) {
                usleep(10000);
                continue;
            }
			//gettimeofday(&ts2, NULL);
			//printf("one frame GetChnFrame time is %ld\n",(ts2.tv_sec - ts0.tv_sec)*1000 + (ts2.tv_usec - ts0.tv_usec)/1000);
            pstFrame.stVFrame.vir_ptr[0] = pvio_image->pym[0].addr[0];
            pstFrame.stVFrame.vir_ptr[1] = pvio_image->pym[0].addr[1];
            pstFrame.stVFrame.phy_ptr[0] = pvio_image->pym[0].paddr[0];
            pstFrame.stVFrame.phy_ptr[1] = pvio_image->pym[0].paddr[1];
            {
                std::lock_guard<std::mutex> lg(mutex_);
                if (!uvc_stream_on) {
                    HB_VPS_ReleaseChnFrame(0, 2,
                        reinterpret_cast<void *>(pvio_image));
                    need_encoder = 1;
                    continue;
                }
            }

            pstFrame.stVFrame.width = pym_width;
            pstFrame.stVFrame.height = pym_height;
            pstFrame.stVFrame.size = pym_width * pym_height * 3 / 2;
            pstFrame.stVFrame.pix_format = HB_PIXEL_FORMAT_NV12;
            // pstFrame.stVFrame.stride = pym_width;
            // pstFrame.stVFrame.vstride = pym_width;

            if (RingQueue<VIDEO_STREAM_S>::Instance().IsValid()) {
                if (!need_encoder) {
                    auto buffer_size = pstFrame.stVFrame.size;
                    pstStream.pstPack.vir_ptr = (char *)calloc(1, buffer_size);
                    pstStream.pstPack.size = buffer_size;

                    memcpy(pstStream.pstPack.vir_ptr, pstFrame.stVFrame.vir_ptr[0],
                                pstFrame.stVFrame.height * pstFrame.stVFrame.width);
                    memcpy(pstStream.pstPack.vir_ptr + pstFrame.stVFrame.height * pstFrame.stVFrame.width,
                                pstFrame.stVFrame.vir_ptr[1],
                                pstFrame.stVFrame.height * pstFrame.stVFrame.width / 2);

                    RingQueue<VIDEO_STREAM_S>::Instance().Push(pstStream);
                } else {
					//gettimeofday(&ts2, NULL);
                    HB_VENC_SendFrame(0, &pstFrame, 1000);
                    ret = HB_VENC_GetStream(0, &pstStream, 2000);
                    if (ret) {
                        HB_VPS_ReleaseChnFrame(0, 2,
                            reinterpret_cast<void *>(pvio_image));
                        printf("HB_VENC_GetStream failed\n");
                        continue;
                    }
					//gettimeofday(&ts3, NULL);
					//printf("one frame venc time is %ld\n",(ts3.tv_sec - ts2.tv_sec)*1000 + (ts3.tv_usec - ts2.tv_usec)/1000);
                    auto video_buffer = pstStream;
                    auto buffer_size = video_buffer.pstPack.size;
                    video_buffer.pstPack.vir_ptr = (char *)calloc(1, buffer_size);
                    if (video_buffer.pstPack.vir_ptr) {
                        memcpy(video_buffer.pstPack.vir_ptr, pstStream.pstPack.vir_ptr,
                            buffer_size);
                        RingQueue<VIDEO_STREAM_S>::Instance().Push(video_buffer);
                    }
                    HB_VENC_ReleaseStream(0, &pstStream);
                }
            }
        } else {
            memset(&pstFrame, 0, sizeof(VIDEO_FRAME_S));
            memset(&pstStream, 0, sizeof(VIDEO_STREAM_S));
            if (!uvc_stream_on) {
                usleep(10000);
                std::lock_guard<std::mutex> lg(mutex_);
                if (!uvc_stream_on) {
                    need_encoder = -1;
                    continue;
                }
            }
            if (RingQueue<VIDEO_STREAM_S>::Instance().IsValid()) {
                ret = HB_VENC_GetStream(0, &pstStream, 2000);
                if (ret) {
                    printf("HB_VENC_GetStream failed\n");
                    continue;
                }
                auto video_buffer = pstStream;
                auto buffer_size = video_buffer.pstPack.size;
                video_buffer.pstPack.vir_ptr = (char *)calloc(1, buffer_size);
                if (video_buffer.pstPack.vir_ptr) {
                    memcpy(video_buffer.pstPack.vir_ptr, pstStream.pstPack.vir_ptr,
                        buffer_size);
                    RingQueue<VIDEO_STREAM_S>::Instance().Push(video_buffer);
                }
                HB_VENC_ReleaseStream(0, &pstStream);
            }
        }
        //gettimeofday(&ts1, NULL);
		//printf("one frame total process time is %ld\n",(ts1.tv_sec - ts0.tv_sec)*1000 + (ts1.tv_usec - ts0.tv_usec)/1000);
        start = time(NULL);
        idx++;
        if (start > last) {
            gettimeofday(&ts1, NULL);
            printf("[%ld.%06ld]venc bind chn %d fps %d\n",
                ts1.tv_sec, ts1.tv_usec, 0, idx);
            last = start;
            idx = 0;
        }
vps_release:
        if (!vencparam->bind_flag) {
            HB_VPS_ReleaseChnFrame(0, 2,
                reinterpret_cast<void *>(pvio_image));
        }
    }
    RingQueue<VIDEO_STREAM_S>::Instance().Exit();
    free(pvio_image);
}

/******************************************************************************
 * Notice: 
 *      For Video encoder, we need to ouput standard resolution like 4K/1080p
 *      But the image upscaled by VPS is bigger than standard value, so we 
 *      need to crop image, then send to venc.
 *  
 *      For MJPEG and NV12 format, we set crop roi then do encoder
 *      For H264, we set image stride then do encoder
 * ***************************************************************************/
void get_frame_thread_ptz(void *param) {
    VIDEO_FRAME_S pstFrame;
    VIDEO_STREAM_S pstStream;
    struct timeval ts0, ts1, ts2;
    int pts = 0, idx = 0;
    time_t start, last = time(NULL);
    int ret;

    auto *pvio_image = reinterpret_cast<pym_buffer_t *>(
        std::calloc(1, sizeof(pym_buffer_t)));

    while (running) {
        memset(&pstFrame, 0, sizeof(VIDEO_FRAME_S));
        memset(&pstStream, 0, sizeof(VIDEO_STREAM_S));
        if (!uvc_stream_on) {
            usleep(10000);
            std::lock_guard<std::mutex> lg(mutex_);
            if (!uvc_stream_on) {
                need_encoder = -1;
                continue;
            }
        }

        crop_region_calc();
		auto ret = HB_VPS_GetChnFrame_Cond(1, 5,
				reinterpret_cast<void *>(pvio_image), 2000, 0);
        if (ret != 0) {
            usleep(10000);
            printf("HB_VPS_GetChnFrame_Cond timeout\n");
            continue;
        }
        auto layer = pym_width == 3840 ? 5 : 2;

        pstFrame.stVFrame.vir_ptr[0] = pvio_image->us[layer].addr[0];
        pstFrame.stVFrame.vir_ptr[1] = pvio_image->us[layer].addr[1];
        pstFrame.stVFrame.phy_ptr[0] = pvio_image->us[layer].paddr[0];
        pstFrame.stVFrame.phy_ptr[1] = pvio_image->us[layer].paddr[1];
        {
            std::lock_guard<std::mutex> lg(mutex_);
            if (!uvc_stream_on) {
                HB_VPS_ReleaseChnFrame(1, 5,
                    reinterpret_cast<void *>(pvio_image));
                need_encoder = 1;
                continue;
            }
        }

        pstFrame.stVFrame.width = pym_width;
        pstFrame.stVFrame.height = pym_height;
        pstFrame.stVFrame.size = pym_width * pym_height * 3 / 2;
        pstFrame.stVFrame.pix_format = HB_PIXEL_FORMAT_NV12;
        pstFrame.stVFrame.stride = image_stride;
        pstFrame.stVFrame.vstride = image_stride;

        if (RingQueue<VIDEO_STREAM_S>::Instance().IsValid()) {
            if (!need_encoder) {
                auto buffer_size = pstFrame.stVFrame.size;
                pstStream.pstPack.vir_ptr = (char *)calloc(1, buffer_size);
                pstStream.pstPack.size = buffer_size;
                auto y_src = pstFrame.stVFrame.vir_ptr[0];
                auto uv_src = pstFrame.stVFrame.vir_ptr[1];
                auto y_dst = pstStream.pstPack.vir_ptr;
                auto uv_dst = y_dst + pym_width * pym_height;

                // jump over stride - width Y
                for (auto i = 0; i < pym_height; i++) {
                    memcpy(y_dst + i * pym_width, y_src + i * image_stride, pym_width);
                }

                // jump over stride - width UV
                for (auto i = 0; i < pym_height / 2; i++) {
                    memcpy(uv_dst + i * pym_width, uv_src + i * image_stride, pym_width);
                }

                RingQueue<VIDEO_STREAM_S>::Instance().Push(pstStream);
            } else {
                // dump_pym_us_to_files(pvio_image, 5, layer);
                HB_VENC_SendFrame(0, &pstFrame, 0);
                ret = HB_VENC_GetStream(0, &pstStream, 2000);
                if (ret) {
                    HB_VPS_ReleaseChnFrame(1, 5,
                        reinterpret_cast<void *>(pvio_image));
                    printf("HB_VENC_GetStream failed\n");
                    continue;
                }
                auto video_buffer = pstStream;
                auto buffer_size = video_buffer.pstPack.size;
                video_buffer.pstPack.vir_ptr = (char *)calloc(1, buffer_size);
                if (video_buffer.pstPack.vir_ptr) {
                    memcpy(video_buffer.pstPack.vir_ptr, pstStream.pstPack.vir_ptr,
                        buffer_size);
                    RingQueue<VIDEO_STREAM_S>::Instance().Push(video_buffer);
                }
                HB_VENC_ReleaseStream(0, &pstStream);
            }
            gettimeofday(&ts1, NULL);
            start = time(NULL);
            idx++;
            if (start > last) {
                gettimeofday(&ts1, NULL);
                printf("[%ld.%06ld]venc chn %d fps %d\n",
                    ts1.tv_sec, ts1.tv_usec, 0, idx);
                last = start;
                idx = 0;
            }
        }
vps_release:
        HB_VPS_ReleaseChnFrame(1, 5, reinterpret_cast<void *>(pvio_image));
    }
    RingQueue<VIDEO_STREAM_S>::Instance().Exit();
    free(pvio_image);
}

int main(int argc, char *argv[]) {
	struct uvc_context *uvc_ctx = NULL;
	video_context *video_ctx = NULL;
	struct uvc_params params;
    IOT_VIN_ATTR_S vin_attr;
    int ret;
    std::thread *t;
	signal(SIGINT, signal_handler);
    std::string cfg_path("./vin_vps_config_usb_cam.json");
    auto config = std::make_shared<VioConfig>(cfg_path);
    if (!config || !config->LoadConfig()) {
        printf("falied to load config file: %s\n", cfg_path.data());
        return -1;
    }
    config->ParserConfig();
    vio_cfg = config->GetConfig();

    ret = camera_init(vio_cfg, &vin_attr);
    if(ret) {
        printf("camera_init failed\n");
        return -1;
    }
    RingQueue<VIDEO_STREAM_S>::Instance().Init(2, [](VIDEO_STREAM_S &elem) {
        if (elem.pstPack.vir_ptr) {
            free(elem.pstPack.vir_ptr);
            elem.pstPack.vir_ptr = nullptr;
        }
    });
    HB_VENC_Module_Init();
    video_ctx = hb_video_alloc_context();
		if (!video_ctx) {
			printf("hb_video_alloc_context failed\n");
			return -1;
		}
    ret = uvc_init(uvc_ctx, video_ctx);
    if(ret) {
        printf("uvc_init failed\n");
        goto error;
    }

    running = 1;
    t = new std::thread(get_frame_thread, &vencparam);
	for(int i = 0; i<1000;i++){
		i = 100;
	}
	while (running) {
		sleep(2);
		printf("runing\n");
	}
	if (uvc_gadget_stop(uvc_ctx)) {
		printf("uvc_gadget_stop failed\n");
    }

    t->join();
	uvc_gadget_deinit(uvc_ctx);
	hb_video_free_context(video_ctx);
	camera_deinit(vio_cfg);
    HB_VENC_Module_Uninit();
	return 0;

error:
	camera_deinit(vio_cfg);
    HB_VENC_Module_Uninit();
	return -1;
}