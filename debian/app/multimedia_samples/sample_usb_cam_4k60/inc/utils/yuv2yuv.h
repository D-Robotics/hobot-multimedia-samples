/*
 * Copyright (C) 2020 zutao.min <zutao.min@horizon.ai>
 *
 * This file is include file use tranfer from yuv420nv12 to yuv422yuy2.
 */

#ifndef  LIB_YUV2YUV_INCLUDE_YUV2YUV_H_
#define  LIB_YUV2YUV_INCLUDE_YUV2YUV_H_
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#include <stdint.h>

/**
* tranfer yuv420nv12 to yuv422yuy2 use arm c
*
* @param[in]   nv12 data source addr
* @param[out]  yuy2 data store addr
* @param[in]   pixel width
* @param[in]   pixel height
*
* @return void
*/
void hb_yuv2yuv_nv12toyuy2_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix);

/**
* tranfer yuv420nv21 to yvyu use arm c
*
* @param[in]   nv12 data source addr
* @param[out]  yvyu data store addr
* @param[in]   pixel width
* @param[in]   pixel height
*
* @return void
*/
void hb_yuv2yuv_nv21toyvyu_c(const uint8_t *ySrc,
        const uint8_t *cbSrc, const uint8_t *crSrc,
        uint8_t *dst, int width, int height, int bytePerPix);

/**
* tranfer yuv420nv12 to yuv422yuy2 use neon
*
* @param[in]   nv12 data source addr
* @param[out]  yuy2 data store addr
* @param[in]   pixel width
* @param[in]   pixel height
*
* @return void
*/
void hb_yuv2yuv_nv12toyuy2_neon(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix);

 /**
 * tranfer yuv420nv21 to yuv422yuy2 use neon
 *
 * @param[in]   nv21 data source addr
 * @param[out]  yvyu data store addr
 * @param[in]   pixel width
 * @param[in]   pixel height
 *
 * @return void
 */
void hb_yuv2yuv_nv21toyvyu_neon(const uint8_t *ySrc,
          const uint8_t *cbSrc, const uint8_t *crSrc,
          uint8_t *dst, int width, int height, int bytePerPix);

/**
* tranfer yuv420p to yuv422yuyv use neon
*
* @param[in]   yuv420p data source addr
* @param[out]  yuyv data store addr
* @param[in]   pixel width
* @param[in]   pixel height
*
* @return void
*/
void hb_yuv2yuv_420ptoyuyv_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix);

/**
* tranfer yuv422p to yuv422yuyv use neon
*
* @param[in]   yuv422p data source addr
* @param[out]  yuyv data store addr
* @param[in]   pixel width
* @param[in]   pixel height
*
* @return void
*/
void hb_yuv2yuv_422ptoyuyv_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix);

/**
* tranfer nv16 to yuv422yuyv use neon
*
* @param[in]   yuv422p data source addr
* @param[out]  yuyv data store addr
* @param[in]   pixel width
* @param[in]   pixel height
*
* @return void
*/
void hb_yuv2yuv_nv16toyuyv_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix);

/**
* tranfer nv16 to yuv422yuyv use neon
*
* @param[in]   yuv422p data source addr
* @param[out]  yuyv data store addr
* @param[in]   pixel width
* @param[in]   pixel height
*
* @return void
*/
void hb_yuv2yuv_nv61toyuyv_c(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height, int bytePerPix);

/**
* tranfer yuv420nv12 to yuv422yuy2 use 4-thread bind on 4-cpu cores use neon
*
* @param[in]   nv12 data source addr
* @param[out]  yuy2 data store addr
* @param[in]   pixel width
* @param[in]   pixel height
*
* @return 0: sucess, -1: failed
*/
int hb_yuv2yuv_nv12toyuy2_multithread_neon(const uint8_t *ySrc,
         const uint8_t *cbSrc, const uint8_t *crSrc,
         uint8_t *dst, int width, int height);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif  // LIB_YUV2YUV_INCLUDE_YUV2YUV_H_
