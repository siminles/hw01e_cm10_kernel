/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/gpiomux.h>

#include <linux/mfd/pm8xxx/pm8921.h>

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

#define CAM_VANA_MINUV                2800000
#define CAM_VANA_MAXUV                2850000

#define CAM_8960_L10_UA               515000  //L10
#define CAM_VANA_LOAD_UA              85600   //L11

#define SMPS_2P85_EN 33
#define CAM_1P2_EN_SBM 25

static struct regulator *cam_vana;
static struct regulator *cam_vio;
static struct regulator *cam_8921_l10;

int msm_cam_clk_enable(struct device *dev, struct msm_cam_clk_info *clk_info,
		struct clk **clk_ptr, int num_clk, int enable)
{
	int i;
	int rc = 0;
	if (enable) {
		for (i = 0; i < num_clk; i++) {
			clk_ptr[i] = clk_get(dev, clk_info[i].clk_name);
			if (IS_ERR(clk_ptr[i])) {
				pr_err("%s get failed\n", clk_info[i].clk_name);
				rc = PTR_ERR(clk_ptr[i]);
				goto cam_clk_get_err;
			}
			if (clk_info[i].clk_rate >= 0) {
				rc = clk_set_rate(clk_ptr[i],
							clk_info[i].clk_rate);
				if (rc < 0) {
					pr_err("%s set failed\n",
						   clk_info[i].clk_name);
					goto cam_clk_set_err;
				}
			}
			rc = clk_enable(clk_ptr[i]);
			if (rc < 0) {
				pr_err("%s enable failed\n",
					   clk_info[i].clk_name);
				goto cam_clk_set_err;
			}
		}
	} else {
		for (i = num_clk - 1; i >= 0; i--) {
			if (clk_ptr[i] != NULL)
				clk_disable(clk_ptr[i]);
				clk_put(clk_ptr[i]);
		}
	}
	return rc;


cam_clk_set_err:
	clk_put(clk_ptr[i]);
cam_clk_get_err:
	for (i--; i >= 0; i--) {
		if (clk_ptr[i] != NULL) {
			clk_disable(clk_ptr[i]);
			clk_put(clk_ptr[i]);
		}
	}
	return rc;
}

int msm_camera_config_vreg(struct device *dev, struct camera_vreg_t *cam_vreg,
		int num_vreg, struct regulator **reg_ptr, int config)
{
	int i = 0;
	int rc = 0;
	struct camera_vreg_t *curr_vreg;
	if (config) {
		for (i = 0; i < num_vreg; i++) {
			curr_vreg = &cam_vreg[i];
			reg_ptr[i] = regulator_get(dev,
				curr_vreg->reg_name);
			if (IS_ERR(reg_ptr[i])) {
				pr_err("%s: %s get failed\n",
				 __func__,
				 curr_vreg->reg_name);
				reg_ptr[i] = NULL;
				goto vreg_get_fail;
			}
			if (curr_vreg->type == REG_LDO) {
				rc = regulator_set_voltage(
				reg_ptr[i],
				curr_vreg->min_voltage,
				curr_vreg->max_voltage);
				if (rc < 0) {
					pr_err(
					"%s: %s set voltage failed\n",
					__func__,
					curr_vreg->reg_name);
					goto vreg_set_voltage_fail;
				}
				if (curr_vreg->op_mode >= 0) {
					rc = regulator_set_optimum_mode(
						reg_ptr[i],
						curr_vreg->op_mode);
					if (rc < 0) {
						pr_err(
						"%s: %s set optimum mode failed\n",
						__func__,
						curr_vreg->reg_name);
						goto vreg_set_opt_mode_fail;
					}
				}
			}
		}
	} else {
		for (i = num_vreg-1; i >= 0; i--) {
			curr_vreg = &cam_vreg[i];
			if (reg_ptr[i]) {
				if (curr_vreg->type == REG_LDO) {
					if (curr_vreg->op_mode >= 0) {
						regulator_set_optimum_mode(
							reg_ptr[i], 0);
					}
					regulator_set_voltage(
						reg_ptr[i],
						0, curr_vreg->max_voltage);
				}
				regulator_put(reg_ptr[i]);
				reg_ptr[i] = NULL;
			}
		}
	}
	return 0;

vreg_unconfig:
if (curr_vreg->type == REG_LDO)
	regulator_set_optimum_mode(reg_ptr[i], 0);

vreg_set_opt_mode_fail:
if (curr_vreg->type == REG_LDO)
	regulator_set_voltage(reg_ptr[i], 0,
		curr_vreg->max_voltage);

vreg_set_voltage_fail:
	regulator_put(reg_ptr[i]);
	reg_ptr[i] = NULL;

vreg_get_fail:
	for (i--; i >= 0; i--) {
		curr_vreg = &cam_vreg[i];
		goto vreg_unconfig;
	}
	return -ENODEV;
}

int msm_camera_enable_vreg(struct device *dev, struct camera_vreg_t *cam_vreg,
		int num_vreg, struct regulator **reg_ptr, int enable)
{
	int i = 0, rc = 0;
	if (enable) {
		for (i = 0; i < num_vreg; i++) {
			if (IS_ERR(reg_ptr[i])) {
				pr_err("%s: %s null regulator\n",
				__func__, cam_vreg[i].reg_name);
				goto disable_vreg;
			}
			rc = regulator_enable(reg_ptr[i]);
			if (rc < 0) {
				pr_err("%s: %s enable failed\n",
				__func__, cam_vreg[i].reg_name);
				goto disable_vreg;
			}
		}
	} else {
		for (i = num_vreg-1; i >= 0; i--)
			regulator_disable(reg_ptr[i]);
	}
	return rc;
disable_vreg:
	for (i--; i >= 0; i--) {
		regulator_disable(reg_ptr[i]);
		goto disable_vreg;
	}
	return rc;
}

int msm_camera_request_gpio_table(struct msm_camera_sensor_info *sinfo,
	int gpio_en)
{
	int rc = 0;
	struct msm_camera_gpio_conf *gpio_conf =
		sinfo->sensor_platform_info->gpio_conf;

#ifdef CONFIG_HUAWEI_KERNEL
    if (NULL == gpio_conf->cam_gpio_common_tbl) {
		pr_err("%s: NULL camera gpio table\n", __func__);
		return -EFAULT;
	}
#else
	if (gpio_conf->cam_gpio_req_tbl == NULL ||
		gpio_conf->cam_gpio_common_tbl == NULL) {
		pr_err("%s: NULL camera gpio table\n", __func__);
		return -EFAULT;
	}
#endif

	if (gpio_en) {
		if (gpio_conf->cam_gpiomux_conf_tbl != NULL) {
			msm_gpiomux_install(
				(struct msm_gpiomux_config *)gpio_conf->
				cam_gpiomux_conf_tbl,
				gpio_conf->cam_gpiomux_conf_tbl_size);
		}
		rc = gpio_request_array(gpio_conf->cam_gpio_common_tbl,
				gpio_conf->cam_gpio_common_tbl_size);
		if (rc < 0) {
			pr_err("%s common gpio request failed\n", __func__);
			return rc;
		}
		if (NULL != gpio_conf->cam_gpio_req_tbl) {
		rc = gpio_request_array(gpio_conf->cam_gpio_req_tbl,
				gpio_conf->cam_gpio_req_tbl_size);
		if (rc < 0) {
			pr_err("%s camera gpio request failed\n", __func__);
			gpio_free_array(gpio_conf->cam_gpio_common_tbl,
				gpio_conf->cam_gpio_common_tbl_size);
			return rc;
		}
		}		
	} else {
	    if (NULL != gpio_conf->cam_gpio_req_tbl) {
		gpio_free_array(gpio_conf->cam_gpio_req_tbl,
				gpio_conf->cam_gpio_req_tbl_size);
		}
		gpio_free_array(gpio_conf->cam_gpio_common_tbl,
			gpio_conf->cam_gpio_common_tbl_size);
	}
	return rc;
}

int msm_camera_config_gpio_table(struct msm_camera_sensor_info *sinfo,
	int gpio_en)
{
	struct msm_camera_gpio_conf *gpio_conf =
		sinfo->sensor_platform_info->gpio_conf;
	int rc = 0, i;

#ifdef CONFIG_HUAWEI_KERNEL
	if(NULL == gpio_conf->cam_gpio_set_tbl)
		return rc;
#endif

	if (gpio_en) {
		for (i = 0; i < gpio_conf->cam_gpio_set_tbl_size; i++) {
			gpio_set_value_cansleep(
				gpio_conf->cam_gpio_set_tbl[i].gpio,
				gpio_conf->cam_gpio_set_tbl[i].flags);
			usleep_range(gpio_conf->cam_gpio_set_tbl[i].delay,
				gpio_conf->cam_gpio_set_tbl[i].delay + 1000);
		}
	} else {
		for (i = gpio_conf->cam_gpio_set_tbl_size - 1; i >= 0; i--) {
			if (gpio_conf->cam_gpio_set_tbl[i].flags)
				gpio_set_value_cansleep(
					gpio_conf->cam_gpio_set_tbl[i].gpio,
					GPIOF_OUT_INIT_LOW);
		}
	}
	return rc;
}

int power_seq_enable_docomo(struct device *dev)
{
    int rc=0;
    //prepare
    struct pm_gpio param = {
        .direction      = PM_GPIO_DIR_OUT,
        .output_buffer  = PM_GPIO_OUT_BUF_CMOS,
        .output_value   = 1,
        .pull           = PM_GPIO_PULL_NO,
        .vin_sel        = PM_GPIO_VIN_S4,
        .out_strength   = PM_GPIO_STRENGTH_NO,
        .function       = PM_GPIO_FUNC_NORMAL,
    };
    //SMPS_2P85_EN  L10
    rc = gpio_request(PM8921_GPIO_PM_TO_SYS(SMPS_2P85_EN), "SMPS_2P85_EN");
    rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(SMPS_2P85_EN), &param);
    rc = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(SMPS_2P85_EN), 1);
    if (cam_8921_l10 == NULL) {
        cam_8921_l10 = regulator_get(dev, "hw_8921_l10");
        if (IS_ERR(cam_8921_l10)) {
            CDBG("%s: 8921_l10 get failed\n", __func__);
            cam_8921_l10 = NULL;
            goto cam_2P85_EN_disable;
        }
        if (regulator_set_voltage(cam_8921_l10, 2700000,
			2700000)) {
			CDBG("%s: 8921_l10 set voltage failed\n",
				__func__);
            goto docom_8921_l10_put;
        }
        if (regulator_set_optimum_mode(cam_8921_l10,
			CAM_8960_L10_UA) < 0) {
			CDBG("%s: 8921_l10 set optimum mode failed\n",
				__func__);
            goto docom_8921_l10_set_voltage;
        }
        if (regulator_enable(cam_8921_l10)) {
			CDBG("%s: 8921_l10 enable failed\n", __func__);
            goto docom_8921_l10_set_optimum_mode;
        }        
    }
    mdelay(1);
    //CAM_ISP_1P2_EN
    rc = gpio_request(PM8921_GPIO_PM_TO_SYS(CAM_1P2_EN_SBM), "CAM_ISP_1P2_EN");
    rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(CAM_1P2_EN_SBM), &param);
    rc = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(CAM_1P2_EN_SBM), 1); 
    mdelay(1);

    //lvs5
    if (cam_vio == NULL) {
        cam_vio = regulator_get(dev, "hw_cam_vio");
        if (IS_ERR(cam_vio)) {
			CDBG("%s: VREG VIO get failed\n", __func__);
			cam_vio = NULL;
			goto cam_ISP_1P2_disable;
		}
		if (regulator_enable(cam_vio)) {
			CDBG("%s: VREG VIO enable failed\n", __func__);
			goto cam_vio_put;
		}        
	}
    mdelay(1);
  
    //L11
	if (cam_vana == NULL) {
		cam_vana = regulator_get(dev, "hw_cam_vana");
		if (IS_ERR(cam_vana)) {
			CDBG("%s: VREG CAM VANA get failed\n", __func__);
			cam_vana = NULL;
			goto cam_vio_disable;
		}
		if (regulator_set_voltage(cam_vana, CAM_VANA_MINUV,
			CAM_VANA_MAXUV)) {
			CDBG("%s: VREG CAM VANA set voltage failed\n",
				__func__);
			goto cam_vana_put;
		}
		if (regulator_set_optimum_mode(cam_vana,
			CAM_VANA_LOAD_UA) < 0) {
			CDBG("%s: VREG CAM VANA set optimum mode failed\n",
				__func__);
			goto cam_vana_set_voltage;
		}
		if (regulator_enable(cam_vana)) {
			CDBG("%s: VREG CAM VANA enable failed\n", __func__);
			goto cam_vana_set_optimum_mode;
		}        
	}
    mdelay(1);
     return 0;
	
cam_vana_set_optimum_mode:
	regulator_set_optimum_mode(cam_vana, 0);
cam_vana_set_voltage:
	regulator_set_voltage(cam_vana, 0, CAM_VANA_MAXUV);	
cam_vana_put:
	regulator_put(cam_vana);
	cam_vana = NULL; 
cam_vio_disable:
    regulator_disable(cam_vio);
cam_vio_put:
    regulator_put(cam_vio);
    cam_vio = NULL;
cam_ISP_1P2_disable:
    gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(CAM_1P2_EN_SBM),0);
    usleep_range(1000,2000);
    gpio_free(PM8921_GPIO_PM_TO_SYS(CAM_1P2_EN_SBM));
    regulator_disable(cam_8921_l10);
docom_8921_l10_set_optimum_mode:
    regulator_set_optimum_mode(cam_8921_l10, 0);
docom_8921_l10_set_voltage:
    regulator_set_voltage(cam_8921_l10, 0, 2700000);    
docom_8921_l10_put:
    regulator_put(cam_8921_l10);
    cam_8921_l10 = NULL;
cam_2P85_EN_disable:
    gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(SMPS_2P85_EN),0);
    usleep_range(1000,2000);
    gpio_free(PM8921_GPIO_PM_TO_SYS(SMPS_2P85_EN));
    printk("%s:  enable failed \n", __func__);
	return -ENODEV;
}

void power_seq_disable_docomo(void)
{
	if (cam_vana) {
		regulator_set_voltage(cam_vana, 0, CAM_VANA_MAXUV);
		regulator_set_optimum_mode(cam_vana, 0);
		regulator_disable(cam_vana);
		regulator_put(cam_vana);
		cam_vana = NULL;        
	}
    mdelay(1);
	if (cam_vio) {
		regulator_disable(cam_vio);
		regulator_put(cam_vio);
		cam_vio = NULL;        
	}
    mdelay(1);
    gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(CAM_1P2_EN_SBM),0);
    usleep_range(1000,2000);
    gpio_free(PM8921_GPIO_PM_TO_SYS(CAM_1P2_EN_SBM));
    mdelay(1);
    if (cam_8921_l10) {
		regulator_set_voltage(cam_8921_l10, 0, 2700000);
		regulator_set_optimum_mode(cam_8921_l10, 0);
		regulator_disable(cam_8921_l10);
		regulator_put(cam_8921_l10);
		cam_8921_l10 = NULL;       
	}
    mdelay(1);
    gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(SMPS_2P85_EN),0);
    usleep_range(1000,2000);
    gpio_free(PM8921_GPIO_PM_TO_SYS(SMPS_2P85_EN));
    mdelay(1);
}
