#include <fstream>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "hb_type.h"
#include "hb_common.h"
#include "utils/utils.h"

static int dumpToFile2plane(char *filename, char *srcBuf, char *srcBuf1,
        unsigned int size, unsigned int size1,
        int width, int height, int stride)
{
    FILE *yuvFd = NULL;
    char *buffer = NULL;
    int i = 0;

    yuvFd = fopen(filename, "w+");

    if (yuvFd == NULL) {
        printf("open(%s) fail", filename);
        return -1;
    }

    buffer = reinterpret_cast<char *>(malloc(size + size1));

    if (buffer == NULL) {
        printf("malloc falied");
        fclose(yuvFd);
        return -1;
    }

    if (width == stride) {
        memcpy(buffer, srcBuf, size);
        memcpy(buffer + size, srcBuf1, size1);
    } else {
        // jump over stride - width Y
        for (i = 0; i < height; i++) {
            memcpy(buffer+i*width, srcBuf+i*stride, width);
        }

        // jump over stride - width UV
        for (i = 0; i < height/2; i++) {
            memcpy(buffer+size+i*width, srcBuf1+i*stride, width);
        }
    }

    fflush(stdout);

    fwrite(buffer, 1, size + size1, yuvFd);

    fflush(yuvFd);

    if (yuvFd)
        fclose(yuvFd);
    if (buffer)
        free(buffer);

    printf("filedump(%s, size(%d) is successed!!\n", filename, size);

    return 0;
}

void dump_pym_to_files(pym_buffer_t *pvio_image, int chn, int layer) {
    static int count = 0;
    char file_name[100] = {0};
    snprintf(file_name,
            sizeof(file_name),
            "grp%d_chn%d_pym_layer_DS%d_%d_%d_%d.yuv",
            0, chn, layer,
            pvio_image->pym[layer].width,
            pvio_image->pym[layer].height, count);
    printf("grp%d_chn%d_pym_layer_DS%d_%d_%d_%d.yuv, strid:%d\n",
            0, chn, layer,
            pvio_image->pym[layer].width,
            pvio_image->pym[layer].height, count, pvio_image->pym[layer].stride_size);
    dumpToFile2plane(
            file_name,
            pvio_image->pym[layer].addr[0],
            pvio_image->pym[layer].addr[1],
            pvio_image->pym[layer].width * pvio_image->pym[layer].height,
            pvio_image->pym[layer].width * pvio_image->pym[layer].height / 2,
            pvio_image->pym[layer].width,
            pvio_image->pym[layer].height,
            pvio_image->pym[layer].stride_size);
    count++;
}

void dump_pym_us_to_files(pym_buffer_t *pvio_image, int chn, int layer) {
    static int count = 0;
    char file_name[100] = {0};
    snprintf(file_name,
            sizeof(file_name),
            "grp%d_chn%d_pym_layer_DS%d_%d_%d_%d.yuv",
            0, chn, layer,
            pvio_image->us[layer].width,
            pvio_image->us[layer].height, count);
    printf("grp%d_chn%d_pym_layer_DS%d_%d_%d_%d.yuv, strid:%d\n",
            0, chn, layer,
            pvio_image->us[layer].width,
            pvio_image->us[layer].height, count, pvio_image->us[layer].stride_size);
    dumpToFile2plane(
            file_name,
            pvio_image->us[layer].addr[0],
            pvio_image->us[layer].addr[1],
            pvio_image->us[layer].width * pvio_image->us[layer].height,
            pvio_image->us[layer].width * pvio_image->us[layer].height / 2,
            pvio_image->us[layer].width,
            pvio_image->us[layer].height,
            pvio_image->us[layer].stride_size);
    count++;
}

void dump_roi_pym_to_files(pym_buffer_t *pvio_image, int chn,
        int main_layer, int sub_layer) {
    static int count = 0;
    char file_name[100] = {0};
    auto w = pvio_image->pym_roi[main_layer][sub_layer].width;
    auto h = pvio_image->pym_roi[main_layer][sub_layer].height;

    snprintf(file_name, sizeof(file_name),
            "grp%d_chn%d_pym_layer_DS%d_%d_%d_%d.yuv",
            0, chn, 4 * main_layer + sub_layer, w, h, count);
    dumpToFile2plane(
            file_name,
            pvio_image->pym_roi[main_layer][sub_layer].addr[0],
            pvio_image->pym_roi[main_layer][sub_layer].addr[1],
            w * h, w * h / 2, w, h,
            w);
    count++;
}

void dump_nv12_to_file(const char* file_name, address_info_t *addr_info) {
    std::ofstream outfile(file_name, std::ofstream::ate);
    if (outfile.fail()) {
        printf("out file failed\n");
        return;
    }
    outfile.write(addr_info->addr[0], addr_info->width * addr_info->height);
    outfile.write(addr_info->addr[1], addr_info->width * addr_info->height / 2);
    outfile.close();
}

void dump_stream_to_file(const char* ptr, size_t size) {
    std::ofstream outfile("./stream.h264", std::ofstream::app);
    if (outfile.fail()) {
        printf("out file failed\n");
        return;
    }
    outfile.write(ptr, size);
    outfile.close();
}

void dump_jpg_to_file(const char* ptr, size_t size, int chn, int layer) {
    static int count = 0;
    char file_name[100] = {0};
    snprintf(file_name,
            sizeof(file_name),
            "grp%d_chn%d_pym_layer_DS%d_%d.jpg",
            0, chn, layer, count);

    std::ofstream outfile(file_name, std::ofstream::ate);
    if (outfile.fail()) {
        printf("out file failed\n");
        return;
    }
    outfile.write(ptr, size);
    outfile.close();
    count++;
}

int AV_open_stream_file(char *FileName, AVFormatContext **avContext,
    AVPacket *avpacket) {
    int ret = 0;

    ret = avformat_open_input(avContext, FileName, 0, 0);
    if (ret < 0) {
        printf("avformat_open_input failed\n");
        return -1;
    }
    ret = avformat_find_stream_info(*avContext, 0);
    if (ret < 0) {
        printf("avformat_find_stream_info failed\n");
        return -1;
    }
	printf("probesize: %ld\n", (*avContext)->probesize);

	/* dump input information to stderr */
	av_dump_format(*avContext, 0, FileName, 0);
    int index = av_find_best_stream(*avContext, AVMEDIA_TYPE_VIDEO,
            -1, -1, NULL, 0);
    if (index < 0) {
        printf("av_find_best_stream failed, ret: %d\n", index);
        return -1;
    }
    av_init_packet(avpacket);
	// avpacket->data = NULL;
	// avpacket->size = 0;

    return index;
}

#define SET_BYTE(_p, _b) \
    *_p++ = (unsigned char)_b;

#define SET_BUFFER(_p, _buf, _len) \
    memcpy(_p, _buf, _len); \
    (_p) += (_len);

static int build_dec_seq_header(uint8_t *        pbHeader,
    const PAYLOAD_TYPE_E p_enType, const AVStream* st, int* sizelength)
{
    AVCodecParameters* avc = st->codecpar;

    uint8_t* pbMetaData = avc->extradata;
    int nMetaData = avc->extradata_size;
    uint8_t* p =    pbMetaData;
    uint8_t *a =    p + 4 - ((long) p & 3);
    uint8_t* t =    pbHeader;
    int         size;
    int         sps, pps, i, nal;

    size = 0;
    *sizelength = 4;  // default size length(in bytes) = 4
    if (p_enType == PT_H264) {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01) {
            // check mov/mo4 file format stream
            p += 4;
            *sizelength = (*p++ & 0x3) + 1;
            sps = (*p & 0x1f);  // Number of sps
            p++;
            for (i = 0; i < sps; i++) {
                nal = (*p << 8) + *(p + 1) + 2;
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x01);
                SET_BUFFER(t, p+2, nal-2);
                p += nal;
                size += (nal - 2 + 4);  // 4 => length of start code to be inserted
            }

            pps = *(p++);  // number of pps
            for (i = 0; i < pps; i++)
            {
                nal = (*p << 8) + *(p + 1) + 2;
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x00);
                SET_BYTE(t, 0x01);
                SET_BUFFER(t, p+2, nal-2);
                p += nal;
                size += (nal - 2 + 4);  // 4 => length of start code to be inserted
            }
        } else if(nMetaData > 3) {
            size = -1;  // return to meaning of invalid stream data;
            for (; p < a; p++) {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1)  {
                    // find startcode
                    size = avc->extradata_size;
                    if (!pbHeader || !pbMetaData)
                        return 0;
                    SET_BUFFER(pbHeader, pbMetaData, size);
                    break;
                }
            }
        }
    } else if (p_enType == PT_H265) {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01) {
            static const int8_t nalu_header[4] = { 0, 0, 0, 1 };
            int numOfArrays = 0;
            uint16_t numNalus = 0;
            uint16_t nalUnitLength = 0;
            uint32_t offset = 0;

            p += 21;
            *sizelength = (*p++ & 0x3) + 1;
            numOfArrays = *p++;

            while(numOfArrays--) {
                p++;   // NAL type
                numNalus = (*p << 8) + *(p + 1);
                p+=2;
                for(i = 0;i < numNalus;i++)
                {
                    nalUnitLength = (*p << 8) + *(p + 1);
                    p+=2;
                    // if(i == 0)
                    {
                        memcpy(pbHeader + offset, nalu_header, 4);
                        offset += 4;
                        memcpy(pbHeader + offset, p, nalUnitLength);
                        offset += nalUnitLength;
                    }
                    p += nalUnitLength;
                }
            }

            size = offset;
        } else if(nMetaData > 3) {
            size = -1;  // return to meaning of invalid stream data;

            for (; p < a; p++)
            {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1)  // find startcode
                {
                    size = avc->extradata_size;
                    if (!pbHeader || !pbMetaData)
                        return 0;
                    SET_BUFFER(pbHeader, pbMetaData, size);
                    break;
                }
            }
        }
    } else {
        SET_BUFFER(pbHeader, pbMetaData, nMetaData);
        size = nMetaData;
    }

    return size;
}

int AV_read_frame(AVFormatContext *avContext, AVPacket *avpacket,
        av_param_t *av_param, vp_param_t *vp_param) {
    uint8_t *seqHeader = NULL;
    int seqHeaderSize = 0, error = 0;

    if (!avpacket->size) {
        error = av_read_frame(avContext, avpacket);
    }
    if (error < 0) {
        if (error == AVERROR_EOF || avContext->pb->eof_reached == 1) {
            printf("There is no more input data, %d!\n", avpacket->size);
        } else {
            printf("Failed to av_read_frame error(0x%08x)\n", error);
        }
        return -1;
    } else {
        seqHeaderSize = 0;
        auto mmz_index = av_param->count % vp_param->mmz_cnt;
        if (av_param->firstPacket) {
            AVCodecParameters* codec;
            int retSize = 0;
            codec = avContext->streams[av_param->videoIndex]->codecpar;
            seqHeader = (uint8_t*)malloc(codec->extradata_size + 1024);
            if (seqHeader == NULL) {
                printf("Failed to mallock seqHeader\n");
                return -1;
            }
            memset((void*)seqHeader, 0x00, codec->extradata_size + 1024);

            seqHeaderSize = build_dec_seq_header(seqHeader,
                    PT_H264, avContext->streams[av_param->videoIndex], &retSize);
            if (seqHeaderSize < 0) {
                printf("Failed to build seqHeader\n");
                return -1;
            }
            av_param->firstPacket = 0;
        }
        if (avpacket->size <= vp_param->mmz_size) {
            if (seqHeaderSize) {
                memcpy((void*)vp_param->mmz_vaddr[mmz_index],
                        (void*)seqHeader, seqHeaderSize);
                av_param->bufSize = seqHeaderSize;
            } else {
                memcpy((void*)vp_param->mmz_vaddr[mmz_index],
                        (void*)avpacket->data, avpacket->size);
                av_param->bufSize = avpacket->size;
                av_packet_unref(avpacket);
                avpacket->size = 0;
            }
        } else {
            printf("The external stream buffer is too small!"
                    "avpacket.size:%d, mmz_size:%d\n",
                    avpacket->size, vp_param->mmz_size);
            return -1;
        }
        if (seqHeader) {
            free(seqHeader);
            seqHeader = NULL;
        }
        ++av_param->count;
        return mmz_index;
    }
}