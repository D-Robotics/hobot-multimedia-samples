#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>


#include "hb_comm_venc.h"
#include "hb_venc.h"
#include "hb_vio_interface.h"
#include "hb_sys.h"
#include "ion.h"

// H264 H265 MJPEG

typedef struct {
  int picWidth;    /*the attribute of video encoder*/
  int picHeight;   /*the attribute of rate  ctrl*/
  PAYLOAD_TYPE_E enType; /*the attribute of gop*/
  int channel;
  char* picFile0;
  char* picFile1;
  pthread_mutex_t init_lock;
  pthread_cond_t init_cond;
  int initialized; /*init single to sync pthread*/
} SAMPLE_VENC_ATTR_S;

int running = 0;

int ion_alloc_phy(int size, int *fd, char **vaddr, uint64_t * paddr);

void signal_handler(int signal_number);

void print_usage();

void *feed_encode_data(void *attr);

void *get_encode_data(void *attr);

int sample_venc_ChnAttr_init(VENC_CHN_ATTR_S *pVencChnAttr, 
        PAYLOAD_TYPE_E enType, int picWidth, int picHeight);

int sample_venc_setRcParam(int chn, VENC_CHN_ATTR_S *pVencChnAttr, 
        int bitRate);

int prepare_user_buf_2lane(void *buf, uint32_t size_y, uint32_t size_uv);

int read_nv12file(hb_vio_buffer_t *vio_buf, char* picFile, 
        int width, int height);

int main(int argc, char **argv) {
    int s32Ret;
    int opt = 0;
    SAMPLE_VENC_ATTR_S sample_attr;
    pthread_t producer, consumer;

    signal(SIGINT, signal_handler);
    if (argc != 11) {
        print_usage();
        return -1;
    }
    while((opt = getopt(argc,argv,"w:h:t:f:g:")) != -1) {
        switch(opt) {
            case 'w':
                sample_attr.picWidth = atoi(optarg);
                break;
            case 'h':
                sample_attr.picHeight = atoi(optarg);
                break;
            case 't':
                if (strcmp(optarg, "h264") == 0)
                    sample_attr.enType = PT_H264;
                else if (strcmp(optarg, "h265") == 0)
                    sample_attr.enType = PT_H265;
                else if (strcmp(optarg, "jpeg") == 0)
                    sample_attr.enType = PT_JPEG;
                else {
                    print_usage();
                    return -1;
                }
                break;
            case 'f':
                sample_attr.picFile0 = optarg;
                break;
            case 'g':
                sample_attr.picFile1 = optarg;
                break;
            default:
                print_usage();
                return -1;
        }
    }
    running = 1;
    sample_attr.channel = 0;
    pthread_mutex_init(&sample_attr.init_lock, NULL);  
    pthread_cond_init(&sample_attr.init_cond, NULL);
    s32Ret = HB_VENC_Module_Init();
    if (s32Ret) {
        printf("HB_VENC_Module_Init: %d\n", s32Ret);
    }

    s32Ret = pthread_create(&producer, NULL, feed_encode_data, &sample_attr);
    if (s32Ret != 0) {
        printf("producer creat failed\n");
        return 0;
    }
    usleep(10*1000);
    s32Ret = pthread_create(&consumer, NULL, get_encode_data, &sample_attr);
    if (s32Ret != 0) {
        printf("consumer creat failed\n");
        return 0;
    }

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    s32Ret = HB_VENC_Module_Uninit();
    if (s32Ret) {
        printf("HB_VENC_Module_Init: %d\n", s32Ret);
    }

    printf("Done\n");
    return 0;
}

void signal_handler(int signal_number)
{
	running = 0;
}

void print_usage() {
    printf("Usage: \n \
./sample_venc_basic -w width -h height -t ecode_type -f file0 \
-g file1\n\
encode_type can be: h264,h265,jpeg\n");
}

void *feed_encode_data(void *attr) {
    int s32Ret;
    SAMPLE_VENC_ATTR_S *sample_attr = (SAMPLE_VENC_ATTR_S *)attr;
    VENC_CHN_ATTR_S vencChnAttr;
    int picWidth = sample_attr->picWidth;
    int picHeight = sample_attr->picHeight;
    PAYLOAD_TYPE_E enType = sample_attr->enType;
    int vencChn = sample_attr->channel;
    VIDEO_FRAME_S pstFrame0, pstFrame1;
	hb_vio_buffer_t in_buffer0, in_buffer1;
    int pts = 0;
    int count = 0;

    printf("feed encode data thread running\n");
    pthread_mutex_lock(&sample_attr->init_lock);
    // 初始化channel属性
    s32Ret = sample_venc_ChnAttr_init(&vencChnAttr, enType, picWidth,
                picHeight);
    if (s32Ret) {
        printf("sample_venc_ChnAttr_init failded: %d\n", s32Ret);
    }
    // 创建channel
    s32Ret = HB_VENC_CreateChn(vencChn, &vencChnAttr);
    if (s32Ret != 0) {
        printf("HB_VENC_CreateChn %d failed, %x.\n", vencChn, s32Ret);
        return NULL;
    }
    // 配置Rc参数
    s32Ret = sample_venc_setRcParam(vencChn, &vencChnAttr, 8000);
    if (s32Ret) {
        printf("sample_venc_setRcParam failded: %d\n", s32Ret);
    }
    // 设置channel属性
    s32Ret = HB_VENC_SetChnAttr(vencChn, &vencChnAttr);  // config
    if (s32Ret != 0) {
        printf("HB_VENC_SetChnAttr failed\n");
        return NULL;
    }
    VENC_RECV_PIC_PARAM_S pstRecvParam;
    pstRecvParam.s32RecvPicNum = 0;  // unchangable

    s32Ret = HB_VENC_StartRecvFrame(vencChn, &pstRecvParam);
    if (s32Ret != 0) {
        printf("HB_VENC_StartRecvFrame failed\n");
        return NULL;
    }
    sample_attr->initialized = 1; // set feed pthread single = 1
    pthread_cond_signal(&sample_attr->init_cond);
    pthread_mutex_unlock(&sample_attr->init_lock);  

    // 读取两张NV12格式图片
	char file_name[100] = { 0 };
    read_nv12file(&in_buffer0, sample_attr->picFile0, picWidth, picHeight);
    read_nv12file(&in_buffer1, sample_attr->picFile1, picWidth, picHeight);
    
	pstFrame0.stVFrame.width = picWidth;
	pstFrame0.stVFrame.height = picHeight;
	pstFrame0.stVFrame.size = picWidth * picHeight * 3 / 2;
	pstFrame0.stVFrame.pix_format = HB_PIXEL_FORMAT_NV12;
	pstFrame0.stVFrame.phy_ptr[0] = in_buffer0.img_addr.paddr[0];
	pstFrame0.stVFrame.phy_ptr[1] = in_buffer0.img_addr.paddr[1];
	pstFrame0.stVFrame.vir_ptr[0] = in_buffer0.img_addr.addr[0];
	pstFrame0.stVFrame.vir_ptr[1] = in_buffer0.img_addr.addr[1];
	pstFrame0.stVFrame.pts = pts;

	pstFrame1.stVFrame.width = picWidth;
	pstFrame1.stVFrame.height = picHeight;
	pstFrame1.stVFrame.size = picWidth * picHeight * 3 / 2;
	pstFrame1.stVFrame.pix_format = HB_PIXEL_FORMAT_NV12;
	pstFrame1.stVFrame.phy_ptr[0] = in_buffer1.img_addr.paddr[0];
	pstFrame1.stVFrame.phy_ptr[1] = in_buffer1.img_addr.paddr[1];
	pstFrame1.stVFrame.vir_ptr[0] = in_buffer1.img_addr.addr[0];
	pstFrame1.stVFrame.vir_ptr[1] = in_buffer1.img_addr.addr[1];
	pstFrame1.stVFrame.pts = pts;

    while (running) {
        // 2s交替显示两张图片
        if ((count++ / 60) % 2 == 0)
		{
			pstFrame0.stVFrame.pts = pts;
            s32Ret = HB_VENC_SendFrame(vencChn, &pstFrame0, 3000);
		}
		else
		{
            pstFrame1.stVFrame.pts = pts;
            s32Ret = HB_VENC_SendFrame(vencChn, &pstFrame1, 3000);
		}
		if (s32Ret != 0)
		{
			printf("HB_VENC_SendFrame. ret=%d\n", s32Ret);
            usleep(33 * 1000);
			continue;
		}
        pts++;
		usleep(33 * 1000);
    }

    s32Ret = HB_VENC_StopRecvFrame(vencChn);
    if (s32Ret != 0) {
        printf("HB_VENC_StopRecvFrame failed\n");
        return NULL;
    }
    s32Ret = HB_VENC_DestroyChn(vencChn);
    if (s32Ret != 0) {
        printf("HB_VENC_DestroyChn failed\n");
        return NULL;
    }
}

void *get_encode_data(void *attr) {
    int s32Ret;
    SAMPLE_VENC_ATTR_S *sample_attr = (SAMPLE_VENC_ATTR_S *)attr;
    VENC_CHN_ATTR_S vencChnAttr;
    int picWidth = sample_attr->picWidth;
    int picHeight = sample_attr->picHeight;
    PAYLOAD_TYPE_E enType = sample_attr->enType;
    int vencChn = sample_attr->channel;
    FILE *outFile;
    VIDEO_STREAM_S pstStream;
    struct timeval now;
    struct timespec outtime;

    printf("get encode data thread running\n");
    if (enType == PT_H264) {
        outFile = fopen("sample_venc.h264", "wb");
    } else if (enType == PT_H265) {
        outFile = fopen("sample_venc.h265", "wb");
    } else {
        outFile = fopen("sample_venc.jpg", "wb");
    }
    // wait for set ready
    pthread_mutex_lock(&sample_attr->init_lock); 
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + 1;
    outtime.tv_nsec = now.tv_usec * 1000;
    while (!sample_attr->initialized && running) {
        pthread_cond_timedwait(&sample_attr->init_cond, &sample_attr->init_lock, &outtime);
    }
    pthread_mutex_unlock(&sample_attr->init_lock); 
    while (running) {
        s32Ret = HB_VENC_GetStream(vencChn, &pstStream, 2000);
        if (s32Ret != 0) {
            printf("HB_VENC_GetStream failed. ret=%d\n", s32Ret);
			continue;
        }
        fwrite(pstStream.pstPack.vir_ptr,
                        pstStream.pstPack.size, 1, outFile);
        s32Ret = HB_VENC_ReleaseStream(vencChn, &pstStream);
        if (s32Ret != 0)
        {
            printf("HB_VENC_ReleaseStream failed\n");
        }
        if (enType == PT_JPEG) {
            running = 0;
            break;
        }
            
    }
    fclose(outFile);

}

int sample_venc_ChnAttr_init(VENC_CHN_ATTR_S *pVencChnAttr,
        PAYLOAD_TYPE_E enType, int picWidth, int picHeight) {
    int streambufSize = 0;
    if (pVencChnAttr == NULL) {
        printf("pVencChnAttr is NULL!\n");
        return -1;
    }
    // 该步骤必不可少
    memset(pVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    // 设置编码模型分别为 PT_H264 PT_H265 PT_MJPEG 
    pVencChnAttr->stVencAttr.enType = enType;
    // 设置编码分辨率
    pVencChnAttr->stVencAttr.u32PicWidth = picWidth;
    pVencChnAttr->stVencAttr.u32PicHeight = picHeight;
    // 设置像素格式 NV12格式
    pVencChnAttr->stVencAttr.enPixelFormat = HB_PIXEL_FORMAT_NV12;
    // 配置图像镜像属性 无镜像
    pVencChnAttr->stVencAttr.enMirrorFlip = DIRECTION_NONE;
    // 配置图像旋转属性 不旋转
    pVencChnAttr->stVencAttr.enRotation = CODEC_ROTATION_0;
    // 配置图像剪裁属性 不剪裁
    pVencChnAttr->stVencAttr.stCropCfg.bEnable = HB_FALSE;
    // 输入图像大小 1024对齐
    streambufSize = (picWidth * picHeight * 3 / 2 + 1024) & ~0x3ff;
    // vlc_buf_size为经验值，可以减少RAM使用，如果想使用默认值则保持为0
    if (picWidth * picHeight > 2688 * 1522) {
        pVencChnAttr->stVencAttr.vlc_buf_size = 7900 * 1024;
    } else if (picWidth * picHeight > 1920 * 1080) {
        pVencChnAttr->stVencAttr.vlc_buf_size = 4 * 1024 * 1024;
    } else if (picWidth * picHeight > 1280 * 720) {
        pVencChnAttr->stVencAttr.vlc_buf_size = 2100 * 1024;
    } else if (picWidth * picHeight > 704 * 576) {
        pVencChnAttr->stVencAttr.vlc_buf_size = 2100 * 1024;
    } else {
        pVencChnAttr->stVencAttr.vlc_buf_size = 2048 * 1024;
    }
    if (enType == PT_JPEG || enType == PT_MJPEG) {
        // 输出码流个数
        pVencChnAttr->stVencAttr.u32BitStreamBufferCount = 1;
        // 输入图像个数
        pVencChnAttr->stVencAttr.u32FrameBufferCount = 2;
        pVencChnAttr->stVencAttr.bExternalFreamBuffer = HB_TRUE;
        // 关闭DCF
        pVencChnAttr->stVencAttr.stAttrJpeg.dcf_enable = HB_FALSE;
        // 质量系数，越小质量越好
        pVencChnAttr->stVencAttr.stAttrJpeg.quality_factor = 0;
        // 配置为0 不建议更改
        pVencChnAttr->stVencAttr.stAttrJpeg.restart_interval = 0;
        // 4096对齐
        pVencChnAttr->stVencAttr.u32BitStreamBufSize = (streambufSize + 4096) & ~4095;
    } else {
        // 输出码流个数
        pVencChnAttr->stVencAttr.u32BitStreamBufferCount = 3;
        // 输入图像个数
        pVencChnAttr->stVencAttr.u32FrameBufferCount = 3;
        pVencChnAttr->stVencAttr.bExternalFreamBuffer = HB_TRUE;
        pVencChnAttr->stVencAttr.u32BitStreamBufSize = streambufSize;
    }

    if (enType == PT_H265) {
        // 配置编码模式为H265VBR
        pVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
        // 不使能QpMap
        pVencChnAttr->stRcAttr.stH265Vbr.bQpMapEnable = HB_FALSE;
        // 设置I帧Qp值
        pVencChnAttr->stRcAttr.stH265Vbr.u32IntraQp = 20;
        // 设置I帧间隔
        pVencChnAttr->stRcAttr.stH265Vbr.u32IntraPeriod = 60;
        // 设置帧率
        pVencChnAttr->stRcAttr.stH265Vbr.u32FrameRate = 30;
    }
    if (enType == PT_H264) {
        // 配置编码模式为H264VBR
        pVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
        // 不使能QpMap
        pVencChnAttr->stRcAttr.stH264Vbr.bQpMapEnable = HB_FALSE;
        // 设置I帧Qp值
        pVencChnAttr->stRcAttr.stH264Vbr.u32IntraQp = 20;
        // 设置I帧间隔
        pVencChnAttr->stRcAttr.stH264Vbr.u32IntraPeriod = 60;
        // 设置帧率
        pVencChnAttr->stRcAttr.stH264Vbr.u32FrameRate = 30;
        // h264_profile为0，系统自动配置
        pVencChnAttr->stVencAttr.stAttrH264.h264_profile = 0;
        // h264_level为0，系统自动配置
        pVencChnAttr->stVencAttr.stAttrH264.h264_level = 0;
    }
    // 设置GOP结构
    pVencChnAttr->stGopAttr.u32GopPresetIdx = 2;
    // 设置IDR帧类型
    pVencChnAttr->stGopAttr.s32DecodingRefreshType = 2;
    return 0;
}

int sample_venc_setRcParam(int chn, VENC_CHN_ATTR_S *pVencChnAttr, 
        int bitRate) {
    VENC_RC_ATTR_S *pstRcParam;
    int s32Ret;

    if (pVencChnAttr->stVencAttr.enType == PT_H264) {
        pstRcParam = &(pVencChnAttr->stRcAttr);
        // 为什么之前是VBR 这里改为CBR 之前的VBR是必须的吗？
        pVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
        s32Ret = HB_VENC_GetRcParam(chn, pstRcParam);
        if (s32Ret != 0) {
            printf("HB_VENC_GetRcParam failed.\n");
            return -1;
        }

        printf(" vencChnAttr.stRcAttr.enRcMode = %d mmmmmmmmmmmmmmmmmm   \n",
                pVencChnAttr->stRcAttr.enRcMode);
        printf(" u32VbvBufferSize = %d mmmmmmmmmmmmmmmmmm   \n",
                pVencChnAttr->stRcAttr.stH264Cbr.u32VbvBufferSize);
        // 设置码率
        pstRcParam->stH264Cbr.u32BitRate = bitRate;
        // 设置帧率
        pstRcParam->stH264Cbr.u32FrameRate = 30;
        // 设置I帧间隔
        pstRcParam->stH264Cbr.u32IntraPeriod = 60;
        // 设置VbvBufferSize，与码率、解码器速率有关，一般在1000～5000
        pstRcParam->stH264Cbr.u32VbvBufferSize = 3000;
    } else if (pVencChnAttr->stVencAttr.enType == PT_H265) {
        pstRcParam = &(pVencChnAttr->stRcAttr);
        pVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
        s32Ret = HB_VENC_GetRcParam(chn, pstRcParam);
        if (s32Ret != 0) {
            printf("HB_VENC_GetRcParam failed.\n");
            return -1;
        }
        printf(" m_VencChnAttr.stRcAttr.enRcMode = %d mmmmmmmmmmmmmmmmmm   \n",
                pVencChnAttr->stRcAttr.enRcMode);
        printf(" u32VbvBufferSize = %d mmmmmmmmmmmmmmmmmm   \n",
                pVencChnAttr->stRcAttr.stH265Cbr.u32VbvBufferSize);
        // 设置码率
        pstRcParam->stH265Cbr.u32BitRate = bitRate;
        // 设置帧率
        pstRcParam->stH265Cbr.u32FrameRate = 30;
        // 设置I帧间隔
        pstRcParam->stH265Cbr.u32IntraPeriod = 30;
        // 设置VbvBufferSize，与码率、解码器速率有关，一般在1000～5000
        pstRcParam->stH265Cbr.u32VbvBufferSize = 3000;
    }
    return 0;
}

int read_nv12file(hb_vio_buffer_t *vio_buf, char* picFile, 
        int width, int height) {
    uint32_t size_y, size_uv;
    size_t read_size;
    int img_in_fd;
    memset(vio_buf, 0, sizeof(hb_vio_buffer_t));
    size_y = width * height;
    size_uv = size_y / 2;
    prepare_user_buf_2lane(vio_buf, size_y, size_uv);
    vio_buf->img_info.planeCount = 2;
    vio_buf->img_info.img_format = 8;
    vio_buf->img_addr.width = width;
    vio_buf->img_addr.height = height;
    vio_buf->img_addr.stride_size = width;

    img_in_fd = open(picFile, O_RDWR | O_CREAT, 0644);
    if (img_in_fd < 0) {
        printf("open image:%s failed !\n", picFile);
        return -1;
    }

    read_size = read(img_in_fd, vio_buf->img_addr.addr[0], size_y);
    usleep(10 * 1000);
    read_size = read(img_in_fd, vio_buf->img_addr.addr[1], size_uv);
    usleep(10 * 1000);
    close(img_in_fd);
}

int prepare_user_buf_2lane(void *buf, uint32_t size_y, uint32_t size_uv)
{
	int ret;
	hb_vio_buffer_t *buffer = (hb_vio_buffer_t *)buf;

	if (buffer == NULL)
		return -1;

	buffer->img_info.fd[0] = ion_open();
	buffer->img_info.fd[1] = ion_open();

	ret  = ion_alloc_phy(size_y, &buffer->img_info.fd[0],
						&buffer->img_addr.addr[0], &buffer->img_addr.paddr[0]);
	if (ret) {
		printf("prepare user buf error\n");
		return ret;
	}
	ret = ion_alloc_phy(size_uv, &buffer->img_info.fd[1],
						&buffer->img_addr.addr[1], &buffer->img_addr.paddr[1]);
	if (ret) {
		printf("prepare user buf error\n");
		return ret;
	}

	printf("buf:y: vaddr = 0x%lx paddr = 0x%lx; uv: vaddr = 0x%lx, paddr = 0x%lx\n",
				(uint64_t)buffer->img_addr.addr[0], (uint64_t)buffer->img_addr.paddr[0],
				(uint64_t)buffer->img_addr.addr[1], (uint64_t)buffer->img_addr.paddr[1]);
	return 0;
}
