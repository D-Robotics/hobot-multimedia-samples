/*
 ** Copyright (C) 2020 zutao.min <zutao.min@horizon.ai>
 **
 ** This file is use tranfer from yuv420nv12 to yuv422yuy2.
 **/

#define __USE_GNU
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arm_neon.h>
#include <pthread.h>

typedef struct {
	uint8_t *y_src;
	uint8_t *uv_src;
	uint8_t *dst_src;
	int width;
	int height;
} tran_frame_info_t;

static void *tranfer_fun(void *usr_data) {
    tran_frame_info_t *info = (tran_frame_info_t *)usr_data;

	int y, i;

	const int dim16_n = info->width >> 4;
	const uint8_t *uvc_base = info->uv_src;
	const uint8_t *src = info->y_src;
	uint8_t *dst = info->dst_src;
	int width = info->width;
	int height = info->height;

    for (y = 0; y < height; y++) {
        uint8_t*idst = dst;
        const uint8_t *yc = src;
        const uint8_t *uvc =  uvc_base + y/2 *width;

	    for (i = 0; i < dim16_n; i++) {
            uint8x16_t y_tmp = vld1q_u8(yc+i*16);
            uint8x16_t uv_tmp = vld1q_u8(uvc+i*16);
            uint8x16x2_t yuv_tmp = vzipq_u8(y_tmp, uv_tmp);
            vst1q_u8(idst+i*32,  yuv_tmp.val[0]);
            vst1q_u8(idst+i*32+16,  yuv_tmp.val[1]);
	    }

        src += width;
        dst  += width*2;
    }
    return NULL;
}

void hb_yuv2yuv_nv12toyuy2_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix) {
    int y, i;

    const int chromWidth = width >> 1;

    if (bytePerPix == 1) {
        const uint8_t *uvc_base = cbSrc;

        for (y = 0; y < height; y++) {
            uint32_t *idst = (uint32_t *)dst;
            const uint8_t *yc = ySrc;

            const uint8_t *uvc =  uvc_base + y/2 *width;

            for (i = 0; i < chromWidth; i++) {
                *idst++ = yc[0] + (uvc[0] << 8) +
                        (yc[1] << 16) + (uvc[1] << 24);
                yc += 2;
                uvc += 2;
            }
            ySrc += width;
            dst  += width*2;
        }
    } else if (bytePerPix == 2) {
        const uint16_t *uvc_base = (uint16_t *)cbSrc;
        uint16_t *yLuma_base = (uint16_t *)ySrc;

        for (y = 0; y < height; y++) {
            uint64_t *idst = (uint64_t *)dst;
            const uint16_t *yc = yLuma_base;

            const uint16_t *uvc =  uvc_base + y/2 *width;

            for (i = 0; i < chromWidth; i++) {
                *idst++ = (uint64_t)yc[0] +
                        ((uint64_t)uvc[0] << (8*bytePerPix)) +
                        ((uint64_t)yc[1] << (16*bytePerPix)) +
                        ((uint64_t)uvc[1] << (24*bytePerPix));
                yc += 2;
                uvc += 2;
            }
            yLuma_base += width;
            dst  += width *2 * bytePerPix;
        }
    }
}

void hb_yuv2yuv_nv21toyvyu_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix) {
    int y, i;

    const int chromWidth = width >> 1;

    if (bytePerPix == 1) {
        const uint8_t *uvc_base = cbSrc;

        for (y = 0; y < height; y++) {
            uint32_t *idst = (uint32_t *)dst;
            const uint8_t *yc = ySrc;

            const uint8_t *uvc =  uvc_base + y/2 *width;

            for (i = 0; i < chromWidth; i++) {
                *idst++ = yc[0] + (uvc[0] << 8) +
                        (yc[1] << 16) + (uvc[1] << 24);
                yc += 2;
                uvc += 2;
            }
            ySrc += width;
            dst  += width*2;
        }
    } else if (bytePerPix == 2) {
        const uint16_t *uvc_base = (uint16_t *)cbSrc;
        uint16_t *yLuma_base = (uint16_t *)ySrc;

        for (y = 0; y < height; y++) {
            uint64_t *idst = (uint64_t *)dst;
            const uint16_t *yc = yLuma_base;

            const uint16_t *uvc =  uvc_base + y/2 *width;

            for (i = 0; i < chromWidth; i++) {
                *idst++ = (uint64_t)yc[0] +
                        ((uint64_t)uvc[0] << (8*bytePerPix)) +
                        ((uint64_t)yc[1] << (16*bytePerPix)) +
                        ((uint64_t)uvc[1] << (24*bytePerPix));
                yc += 2;
                uvc += 2;
            }
            yLuma_base += width;
            dst  += width *2 * bytePerPix;
        }
    }
}

void hb_yuv2yuv_nv12toyuy2_neon(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix) {
    int y, i;

    // const int chromWidth = width >> 1;
    int sliceNum = width >> 4;
    int unit = 1<<4;

    if (bytePerPix == 1) {
        const uint8_t *uvc_base = cbSrc;
        for (y = 0; y < height; y++) {
            uint8_t*idst = dst;
            const uint8_t *yc = ySrc;
            const uint8_t *uvc =  uvc_base + y/2 *width;
            for (i = 0; i < sliceNum; i++) {
                uint8x16_t y_tmp = vld1q_u8(yc+i*unit);
                uint8x16_t uv_tmp = vld1q_u8(uvc+i*unit);
                uint8x16x2_t yuv_tmp = vzipq_u8(y_tmp, uv_tmp);
                vst1q_u8(idst+i*unit*2,  yuv_tmp.val[0]);
                vst1q_u8(idst+i*unit*2+unit,  yuv_tmp.val[1]);
            }
            ySrc += width;
            dst += width*2;
        }
    }
}

void hb_yuv2yuv_nv21toyvyu_neon(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix) {
    int y, i;

    // const int chromWidth = width >> 1;
    int sliceNum = width >> 4;
    int unit = 1 << 4;

    if (bytePerPix == 1) {
        const uint8_t *uvc_base = cbSrc;

        for (y = 0; y < height; y++) {
            uint8_t*idst = dst;
            const uint8_t *yc = ySrc;

            const uint8_t *uvc =  uvc_base + y/2 *width;

            for (i = 0; i < sliceNum; i++) {
                uint8x16_t y_tmp = vld1q_u8(yc+i*unit);
                uint8x16_t uv_tmp = vld1q_u8(uvc+i*unit);
                uint8x16x2_t yuv_tmp = vzipq_u8(y_tmp, uv_tmp);
                vst1q_u8(idst+i*unit*2,  yuv_tmp.val[0]);
                vst1q_u8(idst+i*unit*2+unit,  yuv_tmp.val[1]);
            }

            ySrc += width;
            dst += width*2;
        }
    }
}

void hb_yuv2yuv_420ptoyuyv_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix) {
    int y, i;

    const int chromWidth = width >> 1;
    if (bytePerPix == 1) {
        const uint8_t *uc_base = cbSrc;
        const uint8_t *vc_base = crSrc;
        for (y = 0; y < height; y++) {
            int *idst = (int32_t *)dst;
            const uint8_t *yc = ySrc;

            const uint8_t *uc =  uc_base + y/4 *width;
            const uint8_t *vc =  vc_base + y/4 *width;

            for (i = 0; i < chromWidth; i++) {
                 *idst++ = yc[0] + (uc[0] << 8) +
                         (yc[1] << 16) + (vc[0] << 24);
                 yc += 2;
                 uc += 1;
                 vc += 1;
            }
            ySrc += width;
            dst  += width*2;
        }
    } else if (bytePerPix == 2) {
        const uint16_t *uc_base = (uint16_t *)cbSrc;
        const uint16_t *vc_base = (uint16_t *)crSrc;
        uint16_t *yLuma_base = (uint16_t *)ySrc;

        for (y = 0; y < height; y++) {
            uint64_t *idst = (uint64_t *)dst;
            const uint16_t *yc = yLuma_base;

            const uint16_t *uc =  uc_base + y/4 *width;
            const uint16_t *vc =  vc_base + y/4 *width;

            for (i = 0; i < chromWidth; i++) {
                *idst++ = (uint64_t)yc[0] +
                        ((uint64_t)uc[0] << (8*bytePerPix)) +
                        ((uint64_t)yc[1] << (16*bytePerPix)) +
                        ((uint64_t)vc[0] << (24*bytePerPix));
                yc += 2;
                uc += 1;
                vc += 1;
            }
            yLuma_base += width;
            dst  += width *2 * bytePerPix;
        }
    }
}

void hb_yuv2yuv_422ptoyuyv_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix) {
    int y, i;

    const int chromWidth = width >> 1;

    if (bytePerPix == 1) {
        const uint8_t *uc_base = cbSrc;
        const uint8_t *vc_base = crSrc;

        for (y = 0; y < height; y++) {
            int *idst = (int32_t *)dst;
            const uint8_t *yc = ySrc;

            const uint8_t *uc =  uc_base + y/2 *width;
            const uint8_t *vc =  vc_base + y/2 *width;

            for (i = 0; i < chromWidth; i++) {
                 *idst++ = yc[0] + (uc[0] << 8) +
                         (yc[1] << 16) + (vc[0] << 24);
                 yc += 2;
                 uc += 1;
                 vc += 1;
            }
            ySrc += width;
            dst  += width*2;
        }
    } else if (bytePerPix == 2) {
        const uint16_t *uc_base = (uint16_t *)cbSrc;
        const uint16_t *vc_base = (uint16_t *)crSrc;
        uint16_t *yLuma_base = (uint16_t *)ySrc;

        for (y = 0; y < height; y++) {
            uint64_t *idst = (uint64_t *)dst;
            const uint16_t *yc = yLuma_base;

            const uint16_t *uc =  uc_base + y/2 *width;
            const uint16_t *vc =  vc_base + y/2 *width;

            for (i = 0; i < chromWidth; i++) {
                *idst++ = (uint64_t)yc[0] +
                        ((uint64_t)uc[0] << (8*bytePerPix)) +
                        ((uint64_t)yc[1] << (16*bytePerPix)) +
                        ((uint64_t)vc[0] << (24*bytePerPix));
                yc += 2;
                uc += 1;
                vc += 1;
            }
            yLuma_base += width;
            dst  += width *2 * bytePerPix;
        }
    }
}

void hb_yuv2yuv_nv16toyuyv_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix) {
    int y, i;

    const int chromWidth = width >> 1;

    if (bytePerPix == 1) {
        const uint8_t *uvc_base = cbSrc;

        for (y = 0; y < height; y++) {
            int *idst = (int32_t *)dst;
            const uint8_t *yc = ySrc;

            const uint8_t *uvc =  uvc_base + y * width;

            for (i = 0; i < chromWidth; i++) {
                 *idst++ = yc[0] + (uvc[0] << 8) +
                         (yc[1] << 16) + (uvc[1] << 24);
                 yc += 2;
                 uvc += 2;
            }
            ySrc += width;
            dst  += width*2;
        }
    } else if (bytePerPix == 2) {
        const uint16_t *uvc_base = (uint16_t *)cbSrc;
        uint16_t *yLuma_base = (uint16_t *)ySrc;

        for (y = 0; y < height; y++) {
            uint64_t *idst = (uint64_t *)dst;
            const uint16_t *yc = yLuma_base;

            const uint16_t *uvc =  uvc_base + y * width;

            for (i = 0; i < chromWidth; i++) {
                *idst++ = (uint64_t)yc[0] +
                        ((uint64_t)uvc[0] << (8*bytePerPix)) +
                        ((uint64_t)yc[1] << (16*bytePerPix)) +
                        ((uint64_t)uvc[1] << (24*bytePerPix));
                yc += 2;
                uvc += 2;
            }
            yLuma_base += width;
            dst  += width *2 * bytePerPix;
        }
    }
}

void hb_yuv2yuv_nv61toyuyv_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix) {
    int y, i;

    const int chromWidth = width >> 1;

    if (bytePerPix == 1) {
        const uint8_t *uvc_base = cbSrc;

        for (y = 0; y < height; y++) {
            int *idst = (int32_t *)dst;
            const uint8_t *yc = ySrc;

            const uint8_t *uvc =  uvc_base + y * width;

            for (i = 0; i < chromWidth; i++) {
                 *idst++ = yc[0] + (uvc[1] << 8) +
                         (yc[1] << 16) + (uvc[0] << 24);
                 yc += 2;
                 uvc += 2;
            }
            ySrc += width;
            dst  += width*2;
        }
    } else if (bytePerPix == 2) {
        const uint16_t *uvc_base = (uint16_t *)cbSrc;
        uint16_t *yLuma_base = (uint16_t *)ySrc;

        for (y = 0; y < height; y++) {
            uint64_t *idst = (uint64_t *)dst;
            const uint16_t *yc = yLuma_base;

            const uint16_t *uvc =  uvc_base + y * width;

            for (i = 0; i < chromWidth; i++) {
                *idst++ = (uint64_t)yc[0] +
                        ((uint64_t)uvc[1] << (8*bytePerPix)) +
                        ((uint64_t)yc[1] << (16*bytePerPix)) +
                        ((uint64_t)uvc[0] << (24*bytePerPix));
                yc += 2;
                uvc += 2;
            }
            yLuma_base += width;
            dst  += width *2 * bytePerPix;
        }
    }
}

// Need to modify the cbcr address
int hb_yuv2yuv_nv12toyuy2_multithread_neon(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height) {
    pthread_t multi_thread_1;
    pthread_t multi_thread_2;
    pthread_t multi_thread_3;
    pthread_t multi_thread_4;

    pthread_attr_t attr1;
    pthread_attr_t attr2;
    pthread_attr_t attr3;
    pthread_attr_t attr4;

    tran_frame_info_t pth_arg1;
    tran_frame_info_t pth_arg2;
    tran_frame_info_t pth_arg3;
    tran_frame_info_t pth_arg4;

    int y_w = width;
    int y_h = height;
    uint8_t *yuv420_src = (uint8_t*)ySrc;
    uint8_t *yuy2_dst = dst;

    pthread_attr_init(&attr1);
    pthread_attr_init(&attr2);
    pthread_attr_init(&attr3);
    pthread_attr_init(&attr4);

    cpu_set_t cpu_info;
    CPU_ZERO(&cpu_info);
    CPU_SET(0, &cpu_info);
    if (0 != pthread_attr_setaffinity_np(&attr1, sizeof(cpu_set_t),
                                         &cpu_info)) {
        printf("set attr1 affinity failed");
        return -1;
    }

    CPU_ZERO(&cpu_info);
    CPU_SET(1, &cpu_info);
    if (0 != pthread_attr_setaffinity_np(&attr2, sizeof(cpu_set_t),
                                         &cpu_info)) {
        printf("set attr2 affinity failed");
        return -1;
    }

    CPU_ZERO(&cpu_info);
    CPU_SET(2, &cpu_info);
    if (0 != pthread_attr_setaffinity_np(&attr3, sizeof(cpu_set_t),
                                         &cpu_info)) {
        printf("set attr3 affinity failed");
        return -1;
    }

    CPU_ZERO(&cpu_info);
    CPU_SET(3, &cpu_info);
    if (0 != pthread_attr_setaffinity_np(&attr4, sizeof(cpu_set_t),
                                         &cpu_info)) {
        printf("set attr4 affinity failed");
        return -1;
    }

    pth_arg1.y_src = yuv420_src;
    pth_arg1.uv_src = yuv420_src + y_w*y_h;
    pth_arg1.dst_src = yuy2_dst;
    pth_arg1.width = y_w;
    pth_arg1.height = y_h >> 2;
    if (0 != pthread_create(&multi_thread_1, &attr1,
                            tranfer_fun, (void*)(&pth_arg1))) {
        printf("Create YUV Tranfer Thread1 failed:%s!\n", strerror(errno));
        return -1;
    }

    pth_arg2.y_src = yuv420_src + y_w*(y_h >> 2);
    pth_arg2.uv_src = yuv420_src + y_w*y_h + y_w*(y_h >> 3);
    pth_arg2.dst_src = yuy2_dst + y_w*(y_h >> 1);
    pth_arg2.width = y_w;
    pth_arg2.height = y_h >> 2;
    if (0 != pthread_create(&multi_thread_2, &attr2,
                            tranfer_fun, (void*)(&pth_arg2))) {
        printf("Create YUV Tranfer Thread2 failed:%s!\n", strerror(errno));
        pthread_join(multi_thread_1, NULL);
        return -1;
    }

    pth_arg3.y_src = yuv420_src + y_w*(y_h >> 1);;
    pth_arg3.uv_src = yuv420_src + y_w*y_h + y_w*(y_h >> 2);
    pth_arg3.dst_src = yuy2_dst + y_w*y_h;
    pth_arg3.width = y_w;
    pth_arg3.height = y_h >> 2;
    if (0 != pthread_create(&multi_thread_3, &attr3,
                            tranfer_fun, (void*)(&pth_arg3))) {
        printf("Create YUV Tranfer Thread failed:%s!\n", strerror(errno));
        pthread_join(multi_thread_1, NULL);
        pthread_join(multi_thread_2, NULL);
        return -1;
    }

    pth_arg4.y_src = yuv420_src + y_w*(y_h * 3 / 4);
    pth_arg4.uv_src = yuv420_src + y_w*y_h + y_w*(y_h * 3 / 8);
    pth_arg4.dst_src = yuy2_dst + y_w*y_h*3/2;
    pth_arg4.width = y_w;
    pth_arg4.height = y_h >> 2;
    if (0 != pthread_create(&multi_thread_4, &attr4,
                            tranfer_fun, (void*)(&pth_arg4))) {
        printf("Create YUV Tranfer Thread4 failed:%s!\n", strerror(errno));
        pthread_join(multi_thread_1, NULL);
        pthread_join(multi_thread_2, NULL);
        pthread_join(multi_thread_3, NULL);
        return -1;
    }

    pthread_join(multi_thread_1, NULL);
    pthread_join(multi_thread_2, NULL);
    pthread_join(multi_thread_3, NULL);
    pthread_join(multi_thread_4, NULL);

    return 0;
}
