#include "camera/camera.h"


VIN_LDC_ATTR_S LDC_ATTR_BASE = {
	.ldcEnable = 0,
	.ldcPath = {
		.rg_y_only = 0,
		.rg_uv_mode = 0,
		.rg_uv_interpo = 0,
		.rg_h_blank_cyc = 32,
	},
	.yStartAddr = 524288,
	.cStartAddr = 786432,
	.picSize = {
		.pic_w = 1919,
		.pic_h = 1079,
	},
	.lineBuf = 99,
	.xParam = {
		.rg_algo_param_b = 1,
		.rg_algo_param_a = 1,
	},
	.yParam = {
		.rg_algo_param_b = 1,
		.rg_algo_param_a = 1,
	},
	.offShift = {
		.rg_center_xoff = 0,
		.rg_center_yoff = 0,
	},
	.xWoi = {
		.rg_start = 0,
		.rg_length = 1919,
	},
	.yWoi = {
		.rg_start = 0,
		.rg_length = 1079,
	}
};

VIN_DIS_ATTR_S DIS_ATTR_BASE = {
    .picSize =
        {
            .pic_w = 1919,
            .pic_h = 1079,
        },
    .disPath =
        {
            .rg_dis_enable = 0,
            .rg_dis_path_sel = 1,
        },
    .disHratio = 65536,
    .disVratio = 65536,
    .xCrop =
        {
            .rg_dis_start = 0,
            .rg_dis_end = 1919,
        },
    .yCrop =
        {
            .rg_dis_start = 0,
            .rg_dis_end = 1079,
        },
    .disBufNum = 8,
};