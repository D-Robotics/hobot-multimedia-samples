/***************************************************************************
 * COPYRIGHT NOTICE
 * Copyright 2020 Horizon Robotics, Inc.
 * All rights reserved.
 ***************************************************************************/
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include "vio/vio_log.h"
#include "vio/vio_cfg.h"
int running = 0;

bool VioConfig::LoadConfig() {
    std::ifstream ifs(path_);
    if (!ifs.is_open()) {
        std::cout << "Open config file " << path_ << " failed" << std::endl;
        return false;
    }
    std::cout << "Open config file " << path_ << " success" << std::endl;
    std::stringstream ss;
    ss << ifs.rdbuf();
    ifs.close();
    std::string content = ss.str();
    Json::Value value;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING error;
    std::shared_ptr<Json::CharReader> reader(builder.newCharReader());
    try {
        auto ret = reader->parse(content.c_str(),
                content.c_str() + content.size(),
                &json_, &error);
        if (ret) {
            std::cout << "Open config file1 " << path_ << std::endl;
            if (json_)
                return true;
        } else {
            std::cout << "Open config file2 " << path_ << std::endl;
            return false;
        }
    } catch (std::exception &e) {
        std::cout << "Open config file3 " << path_ << std::endl;
        return false;
    }
	return false;
}

int VioConfig::GetIntValue(const std::string &key) const {
  std::lock_guard<std::mutex> lk(mutex_);
  if (json_[key].empty()) {
    std::cout << "Can not find key: " << key << std::endl;
    return -1;
  }

  return json_[key].asInt();
}

std::string VioConfig::GetStringValue(const std::string &key) const {
  std::lock_guard<std::mutex> lk(mutex_);
  if (json_[key].empty()) {
    std::cout << "Can not find key: " << key << std::endl;
    return "";
  }

  return json_[key].asString();
}

Json::Value VioConfig::GetJson() const { return this->json_; }

bool VioConfig::ParserConfig() {
    int grp_start_index, grp_max_num;
    int i, j, k, m, n;

    vio_cfg_.cam_num = GetIntValue("cam_num");
    vio_cfg_.vps_grp_num = json_["vps_grp_num"].asInt();
    vio_cfg_.need_clk[0] = json_["need_clk"].asInt();
    vio_cfg_.need_vo = GetIntValue("need_vo");
    vio_cfg_.vps_dump = GetIntValue("vps_dump");
    vio_cfg_.need_rtsp = GetIntValue("need_rtsp");
    vio_cfg_.testpattern_fps = GetIntValue("testpattern_fps");
		vio_cfg_.Again_ispDgain = GetIntValue("Again_ispDgain");
		vio_cfg_.sensorAgain = GetIntValue("sensorAgain");
		vio_cfg_.ispDgain = GetIntValue("ispDgain");
		vio_cfg_.sleepTime = GetIntValue("sleepTime");
		vio_cfg_.AEM1 = GetIntValue("AEM1");
		vio_cfg_.tailWeight = GetIntValue("tailWeight");
		vio_cfg_.manualTimeCount = GetIntValue("manualTimeCount");
		vio_cfg_.autoTimeCount = GetIntValue("autoTimeCount");
		vio_cfg_.manualAeM1 = GetIntValue("manualAeM1");
		vio_cfg_.mjpgQp = GetIntValue("mjpgQp");
    /* 1. parse cam cfg */
    for (i = 0; i < vio_cfg_.cam_num; i++) {
        std::string cam_name = "cam" + std::to_string(i);
        vio_cfg_.sensor_id[i] =
            json_[cam_name]["sensor"]["sensor_id"].asInt();
        vio_cfg_.sensor_port[i] =
            json_[cam_name]["sensor"]["sensor_port"].asInt();
        vio_cfg_.mipi_idx[i] =
            json_[cam_name]["sensor"]["mipi_idx"].asInt();
        vio_cfg_.i2c_bus[i] =
            json_[cam_name]["sensor"]["i2c_bus"].asInt();
        vio_cfg_.serdes_index[i] =
            json_[cam_name]["sensor"]["serdes_index"].asInt();
        vio_cfg_.serdes_port[i] =
            json_[cam_name]["sensor"]["serdes_port"].asInt();
        vio_cfg_.temper_mode[i] =
            json_[cam_name]["isp"]["temper_mode"].asInt();
        vio_cfg_.isp_out_buf_num[i] =
            json_[cam_name]["isp"]["isp_out_buf_num"].asInt();
        vio_cfg_.gpio_num[i] =
            json_[cam_name]["sensor"]["gpio_num"].asInt();
        vio_cfg_.gpio_pin[i] =
            json_[cam_name]["sensor"]["gpio_pin"].asInt();
        vio_cfg_.gpio_level[i] =
            json_[cam_name]["sensor"]["gpio_level"].asInt();
    }

    /* 2. parse vps cfg */
    for (i = 0; i < vio_cfg_.vps_grp_num; i++) {
        std::string grp_name = "grp" + std::to_string(i);
        vio_cfg_.fb_width[i] =
            json_["vps"][grp_name]["fb_width"].asInt();
        vio_cfg_.fb_height[i] =
            json_["vps"][grp_name]["fb_height"].asInt();
        vio_cfg_.fb_buf_num[i] =
            json_["vps"][grp_name]["fb_buf_num"].asInt();
        vio_cfg_.grp_rotate[i] =
            json_["vps"][grp_name]["grp_rotate"].asInt();
        vio_cfg_.grp_frame_depth[i] =
            json_["vps"][grp_name]["grp_frame_depth"].asInt();
        vio_cfg_.grp_width[i] =
            json_["vps"][grp_name]["grp_width"].asInt();
        vio_cfg_.grp_height[i] =
            json_["vps"][grp_name]["grp_heigth"].asInt();
        vio_cfg_.vin_vps_mode[i] =
            json_["vps"][grp_name]["vin_vps_mode"].asInt();
        vio_cfg_.grp_gdc_en[i] =
            json_["vps"][grp_name]["grp_gdc_en"].asInt();
        /* 3. channel config */
        vio_cfg_.chn_num[i] = json_["vps"][grp_name]\
                                    ["chn_num"].asInt();
        if (vio_cfg_.chn_num[i] > MAX_CHN_NUM) {
            pr_err("chn num exceed max\n");
        }
        for (j = 0; j < vio_cfg_.chn_num[i]; j++) {
            std::string chn_name = "chn" + std::to_string(j);
            auto item = json_["vps"][grp_name][chn_name];
            vio_cfg_.chn_index[i][j] = item["chn_index"].asInt();
            vio_cfg_.chn_scale_en[i][j] = item["scale_en"].asInt();
            vio_cfg_.chn_crop_en[i][j] = item["crop_en"].asInt();
            vio_cfg_.chn_pym_en[i][j] = item["pym_en"].asInt();
            vio_cfg_.chn_width[i][j] = item["width"].asInt();
            vio_cfg_.chn_height[i][j] = item["height"].asInt();
            vio_cfg_.chn_frame_depth[i][j] = item["frame_depth"].asInt();
            vio_cfg_.chn_gdc_en[i][j] = item["chn_gdc_en"].asInt();
            if (vio_cfg_.chn_crop_en[i][j]) {
                vio_cfg_.chn_crop_width[i][j] = item["crop_width"].asInt();
                vio_cfg_.chn_crop_height[i][j] =
                    item["crop_height"].asInt();
                vio_cfg_.chn_crop_x[i][j] = item["crop_x"].asInt();
                vio_cfg_.chn_crop_y[i][j] = item["crop_y"].asInt();
            }
            if (vio_cfg_.chn_pym_en[i][j]) {
                auto sub_item = item["pym"]["pym_ctrl_config"];
                vio_cfg_.chn_pym_cfg[i][j].frame_id =
                    sub_item["frame_id"].asUInt();
                vio_cfg_.chn_pym_cfg[i][j].ds_uv_bypass =
                    sub_item["ds_uv_bypass"].asUInt();
                vio_cfg_.chn_pym_cfg[i][j].ds_layer_en =
                    sub_item["ds_layer_en"].asUInt();
                vio_cfg_.chn_pym_cfg[i][j].us_layer_en =
                    sub_item["us_layer_en"].asUInt();
                vio_cfg_.chn_pym_cfg[i][j].us_uv_bypass =
                    sub_item["us_uv_bypass"].asUInt();
                vio_cfg_.chn_pym_cfg[i][j].frameDepth =
                    sub_item["frame_depth"].asUInt();
                vio_cfg_.chn_pym_cfg[i][j].timeout =
                    sub_item["timeout"].asInt();
                /* 4.2 pym downscale config */
                for (k = 0 ; k < MAX_PYM_DS_NUM; k++) {
                    if (k % 4 == 0) continue;
                    std::string factor_name = "factor_" + std::to_string(k);
                    std::string roi_x_name = "roi_x_" + std::to_string(k);
                    std::string roi_y_name = "roi_y_" + std::to_string(k);
                    std::string roi_w_name = "roi_w_" + std::to_string(k);
                    std::string roi_h_name = "roi_h_" + std::to_string(k);
                    auto sub_item = item["pym"]["pym_ds_config"];
                    PYM_SCALE_INFO_S &tar_item = vio_cfg_.chn_pym_cfg[i][j].ds_info[k];
                    tar_item.factor = sub_item[factor_name].asUInt();
                    tar_item.roi_x = sub_item[roi_x_name].asUInt();
                    tar_item.roi_y = sub_item[roi_y_name].asUInt();
                    tar_item.roi_width = sub_item[roi_w_name].asUInt();
                    tar_item.roi_height = sub_item[roi_h_name].asUInt();
                }
                /* 4.3 pym upscale config */
                for (m = 0 ; m < MAX_PYM_US_NUM; m++) {
                        std::string factor_name = "factor_" + std::to_string(m);
                        std::string roi_x_name = "roi_x_" + std::to_string(m);
                        std::string roi_y_name = "roi_y_" + std::to_string(m);
                        std::string roi_w_name = "roi_w_" + std::to_string(m);
                        std::string roi_h_name = "roi_h_" + std::to_string(m);
                        auto sub_item = item["pym"]["pym_us_config"];
                        PYM_SCALE_INFO_S &tar_item = vio_cfg_.chn_pym_cfg[i][j].us_info[m];
                        tar_item.factor = sub_item[factor_name].asUInt();
                        tar_item.roi_x = sub_item[roi_x_name].asUInt();
                        tar_item.roi_y = sub_item[roi_y_name].asUInt();
                        tar_item.roi_width = sub_item[roi_w_name].asUInt();
                        tar_item.roi_height = sub_item[roi_h_name].asUInt();

                }
            }
        }
    }
    // PrintConfig();
    return true;
}

bool VioConfig::PrintConfig() {
    int i, j, k, m, n;
    int grp_start_index, grp_max_num = 0;
    /* 1. print sensor config */
    std::cout << "*********** iot vio config start ***********" << std::endl;
    std::cout << "cam_num: " << vio_cfg_.cam_num << std::endl;
    std::cout << "vps_dump: " << vio_cfg_.vps_dump << std::endl;
    std::cout << "need_vo: " << vio_cfg_.need_vo << std::endl;
    std::cout << "need_rtsp: " << vio_cfg_.need_rtsp << std::endl;
    std::cout << "testpattern_fps: " << vio_cfg_.testpattern_fps << std::endl;
    for (n = 0; n < vio_cfg_.cam_num; n++) {
        std::cout << "#########cam:" << n << " cam config start#########" << std::endl;
        std::cout << "cam_index: "<< n << " " << "sensor_id: "
            << vio_cfg_.sensor_id[n] << std::endl;
        std::cout << "cam_index: "<< n << " " << "sensor_port: "
            << vio_cfg_.sensor_port[n] << std::endl;
        if (n < MAX_MIPIID_NUM) {
            std::cout << "cam_index: "<< n << " " << "mipi_idx: "
                << vio_cfg_.mipi_idx[n] << std::endl;
        }
        std::cout << "cam_index: "<< n << " " << "i2c_bus: "
            << vio_cfg_.i2c_bus[n] << std::endl;
        std::cout << "cam_index: "<< n << " " << "serdes_index: "
            << vio_cfg_.serdes_index[n] << std::endl;
        std::cout << "cam_index: "<< n << " " << "serdes_port: "
            << vio_cfg_.serdes_port[n] << std::endl;
        std::cout << "cam_index: "<< n << " " << "temper_mode: "
            << vio_cfg_.temper_mode[n] << std::endl;
        std::cout << "cam_index: "<< n << " " << "isp_out_buf_num: "
            << vio_cfg_.isp_out_buf_num[n] << std::endl;
        std::cout << "cam_index: "<< n << " " << "grp_num: "
            << vio_cfg_.vps_grp_num << std::endl;
        std::cout << "cam_index: "<< n << " " << "gpio_num: "
            << vio_cfg_.gpio_num[n] << std::endl;
        std::cout << "cam_index: "<< n << " " << "gpio_pin: "
            << vio_cfg_.gpio_pin[n] << std::endl;
        /* 2. print group config */

    }
    for (i = 0; i < vio_cfg_.vps_grp_num; i++) {
        std::cout << "=========grp:" << i << " group config start==========" << std::endl;
        std::cout << "grp_index: "<< i << " " << "fb_width: "
            << vio_cfg_.fb_width[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "fb_height: "
            << vio_cfg_.fb_height[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "fb_buf_num: "
            << vio_cfg_.fb_buf_num[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "vin_fd: "
            << vio_cfg_.vin_fd[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "vin_vps_mode: "
            << vio_cfg_.vin_vps_mode[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "need_clk: "
            << vio_cfg_.need_clk[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "need_md: "
            << vio_cfg_.need_md[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "need_chnfd: "
            << vio_cfg_.need_chnfd[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "need_dis: "
            << vio_cfg_.need_dis[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "grp_gdc_en: "
            << vio_cfg_.grp_gdc_en[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "grp_width: "
            << vio_cfg_.grp_width[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "grp_heigth: "
            << vio_cfg_.grp_height[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "grp_rotate: "
            << vio_cfg_.grp_rotate[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "dol2_vc_num: "
            << vio_cfg_.dol2_vc_num[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "grp_frame_depth: "
            << vio_cfg_.grp_frame_depth[i] << std::endl;
        std::cout << "grp_index: "<< i << " " << "chn_num: "
            << vio_cfg_.chn_num[i] << std::endl;
        /* 3. print channel config */
        for (j = 0 ; j < vio_cfg_.chn_num[i]; j++) {
            auto chn_index = vio_cfg_.chn_index[i][j];
            std::cout << "chn_index: "<< chn_index
                << " " << "scale_en: " << vio_cfg_.chn_scale_en[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "width: "
                << vio_cfg_.chn_width[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "height: "
                << vio_cfg_.chn_height[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "frame_depth: "
                << vio_cfg_.chn_frame_depth[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "crop_en: "
                << vio_cfg_.chn_crop_en[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "chn_pym_en: "
                << vio_cfg_.chn_pym_en[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "crop_width: "
                << vio_cfg_.chn_crop_width[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "crop_height: "
                << vio_cfg_.chn_crop_height[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "crop_x: "
                << vio_cfg_.chn_crop_x[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "crop_y: "
                << vio_cfg_.chn_crop_y[i][j] << std::endl;
            std::cout << "chn_index: "<< chn_index << " " << "chn_gdc_en: "
                << vio_cfg_.chn_gdc_en[i][j] << std::endl;
            if (vio_cfg_.chn_pym_en[i][j] == 1) {
                std::cout << "--------chn:" << chn_index << " pym config start---------" << std::endl;
                std::cout << "chn_index: "<< chn_index << " " << "frame_id: "
                    << vio_cfg_.chn_pym_cfg[i][j].frame_id << std::endl;
                std::cout << "chn_index: "<< chn_index << " " << "ds_layer_en: "
                    << vio_cfg_.chn_pym_cfg[i][j].ds_layer_en << std::endl;
                std::cout << "chn_index: "<< chn_index << " " << "ds_uv_bypass: "
                    << vio_cfg_.chn_pym_cfg[i][j].ds_uv_bypass << std::endl;
                std::cout << "chn_index: "<< chn_index << " " << "us_layer_en: "
                    << static_cast<int>(vio_cfg_.chn_pym_cfg[i][j].\
                            us_layer_en) << std::endl;
                std::cout << "chn_index: "<< chn_index << " " << "us_uv_bypass: "
                    << static_cast<int>(vio_cfg_.chn_pym_cfg[i][j].\
                            us_uv_bypass) << std::endl;
                std::cout << "chn_index: "<< chn_index << " " << "frameDepth: "
                    << vio_cfg_.chn_pym_cfg[i][j].frameDepth << std::endl;
                std::cout << "chn_index: "<< chn_index << " " << "timeout: "
                    << vio_cfg_.chn_pym_cfg[i][j].timeout << std::endl;

                for (k = 0 ; k < MAX_PYM_DS_NUM; k++) {
                    if (k % 4 == 0) continue;
                    std::cout << "ds_pym_layer: "<< k << " " << "ds roi_x: "
                        << vio_cfg_.chn_pym_cfg[i][j].ds_info[k].roi_x << std::endl;
                    std::cout << "ds_pym_layer: "<< k << " " << "ds roi_y: "
                        << vio_cfg_.chn_pym_cfg[i][j].ds_info[k].roi_y << std::endl;
                    std::cout << "ds_pym_layer: "<< k << " " << "ds roi_width: "
                        << vio_cfg_.chn_pym_cfg[i][j].ds_info[k].roi_width << std::endl;
                    std::cout << "ds_pym_layer: "<< k << " " << "ds roi_height: "
                        << vio_cfg_.chn_pym_cfg[i][j].ds_info[k].roi_height << std::endl;
                    std::cout << "ds_pym_layer: "<< k << " " << "ds factor: "
                        << static_cast<int>(vio_cfg_.chn_pym_cfg[i][j].\
                                ds_info[k].factor) << std::endl;
                }
                /* 4.3 pym upscale config */
                for (m = 0 ; m < MAX_PYM_US_NUM; m++) {
                    std::cout << "us_pym_layer: "<< m << " " << "us roi_x: "
                        << vio_cfg_.chn_pym_cfg[i][j].us_info[m].roi_x << std::endl;
                    std::cout << "us_pym_layer: "<< m << " " << "us roi_y: "
                        << vio_cfg_.chn_pym_cfg[i][j].us_info[m].roi_y << std::endl;
                    std::cout << "us_pym_layer: "<< m << " " << "us roi_width: "
                        << vio_cfg_.chn_pym_cfg[i][j].us_info[m].roi_width << std::endl;
                    std::cout << "us_pym_layer: "<< m << " " << "us roi_height: "
                        << vio_cfg_.chn_pym_cfg[i][j].us_info[m].roi_height << std::endl;
                    std::cout << "us_pym_layer: "<< m << " " << "us factor: "
                        << static_cast<int>(vio_cfg_.chn_pym_cfg[i][j].\
                                us_info[m].factor) << std::endl;
                }
                std::cout << "---------chn:" << j << " pym config end----------" << std::endl;
            }
        }
            std::cout << "=========grp:" << i << " group config end==========" << std::endl;
    }
    std::cout << "*********** iot vio config end ***********" << std::endl;

    return true;
}
