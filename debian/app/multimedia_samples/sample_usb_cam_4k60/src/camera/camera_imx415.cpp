#include "camera/camera.h"

MIPI_SENSOR_INFO_S SENSOR_IMX415_30FPS_10BIT_LINEAR_INFO = {
    .deseEnable = 0,
    .inputMode = INPUT_MODE_MIPI,
    .deserialInfo = {},
    .sensorInfo = {
        .port = 0,
        .dev_port = 0,
        .bus_type = 0,
        .bus_num = 0,
        .fps = 60,
        .resolution = 2160,
        .sensor_addr = 0x1a,
        .serial_addr = 0,
        .entry_index = 0,
        .sensor_mode = static_cast<MIPI_SENSOR_MODE_E>(1),
        .reg_width = 16,
        .sensor_name = const_cast<char*>("imx415"),
        .extra_mode = 0,
        .deserial_index = -1,
        .deserial_port = 0,
    }
};

MIPI_ATTR_S MIPI_SENSOR_IMX415_30FPS_10BIT_LINEAR_ATTR = {
    .mipi_host_cfg = {
		4,           /* lane */
		0x2b,        /* datatype */
		24,          /* mclk */
		8000,        /* mipiclk */
		60,          /* fps */
		3840,        /* width */
		2160,        /*height */
		4400,        /* linlength */
		2700,        /* framelength */
		15,          /* settle */
		4,           /*chnnal_num*/
		{0, 1, 2, 3} /*vc */
    },
    .dev_enable = 0 /*  mipi dev enable */
};

MIPI_ATTR_S MIPI_SENSOR_IMX415_30FPS_10BIT_LINEAR_SENSOR_CLK_ATTR = {
    .mipi_host_cfg = {
		4,           /* lane */
		0x2b,        /* datatype */
		2400,        /* mclk */
		8000,        /* mipiclk */
		60,          /* fps */
		3840,        /* width  */
		2160,        /*height */
		4400,        /* linlength */
		2700,        /* framelength */
		15,          /* settle */
		4,           /*chnnal_num*/
		{0, 1, 2, 3} /*vc */
    },
    .dev_enable = 0 /*  mipi dev enable */
};

VIN_DEV_ATTR_S DEV_ATTR_IMX415_LINEAR_BASE = {
    .stSize = {
		0,    /*format*/
		3840, /*width*/
		2160, /*height*/
		1     /*pix_length*/
    },
    {
    .mipiAttr =
        {
            .enable = 1,
            .ipi_channels = 1,
            .ipi_mode = 0,
            .enable_mux_out = 1,
            .enable_frame_id = 1,
            .enable_bypass = 0,
            .enable_line_shift = 0,
            .enable_id_decoder = 0,
            .set_init_frame_id = 1,
            .set_line_shift_count = 0,
            .set_bypass_channels = 1,
        },
    },
    .DdrIspAttr = {.stride = 0,
                   .buf_num = 4,
                   .raw_feedback_en = 0,
                   .data =
                       {
                           .format = 0,
                           .width = 3840,
                           .height = 2160,
                           .pix_length = 1,
                       }},
    .outDdrAttr =
        {
            .stride = 4800,
            .buffer_num = 10,
        },
    .outIspAttr = {
        .dol_exp_num = 1,
        .enable_dgain = 0,
        .set_dgain_short = 0,
        .set_dgain_medium = 0,
        .set_dgain_long = 0,
    }};

VIN_PIPE_ATTR_S PIPE_ATTR_IMX415_LINEAR_BASE = {
    .ddrOutBufNum = 5,
    .frameDepth = 0,
    .snsMode = SENSOR_NORMAL_MODE,
    .stSize = {
        .format = 0,
        .width = 3840,
        .height = 2160,
    },
    .cfaPattern = static_cast<VIN_PIPE_CFA_PATTERN_E>(2),
    .temperMode = 2,
    .ispBypassEn = 0,
    .ispAlgoState = 1,
    .bitwidth = 10,
    .startX = 0,
    .startY = 0,
    .calib = {
        .mode = 1,
        .lname = const_cast<char*>("/lib/sensorlib/libimx415_linear.so"),
    }
};

VIN_DIS_ATTR_S DIS_ATTR_IMX415_BASE = {
  .picSize = {
    .pic_w = 3839,
	.pic_h = 2159,
  },
  .disPath = {
    .rg_dis_enable = 0,
    .rg_dis_path_sel = 1,
  },
  .disHratio = 65536,
  .disVratio = 65536,
  .xCrop = {
    .rg_dis_start = 0,
    .rg_dis_end = 3839,
  },
  .yCrop = {
	.rg_dis_start = 0,
	.rg_dis_end = 2159,
  }
};

VIN_LDC_ATTR_S LDC_ATTR_IMX415_BASE = {
    .ldcEnable = 0,
    .ldcPath = {
        .rg_y_only = 0,
        .rg_uv_mode = 0,
        .rg_uv_interpo = 0,
        .reserved1 = 0,
        .rg_h_blank_cyc = 32,
        .reserved0 = 0,
    },
    .yStartAddr = 524288,
    .cStartAddr = 786432,
    .picSize = {
        .pic_w = 3839,
        .pic_h = 2159,
    },
    .lineBuf = 99,
    .xParam = {
        .rg_algo_param_b = 3,
        .rg_algo_param_a = 2,
    },
    .yParam = {
        .rg_algo_param_b = 5,
        .rg_algo_param_a = 4,
    },
    .offShift = {
        .rg_center_xoff = 0,
        .rg_center_yoff = 0,
    },
    .xWoi = {
        .rg_start = 0,
        .reserved1 = 0,
        .rg_length = 3839,
        .reserved0 = 0
    },
    .yWoi = {
        .rg_start = 0,
        .reserved1 = 0,
        .rg_length = 2159,
        .reserved0 = 0
    }
};
