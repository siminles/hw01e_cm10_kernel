/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/mfd/pm8xxx/pm8921.h>
#include "msm_sensor.h"

#include "msm.h"

#include <hsad/config_interface.h>
#define SENSOR_NAME "imx091_sunny"
#define PLATFORM_DRIVER_NAME "msm_camera_imx091_sunny"
#define imx091_sunny_obj imx091_sunny_##obj

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define SMPS_2P85_EN 33
#define SMPS_1P2_EN 25
static int imx091_sunny_otp_read_flag = 0;
//unsigned short  otp_id[8] = {0};
static unsigned short  imx091_sunny_otp_awb[3] = {0};
unsigned short  imx091_sunny_otp_af[2] = {0};


DEFINE_MUTEX(imx091_sunny_mut);
static struct msm_sensor_ctrl_t imx091_sunny_s_ctrl;

static struct msm_camera_i2c_reg_conf imx091_sunny_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf imx091_sunny_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf imx091_sunny_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf imx091_sunny_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf imx091_sunny_prev_settings[] = {
	//********30fps  1/2*1/2
	//PLL setting
	{0x0305,0x02},   //pre_pll_clk_div[7:0]
	{0x0307,0x2F},   //pll_multiplier[7:0]
	{0x30A4,0x02},
	{0x303C,0x4B},

	//mode setting
	{0x0340,0x06},      //frame_length_lines[15:8]
	{0x0341,0x5A},      //frame_length_lines[7:0]
	{0x0342,0x12},     //line_length_pck[15:8]
	{0x0343,0x0C},     //line_length_pck[7:0]
	{0x0344,0x00},     //x_addr_start[15:8]
	{0x0345,0x08},     //x_addr_start[7:0]
	{0x0346,0x00},     //y_addr_start[15:8]
	{0x0347,0x30},     //y_addr_start[7:0]
	{0x0348,0x10},     //x_addr_end[15:8]
	{0x0349,0x77},     //x_addr_end[7:0]
	{0x034A,0x0C},     //y_addr_end[15:8]
	{0x034B,0x5F},     //y_addr_end[7:0]
	{0x034C,0x08},     //x_output_size[15:8]
	{0x034D,0x38},     //x_output_size[7:0]
	{0x034E,0x06},     //y_output_size[15:8]
	{0x034F,0x18},     //y_output_size[7:0]
	{0x0381,0x01},     //x_even_inc[3:0]
	{0x0383,0x03},     //x_odd_inc[3:0]
	{0x0385,0x01},     //y_even_inc[7:0]
	{0x0387,0x03},     //y_odd_inc[7:0]
	{0x3040,0x08},
	{0x3041,0x97},
	{0x3048,0x01},
	{0x3064,0x12},
	{0x309B,0x28},
	{0x309E,0x00},
	{0x30D5,0x09},
	{0x30D6,0x01},
	{0x30D7,0x01},
	{0x30D8,0x64},
	{0x30D9,0x89},
	{0x30DE,0x02},
	{0x3102,0x10},
	{0x3103,0x44},
	{0x3104,0x40},
	{0x3105,0x00},
	{0x3106,0x0D},
	{0x3107,0x01},
	{0x310A,0x0A},
	{0x315C,0x99},
	{0x315D,0x98},
	{0x316E,0x9A},
	{0x316F,0x99},
	{0x3318,0x73},

	//test register
//	{0x3282,0x01},
//	{0x3032,0x3C},
//	{0x0600,0x00},
//	{0x0601,0x02},
};

static struct msm_camera_i2c_reg_conf imx091_sunny_snap_settings[] = {
	/*full size  15fps*/
	{0x0307,0x2F},        //47
	{0x30A4,0x02},
	{0x303C,0x4B},
	
	{0x0112,0x0A},
	{0x0113,0x0A},
	{0x0340,0x0C},
	{0x0341,0x8C},
	{0x0344,0x00},
	{0x0345,0x08},
	{0x0346,0x00},
	{0x0347,0x30},
	{0x0348,0x10},
	{0x0349,0x77},
	{0x034A,0x0C},
	{0x034B,0x5F},
	{0x034C,0x10},
	{0x034D,0x70},
	{0x034E,0x0C},
	{0x034F,0x30},
	{0x0381,0x01},
	{0x0383,0x01},
	{0x0385,0x01},
	{0x0387,0x01},
//	{0x303E,0xD1},
	{0x3041,0x97},
//	{0x3048,0x20},
	{0x3064,0x12},
	{0x309B,0x20},
	{0x30D5,0x00},
	{0x30D6,0x85},
	{0x30D7,0x2A},
	{0x30D8,0x64},
	{0x30D9,0x89},
	{0x30DE,0x00},
	{0x3102,0x10},
	{0x3103,0x44},
	{0x3104,0x40},
	{0x3105,0x00},
	{0x3106,0x0D},
	{0x3107,0x01},
//	{0x3301,0x00},
	{0x3304,0x05},
	{0x3305,0x04},
	{0x3306,0x12},
	{0x3307,0x03},
	{0x3308,0x0D},
	{0x3309,0x05},
	{0x330A,0x09},
	{0x330B,0x04},
	{0x330C,0x08},
	{0x330D,0x05},
	{0x330E,0x03},
	{0x3318,0x60},
	{0x3322,0x02},
	{0x3342,0x0F},
};

static struct msm_camera_i2c_reg_conf imx091_sunny_1080p_settings[] = 

{

	/*full size  25fps*/
	{0x0305,0x02},   //pre_pll_clk_div[7:0]
	{0x0307,0x2F},   //pll_multiplier[7:0]
	{0x30A4,0x02},
	{0x303C,0x4B},
	
	{0x0112,0x0A},
	{0x0113,0x0A},


	//mode setting
	{0x0340,0x07},      //frame_length_lines[15:8]
	{0x0341,0xA2},      //frame_length_lines[7:0]
	{0x0342,0x12},     //line_length_pck[15:8]
	{0x0343,0x0C},     //line_length_pck[7:0]
	
	{0x0344,0x01},     //x_addr_start[15:8]
	{0x0345,0x40},     //x_addr_start[7:0]
	{0x0346,0x02},     //y_addr_start[15:8]
	{0x0347,0x8e},     //y_addr_start[7:0]
	{0x0348,0x0f},     //x_addr_end[15:8]
	{0x0349,0x3f},     //x_addr_end[7:0]
	{0x034A,0x0a},     //y_addr_end[15:8]
	{0x034B,0x01},     //y_addr_end[7:0]
	
	{0x034C,0x0e},     //x_output_size[15:8]
	{0x034D,0x00},     //x_output_size[7:0]
	{0x034E,0x07},     //y_output_size[15:8]
	{0x034F,0x74},     //y_output_size[7:0]
	
	{0x0381,0x01},     //x_even_inc[3:0]
	{0x0383,0x01},     //x_odd_inc[3:0]
	{0x0385,0x01},     //y_even_inc[7:0]
	{0x0387,0x01},     //y_odd_inc[7:0]
	
//	{0x303E,0xD1},
	{0x3041,0x97},
//	{0x3048,0x20},
	{0x3064,0x12},
	{0x309B,0x20},
	{0x30D5,0x00},
	{0x30D6,0x85},
	{0x30D7,0x2A},
	{0x30D8,0x64},
	{0x30D9,0x89},
	{0x30DE,0x00},
	{0x3102,0x10},
	{0x3103,0x44},
	{0x3104,0x40},
	{0x3105,0x00},
	{0x3106,0x0D},
	{0x3107,0x01},
//	{0x3301,0x00},
	{0x3304,0x05},
	{0x3305,0x04},
	{0x3306,0x12},
	{0x3307,0x03},
	{0x3308,0x0D},
	{0x3309,0x05},
	{0x330A,0x09},
	{0x330B,0x04},
	{0x330C,0x08},
	{0x330D,0x05},
	{0x330E,0x03},
	{0x3318,0x60},
	{0x3322,0x02},
	{0x3342,0x0F},
};
static struct msm_camera_i2c_reg_conf imx091_sunny_recommend_settings[] = {
	//{0x0100,0x00}, //   stand-by
	//global setting
	{0x3087,0x53},
	{0x309D,0x94},
	{0x30A1,0x08},
	{0x30C7,0x00},
	{0x3115,0x0E},
	{0x3118,0x42},
	{0x311D,0x34},
	{0x3121,0x0D},
	{0x3212,0xF2},
	{0x3213,0x0F},
	{0x3215,0x0F},
	{0x3217,0x0B},
	{0x3219,0x0B},
	{0x321B,0x0D},
	{0x321D,0x0D},
	//black level setting
	{0x3032,0x40},

	//test register
//	{0x3282,0x01},
//	{0x3032,0x3C},
//	{0x0600,0x00},
//	{0x0601,0x02},
	
};

static struct v4l2_subdev_info imx091_sunny_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array imx091_sunny_init_conf[] = {
	{&imx091_sunny_recommend_settings[0],
	ARRAY_SIZE(imx091_sunny_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array imx091_sunny_confs[] = {
	{&imx091_sunny_snap_settings[0],
	ARRAY_SIZE(imx091_sunny_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx091_sunny_prev_settings[0],
	ARRAY_SIZE(imx091_sunny_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx091_sunny_1080p_settings[0],
	ARRAY_SIZE(imx091_sunny_1080p_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t imx091_sunny_dimensions[] = {
	/* snapshot */
    {
		.x_output = 0X1070,   //4208    
		.y_output = 0XC30,    //3120
		.line_length_pclk = 0X120C,     //4620
		.frame_length_lines = 0XC8C,  //3212
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	/* preview */
	{
	//30 fps 1/2*1/2
		.x_output = 0X838,
		.y_output = 0X618,
		.line_length_pclk = 0X120C,
		.frame_length_lines = 0X65A,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},

      {
		.x_output = 0Xe00,   //4208    
		.y_output = 0x0774,    //1908
		.line_length_pclk = 0X120C,     //4620
		.frame_length_lines = 0x07A2,  //2210
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
}; 

static struct msm_camera_csid_vc_cfg imx091_sunny_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params imx091_sunny_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 4,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = imx091_sunny_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 4,
		.settle_cnt = 0x1B,
	},
};

static struct msm_camera_csi2_params *imx091_sunny_csi_params_array[] = {
	&imx091_sunny_csi_params,
	&imx091_sunny_csi_params,
	&imx091_sunny_csi_params,
};

static struct msm_sensor_output_reg_addr_t imx091_sunny_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t imx091_sunny_id_info = {
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x0091,
};

static struct msm_sensor_exp_gain_info_t imx091_sunny_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 5,
};

static int imx091_sunny_otp_data_read(void)
{
	int32_t rc = 0;
	int32_t count= 0;
	uint16_t otpdata = 0;
	uint16_t addr = 0x3572;    //Bank E  MAX CODE
//	uint16_t addr_awb= 0x3568;    //Bank E AWB
	uint16_t bank_set = 0x0E;

	for(count = 0; count <5; count++)
	{
		rc = msm_camera_i2c_write((&imx091_sunny_s_ctrl)->sensor_i2c_client, 0x34C9, bank_set, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) 
		{
			CDBG("%s: Error Setting 0x34C9 values!\n", __func__);
			return rc;
		}
		rc = msm_camera_i2c_read((&imx091_sunny_s_ctrl)->sensor_i2c_client, addr, &otpdata, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) 
		{
			CDBG("%s: Error Reading OTP data @ 0x%x\n", __func__, addr);
			return rc;
		}
		if(0x0 == otpdata)
		{
			addr = addr - 0x18;  //three banks
			bank_set = bank_set - 0x3;
		}
		else
		{
			break;
		}
	}
	
	if(5 == count)
	{
		CDBG("%s: OTP data doesn't exit!\n", __func__);
		imx091_sunny_otp_read_flag = -1;
		rc = -ENXIO;
		return rc;
	}

	
	/******************read AF information *******************/
	/******************start code*******************/
//	addr = addr_awb + 0xA;
	addr = addr -0x2;
	rc = msm_camera_i2c_read((&imx091_sunny_s_ctrl)->sensor_i2c_client, addr, &otpdata, MSM_CAMERA_I2C_WORD_DATA);
	if(rc < 0) 
	{
		CDBG("%s: Error Reading OTP data @ 0x%x\n", __func__, addr);
		return rc;
	}
	imx091_sunny_otp_af[0] = otpdata&0xFFFF;


	/*max code*/
	addr += 0x2;
	rc = msm_camera_i2c_read((&imx091_sunny_s_ctrl)->sensor_i2c_client, addr, &otpdata, MSM_CAMERA_I2C_WORD_DATA);
	if(rc < 0) 
	{
		CDBG("%s: Error Reading OTP data @ 0x%x\n", __func__, addr);
		return rc;
	}
	imx091_sunny_otp_af[1] = otpdata&0xFFFF;

	bank_set = bank_set -0x1;
	msm_camera_i2c_write((&imx091_sunny_s_ctrl)->sensor_i2c_client, 0x34C9, bank_set, MSM_CAMERA_I2C_BYTE_DATA);

	
	/*R/G data*/
	addr = addr - 0xA;
	rc = msm_camera_i2c_read((&imx091_sunny_s_ctrl)->sensor_i2c_client, addr, &otpdata, MSM_CAMERA_I2C_WORD_DATA);
	if(rc < 0) 
	{
		CDBG("%s: Error Reading OTP data @ 0x%x\n", __func__, addr);
		return rc;
	}
	imx091_sunny_otp_awb[0] = otpdata&0xFFFF;


	/*B/G data*/
	addr += 0x2;
	rc = msm_camera_i2c_read((&imx091_sunny_s_ctrl)->sensor_i2c_client, addr, &otpdata, MSM_CAMERA_I2C_WORD_DATA);
	if(rc < 0) 
	{
		CDBG("%s: Error Reading OTP data @ 0x%x\n", __func__, addr);
		return rc;
	}
	imx091_sunny_otp_awb[1] = otpdata&0xFFFF;


	/*GB/GR data*/
	addr += 0x2;
	rc = msm_camera_i2c_read((&imx091_sunny_s_ctrl)->sensor_i2c_client, addr, &otpdata, MSM_CAMERA_I2C_WORD_DATA);
	if(rc < 0) 
	{
		CDBG("%s: Error Reading OTP data @ 0x%x\n", __func__, addr);
		return rc;
	}
	imx091_sunny_otp_awb[2] = otpdata&0xFFFF;

//	printk("zhanglijun==== r_over_g=0x%x, %d.\n \t\t\t\tb_over_g=0x%x, %d, \n \t\t\t\tgr_over_gb=0x%x,  %d\n \t\t\timx091_sunny_otp_af[0] = %d, imx091_sunny_otp_af[1] = %d.\n",
//       imx091_sunny_otp_awb[0], imx091_sunny_otp_awb[0], 
//        imx091_sunny_otp_awb[1], imx091_sunny_otp_awb[1], 
//        imx091_sunny_otp_awb[2], imx091_sunny_otp_awb[2],
//       imx091_sunny_otp_af[0], imx091_sunny_otp_af[1]);
	return rc;
}

static int imx091_sunny_otp_data_update(struct sensor_cfg_data *cfg)
{
    cfg->cfg.calib_info.r_over_g = imx091_sunny_otp_awb[0];//(((imx091_sunny_otp_awb[0]&0xFF)<<8)|(imx091_sunny_otp_awb[1]&0xFF));
    cfg->cfg.calib_info.b_over_g = imx091_sunny_otp_awb[1];//(((imx091_sunny_otp_awb[2]&0xFF)<<8)|(imx091_sunny_otp_awb[3]&0xFF));
    cfg->cfg.calib_info.gr_over_gb = imx091_sunny_otp_awb[2];//(((imx091_sunny_otp_awb[4]&0xFF)<<8)|(imx091_sunny_otp_awb[5]&0xFF));
    cfg->cfg.calib_info.af_start_code= imx091_sunny_otp_af[0];//(((imx091_sunny_otp_awb[4]&0xFF)<<8)|(imx091_sunny_otp_awb[5]&0xFF));
    cfg->cfg.calib_info.af_max_code= imx091_sunny_otp_af[1];//(((imx091_sunny_otp_awb[4]&0xFF)<<8)|(imx091_sunny_otp_awb[5]&0xFF));

//    printk("zhanglijun==== r_over_g=0x%x  b_over_g=0x%x  gr_over_gb=0x%x\n ,imx091_sunny_otp_af[0] is 0x%x, imx091_sunny_otp_af[1] is 0x%x.\n   "
//      ,cfg->cfg.calib_info.r_over_g,cfg->cfg.calib_info.b_over_g,cfg->cfg.calib_info.gr_over_gb, imx091_sunny_otp_af[0], imx091_sunny_otp_af[1]);
    
    return imx091_sunny_otp_read_flag;
 //	return -1;
}


void imx091_sunny_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	mdelay(5);
	msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->start_stream_conf,
		s_ctrl->msm_sensor_reg->start_stream_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
}
int32_t imx091_sunny_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc=0;
    int shdn_gpio;
    int vcm_pwd_gpio;

    msm_sensor_expand_power_up(s_ctrl);
	
    shdn_gpio = get_gpio_num_by_name("CAM_SHDN");	
    if(shdn_gpio < 0)
    {
        printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
        return shdn_gpio;
    }
    rc = gpio_request(shdn_gpio,"imx091_sunny");
    if (rc) 
    {
        gpio_free(shdn_gpio);
        rc = gpio_request(shdn_gpio,"imx091_sunny");
        if(rc)
        {
            printk("%s gpio_request(%d) again fail \n",__func__,shdn_gpio);
            return rc;
        }
        printk("%s gpio_request(%d) again success\n",__func__,shdn_gpio);
    }	
    vcm_pwd_gpio = get_gpio_num_by_name("CAM_AF_SHDN");
    if(vcm_pwd_gpio < 0)
    {
        printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
        return vcm_pwd_gpio;
    }
    rc = gpio_request(vcm_pwd_gpio,"imx091_sunny");
    if (rc) 
    {
        gpio_free(vcm_pwd_gpio);
        rc = gpio_request(vcm_pwd_gpio,"imx091_sunny");
        if(rc)
        {
            printk("%s gpio_request(%d) again fail \n",__func__,vcm_pwd_gpio);
            return rc;
        }
        printk("%s gpio_request(%d) again success\n",__func__,vcm_pwd_gpio);
    }	
        
    gpio_direction_output(shdn_gpio, 1);
	
    gpio_direction_output(vcm_pwd_gpio, 1);
    mdelay(10);   
    gpio_direction_output(vcm_pwd_gpio, 0);
    mdelay(10);    
    gpio_direction_output(vcm_pwd_gpio, 1);
    mdelay(10);
    
    return 0;
}

int32_t imx091_sunny_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
    int shdn_gpio;
    int vcm_pwd_gpio;

    shdn_gpio = get_gpio_num_by_name("CAM_SHDN");	
    if(shdn_gpio < 0)
    {
        printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
        return shdn_gpio;
    }
    vcm_pwd_gpio = get_gpio_num_by_name("CAM_AF_SHDN");
    if(vcm_pwd_gpio < 0)
    {
        printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
        return vcm_pwd_gpio;
    }
    gpio_direction_output(shdn_gpio, 0);
    gpio_direction_output(vcm_pwd_gpio, 0);
    gpio_free(shdn_gpio);
    gpio_free(vcm_pwd_gpio);
    
    msm_sensor_expand_power_down(s_ctrl);
    return 0;
}

static int32_t imx091_sunny_msm_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	int ret = 0;
	struct msm_sensor_ctrl_t *s_ctrl;
	CDBG("%s_i2c_probe called\n", client->name);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CDBG("i2c_check_functionality failed\n");
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	if (s_ctrl->sensor_i2c_client != NULL) {
		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;
	} else {
		rc = -EFAULT;
		return rc;
	}

	s_ctrl->sensordata = client->dev.platform_data;
	if (s_ctrl->sensordata == NULL) {
		pr_err("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}

	rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	if (rc < 0)
		goto probe_fail;

	rc = msm_sensor_match_id(s_ctrl);
	if (rc < 0)
		goto probe_fail;
	
	ret= imx091_sunny_otp_data_read();
       // if(ret < 0)
	
	if (s_ctrl->sensor_eeprom_client != NULL) {
		struct msm_camera_eeprom_client *eeprom_client =
			s_ctrl->sensor_eeprom_client;
		if (eeprom_client->func_tbl.eeprom_init != NULL &&
			eeprom_client->func_tbl.eeprom_release != NULL) {
			rc = eeprom_client->func_tbl.eeprom_init(
				eeprom_client,
				s_ctrl->sensor_i2c_client->client->adapter);
			if (rc < 0)
				goto probe_fail;

			rc = msm_camera_eeprom_read_tbl(eeprom_client,
			eeprom_client->read_tbl, eeprom_client->read_tbl_size);
			eeprom_client->func_tbl.eeprom_release(eeprom_client);
			if (rc < 0)
				goto probe_fail;
		}
	}

	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);
	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		s_ctrl->sensor_v4l2_subdev_ops);

	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);
	goto power_down;
probe_fail:
	CDBG("%s_i2c_probe failed\n", client->name);
power_down:
	if (rc > 0)
		rc = 0;
	s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	return rc;
}
#if 0 /* not use temply */
static struct sensor_calib_data imx091_sunny_calib_data;
#endif

static const struct i2c_device_id imx091_sunny_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&imx091_sunny_s_ctrl},
	{ }
};

static struct i2c_driver imx091_sunny_i2c_driver = {
	.id_table = imx091_sunny_i2c_id,

	.probe  = imx091_sunny_msm_sensor_i2c_probe,


	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx091_sunny_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

#if 0 /* not use temply */
static struct msm_camera_i2c_client imx091_sunny_eeprom_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_camera_eeprom_read_t imx091_sunny_eeprom_read_tbl[] = {
	{0x10, &imx091_sunny_calib_data.r_over_g, 2, 1},
	{0x12, &imx091_sunny_calib_data.b_over_g, 2, 1},
	{0x14, &imx091_sunny_calib_data.gr_over_gb, 2, 1},
};

static struct msm_camera_eeprom_data_t imx091_sunny_eeprom_data_tbl[] = {
	{&imx091_sunny_calib_data, sizeof(struct sensor_calib_data)},
};

static struct msm_camera_eeprom_client imx091_sunny_eeprom_client = {
	.i2c_client = &imx091_sunny_eeprom_i2c_client,
	.i2c_addr = 0xA4,

	.func_tbl = {
		.eeprom_set_dev_addr = NULL,
		.eeprom_init = msm_camera_eeprom_init,
		.eeprom_release = msm_camera_eeprom_release,
		.eeprom_get_data = msm_camera_eeprom_get_data,
	},

	.read_tbl = imx091_sunny_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(imx091_sunny_eeprom_read_tbl),
	.data_tbl = imx091_sunny_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(imx091_sunny_eeprom_data_tbl),
};
#endif

static int __init imx091_sunny_sensor_init_module(void)
{
	return i2c_add_driver(&imx091_sunny_i2c_driver);
}

static struct v4l2_subdev_core_ops imx091_sunny_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops imx091_sunny_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx091_sunny_subdev_ops = {
	.core = &imx091_sunny_subdev_core_ops,
	.video  = &imx091_sunny_subdev_video_ops,
};

static struct msm_sensor_fn_t imx091_sunny_func_tbl = {
	.sensor_start_stream = imx091_sunny_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1, 
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = imx091_sunny_sensor_power_up,
	.sensor_power_down = imx091_sunny_sensor_power_down,
	.sensor_otp_reading = imx091_sunny_otp_data_update,
	.sensor_csi_setting = msm_sensor_setting1,
};

static struct msm_sensor_reg_t imx091_sunny_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = imx091_sunny_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(imx091_sunny_start_settings),
	.stop_stream_conf = imx091_sunny_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(imx091_sunny_stop_settings),
	.group_hold_on_conf = imx091_sunny_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(imx091_sunny_groupon_settings),
	.group_hold_off_conf = imx091_sunny_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(imx091_sunny_groupoff_settings),
	.init_settings = &imx091_sunny_init_conf[0],
	.init_size = ARRAY_SIZE(imx091_sunny_init_conf),
	.mode_settings = &imx091_sunny_confs[0],
	.output_settings = &imx091_sunny_dimensions[0],
	.num_conf = ARRAY_SIZE(imx091_sunny_confs),
};

static struct msm_sensor_ctrl_t imx091_sunny_s_ctrl = {
	.msm_sensor_reg = &imx091_sunny_regs,
	.sensor_i2c_client = &imx091_sunny_sensor_i2c_client,
	.sensor_i2c_addr = 0x35,
	#if 0 /* not use temply */
	.sensor_eeprom_client = &imx091_sunny_eeprom_client,
	#endif
	.sensor_output_reg_addr = &imx091_sunny_reg_addr,
	.sensor_id_info = &imx091_sunny_id_info,
	.sensor_exp_gain_info = &imx091_sunny_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &imx091_sunny_csi_params_array[0],
	.msm_sensor_mutex = &imx091_sunny_mut,
	.sensor_i2c_driver = &imx091_sunny_i2c_driver,
	.sensor_v4l2_subdev_info = imx091_sunny_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx091_sunny_subdev_info),
	.sensor_v4l2_subdev_ops = &imx091_sunny_subdev_ops,
	.func_tbl = &imx091_sunny_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(imx091_sunny_sensor_init_module);
MODULE_DESCRIPTION("aptia 12MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
