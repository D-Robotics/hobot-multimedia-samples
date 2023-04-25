/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#ifndef INCLUDE_VIO_VIO_CFG_H_
#define INCLUDE_VIO_VIO_CFG_H_

#include <string>
#include <mutex>
#include "json/json.h"
#include "vio/vio_cfg_type.h"

class VioConfig {
 public:
     VioConfig() = default;
     explicit VioConfig(const std::string &path) : path_(path) {}
     std::string GetStringValue(const std::string &key) const;
     int GetIntValue(const std::string &key) const;
     Json::Value GetJson() const;
     bool LoadConfig();
     bool ParserConfig();
     bool PrintConfig();
     vio_cfg_t GetConfig() {return vio_cfg_;}
 private:
     std::string path_;
     Json::Value json_;
     mutable std::mutex mutex_;
     vio_cfg_t vio_cfg_ = { 0 };
};

#endif  // INCLUDE_VIO_VIO_CFG_H_
