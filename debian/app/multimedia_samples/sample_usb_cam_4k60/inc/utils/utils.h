/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_
#include <iostream>
#include <mutex>
#include <deque>
#include <condition_variable>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#ifdef __cplusplus
}
#endif
#include "hb_vio_interface.h"
#include <functional>

template <typename T>
class CSingleton {
  struct object_creator {
   public:
    object_creator() { CSingleton<T>::Instance(); }
    inline void do_nothing() const {}
  };

  static object_creator create_object;

 public:
  typedef T object_type;
  static object_type &Instance() {
    static object_type obj;
    // 据说这个do_nothing是确保create_object构造函数被调用
    // 这跟模板的编译有关
    create_object.do_nothing();
    return obj;
  }
};

template <typename T>
typename CSingleton<T>::object_creator CSingleton<T>::create_object;

template <typename Dtype>
class RingQueue : public CSingleton<RingQueue<Dtype>> {
 public:
  using ReleaseDataFunc = std::function<void(Dtype& elem)>;
  RingQueue() {}
  int Init(typename std::deque<Dtype>::size_type size,
           ReleaseDataFunc release_func) {
    if (!size_) {
      printf("try to construct empty ring queue");
      size_ = size;
    }
    release_func_ = release_func;
    return 0;
  }

  void Push(const Dtype& elem) {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      if (size_ <= 0) {
        return;
      }
      if (que_.size() >= size_) {
        auto e = que_.front();
        if (release_func_) {
          release_func_(e);
        }
        que_.pop_front();
      }
      que_.push_back(elem);
    }
    cv_.notify_all();
  }

  bool Pop(Dtype &video_buffer) {
    if (exit) {
        return false;
    }
    std::unique_lock<std::mutex> lock(mtx_);
    while(que_.empty()) {
        cv_.wait(lock);
    }
    video_buffer = que_.front();
    que_.pop_front();

    // if (!que_.empty()) {
    //   video_buffer = que_.front();
    //   que_.pop_front();
    // } else {
    //   return false;
    // }
    return true;
  }

  bool Pop() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (!que_.empty()) {
      que_.pop_front();
    } else {
      return false;
    }
    return true;
  }

  bool Peak(Dtype &video_buffer) {
    std::unique_lock<std::mutex> lock(mtx_);
    if (!que_.empty()) {
      video_buffer = que_.front();
    } else {
      return false;
    }
    return true;
  }

  bool Empty() {
    std::lock_guard<std::mutex> l(mtx_);
    return que_.empty();
  }

  bool IsValid() {
    std::lock_guard<std::mutex> l(mtx_);
    // may push twice at one time
    return (size_ > 0 && que_.size() < size_);
  }

  void Clear() {
    std::lock_guard<std::mutex> l(mtx_);
    while (!que_.empty()) {
      auto elem = que_.front();
      if (release_func_) {
        release_func_(elem);
      }
      que_.pop_front();
    }
  }

  bool Exit() {
    std::lock_guard<std::mutex> l(mtx_);
    exit = 1;
	return true;
  }

 private:
  int exit = 0;
  typename std::deque<Dtype>::size_type size_ = 0;
  std::deque<Dtype> que_;
  std::mutex mtx_;
  std::condition_variable cv_;
  ReleaseDataFunc release_func_;
};

typedef struct vp_param {
    uint64_t mmz_paddr[8];
    char *mmz_vaddr[8];
    int mmz_cnt;
    int mmz_size;
} vp_param_t;

typedef struct av_param {
    int count;
    int videoIndex;
    int bufSize;
    int firstPacket;
} av_param_t;

int feedback_read_rawfile(char *fileName, hb_vio_buffer_t *feedback_buf);
int feedback_read_nv12file(char *fileName, hb_vio_buffer_t *feedback_buf);
void dump_pym_to_files(pym_buffer_t *pvio_image, int chn, int layer);
void dump_pym_us_to_files(pym_buffer_t *pvio_image, int chn, int layer);
void dump_roi_pym_to_files(pym_buffer_t *pvio_image, int chn,
    int main_layer, int sub_layer);
void dump_nv12_to_file(const char* file_name, address_info_t *addr_info);
void dump_stream_to_file(const char* ptr, size_t size);
void dump_jpg_to_file(const char* ptr, size_t size, int chn, int layer);
int AV_open_stream_file(char *FileName, AVFormatContext **avContext,
    AVPacket *avpacket);
int AV_read_frame(AVFormatContext *avContext, AVPacket *avpacket,
    av_param_t *av_param, vp_param_t *vp_param);
#endif // INCLUDE_UTILS_H_