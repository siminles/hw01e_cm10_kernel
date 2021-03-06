/* Copyright (c) 2010-2011, The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/mfd/pm8xxx/pwm.h>
#include <linux/leds-pm8xxx.h>

/* for button-backlight */
#include <linux/mfd/pm8xxx/mpp.h> 
#include <linux/mfd/pm8xxx/pm8921.h>
#include <hsad/config_interface.h>
#define SSBI_REG_ADDR_DRV_KEYPAD	0x48
#define PM8XXX_DRV_KEYPAD_BL_MASK	0xf0
#define PM8XXX_DRV_KEYPAD_BL_SHIFT	0x04

#define SSBI_REG_ADDR_FLASH_DRV0        0x49
#define PM8XXX_DRV_FLASH_MASK           0xf0
#define PM8XXX_DRV_FLASH_SHIFT          0x04

#define SSBI_REG_ADDR_FLASH_DRV1        0xFB

#define SSBI_REG_ADDR_LED_CTRL_BASE	0x131
#define SSBI_REG_ADDR_LED_CTRL(n)	(SSBI_REG_ADDR_LED_CTRL_BASE + (n))
#define PM8XXX_DRV_LED_CTRL_MASK	0xf8
#define PM8XXX_DRV_LED_CTRL_SHIFT	0x03

#define MAX_FLASH_LED_CURRENT		300
#define MAX_LC_LED_CURRENT		40
#define MAX_KP_BL_LED_CURRENT		300

#define PM8XXX_ID_LED_CURRENT_FACTOR	2  /* Iout = x * 2mA */
#define PM8XXX_ID_FLASH_CURRENT_FACTOR	20 /* Iout = x * 20mA */

#define PM8XXX_FLASH_MODE_DBUS1		1
#define PM8XXX_FLASH_MODE_DBUS2		2
#define PM8XXX_FLASH_MODE_PWM		3

#define MAX_LC_LED_BRIGHTNESS		20
#define MAX_FLASH_BRIGHTNESS		15
#define MAX_KB_LED_BRIGHTNESS		15

#define PM8XXX_LED_OFFSET(id) ((id) - PM8XXX_ID_LED_0)

#define PM8XXX_LED_PWM_FLAGS	(PM_PWM_LUT_LOOP | PM_PWM_LUT_RAMP_UP)
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8XXX_LED_PWM_PERIOD    1000000
/**
 * struct pm8xxx_led_data - internal led data structure
 * @led_classdev - led class device
 * @id - led index
 * @work - workqueue for led
 * @lock - to protect the transactions
 * @reg - cached value of led register
 * @pwm_dev - pointer to PWM device if LED is driven using PWM
 * @pwm_channel - PWM channel ID
 * @pwm_period_us - PWM period in micro seconds
 * @pwm_duty_cycles - struct that describes PWM duty cycles info
 */
struct pm8xxx_led_data {
	struct led_classdev	cdev;
	int			id;
	u8			reg;
	struct device		*dev;
	struct work_struct	work;
	struct mutex		lock;
	struct pwm_device	*pwm_dev;
	int			pwm_channel;
	u32			pwm_period_us;
	u32			on_time;
	struct pm8xxx_pwm_duty_cycles *pwm_duty_cycles;
};
#define LEDS_PWM_FREQ_HZ 300
#define LEDS_PWM_PERIOD_USEC (USEC_PER_SEC / LEDS_PWM_FREQ_HZ)
#define LEDS_PWM_LEVEL 100
#define LEDS_PWM_DUTY_LEVEL \
	(LEDS_PWM_PERIOD_USEC / LEDS_PWM_LEVEL)

static struct pwm_device *bl_lpm;
static void leds_panel_pwm_cfg(void)
{
	int rc;
	static struct pm_gpio pwm_mode = {
		.direction        = PM_GPIO_DIR_OUT,
		.output_buffer    = PM_GPIO_OUT_BUF_CMOS,
		.output_value     = 0,
		.pull             = PM_GPIO_PULL_NO,
		.vin_sel          = PM_GPIO_VIN_S4,
		.out_strength     = PM_GPIO_STRENGTH_HIGH,
		.function         = PM_GPIO_FUNC_2,
		.inv_int_pol      = 0,
		.disable_pin      = 0,
	};
	/* pm8xxx: gpio-24, Bl: Off, PWM mode */
	rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(24),
					&pwm_mode);
	if (rc != 0)
		pr_err("%s: pwm_mode failed\n", __func__);
}

static void led_kp_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
    /* for button-backlight */
#if 0
	int rc;
	u8 level;

	level = (value << PM8XXX_DRV_KEYPAD_BL_SHIFT) &
				 PM8XXX_DRV_KEYPAD_BL_MASK;

	led->reg &= ~PM8XXX_DRV_KEYPAD_BL_MASK;
	led->reg |= level;

	rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_DRV_KEYPAD,
								led->reg);
	if (rc < 0)
		dev_err(led->cdev.dev,
			"can't set keypad backlight level rc=%d\n", rc);
#endif
    struct pm8xxx_mpp_config_data  mpp_config_data;
    unsigned    nmpp;
    int ret = 0;
    if(bl_lpm) {
		int max_brightness = get_leds_max_brightness();
		if(-1 == max_brightness) {
            max_brightness = 15;
		}
		pr_info("%s, max brightness: %d.\n",__func__, max_brightness);
		if(value > max_brightness) {
            value = max_brightness;
		}

		ret = pwm_config(bl_lpm, LEDS_PWM_DUTY_LEVEL *
            value, LEDS_PWM_PERIOD_USEC);
		if (ret) {
            pr_err("pwm_config on lpm failed %d\n", ret);
            return;
		}
		if (value) {
            ret = pwm_enable(bl_lpm);
            if (ret)
				pr_err("pwm enable/disable on lpm failed"
						"for bl %d\n",	value);
		} else {
            pwm_disable(bl_lpm);
		}
    } else {
		if(value > PM8XXX_MPP_CS_OUT_40MA)
            value = PM8XXX_MPP_CS_OUT_40MA;
		mpp_config_data.type =  PM8XXX_MPP_TYPE_SINK;
		mpp_config_data.level = PM8XXX_MPP_CS_OUT_5MA;
		if(value == 0) {
            mpp_config_data.control = PM8XXX_MPP_CS_CTRL_MPP_HIGH_EN;
		} else {
            mpp_config_data.control = PM8XXX_MPP_CS_CTRL_MPP_LOW_EN;
		}

#ifdef CONFIG_HUAWEI_KERNEL
		nmpp = led->cdev.gpio;
#endif

		pm8xxx_mpp_config(nmpp, &mpp_config_data);
    }
}

static void led_lc_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc, offset;
	u8 level;

	level = (value << PM8XXX_DRV_LED_CTRL_SHIFT) &
				PM8XXX_DRV_LED_CTRL_MASK;

	offset = PM8XXX_LED_OFFSET(led->id);

	led->reg &= ~PM8XXX_DRV_LED_CTRL_MASK;
	led->reg |= level;

	if(!strcmp(led->cdev.name,"red")) {
		led->reg |= PM8XXX_LED_MODE_PWM3;
	} else if(!strcmp(led->cdev.name,"green")) {
		led->reg |= PM8XXX_LED_MODE_PWM2;
	} else if(!strcmp(led->cdev.name,"blue")) {
		led->reg |= PM8XXX_LED_MODE_PWM1;
	}
	rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset),
								led->reg);
	if (rc)
		dev_err(led->cdev.dev, "can't set (%d) led value rc=%d\n",
				led->id, rc);
}

static void
led_flash_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc;
	u8 level;
	u16 reg_addr;

	level = (value << PM8XXX_DRV_FLASH_SHIFT) &
				 PM8XXX_DRV_FLASH_MASK;

	led->reg &= ~PM8XXX_DRV_FLASH_MASK;
	led->reg |= level;

	if (led->id == PM8XXX_ID_FLASH_LED_0)
		reg_addr = SSBI_REG_ADDR_FLASH_DRV0;
	else
		reg_addr = SSBI_REG_ADDR_FLASH_DRV1;

	rc = pm8xxx_writeb(led->dev->parent, reg_addr, led->reg);
	if (rc < 0)
		dev_err(led->cdev.dev, "can't set flash led%d level rc=%d\n",
			 led->id, rc);
}

static int pm8xxx_led_pwm_work(struct pm8xxx_led_data *led)
{
	int duty_us;
	int rc = 0;

	if (led->pwm_duty_cycles == NULL) {
		 //duty_us =(led->pwm_period_us * led->cdev.brightness) /
								//LED_FULL;
		duty_us = led->on_time;
		rc = pwm_config(led->pwm_dev, duty_us, led->pwm_period_us);
		if (led->cdev.brightness)
			rc = pwm_enable(led->pwm_dev);
		else
			pwm_disable(led->pwm_dev);
	} else {
		rc = pm8xxx_pwm_lut_enable(led->pwm_dev, led->cdev.brightness);
	}

	return rc;
}

static void __pm8xxx_led_work(struct pm8xxx_led_data *led,
					enum led_brightness level)
{
	mutex_lock(&led->lock);

	switch (led->id) {
	case PM8XXX_ID_LED_KB_LIGHT:
		led_kp_set(led, level);
	break;
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		led_lc_set(led, level);
	break;
	case PM8XXX_ID_FLASH_LED_0:
	case PM8XXX_ID_FLASH_LED_1:
		led_flash_set(led, level);
	break;
	}

	mutex_unlock(&led->lock);
}

static void pm8xxx_led_work(struct work_struct *work)
{
	int rc;

	struct pm8xxx_led_data *led = container_of(work,
					 struct pm8xxx_led_data, work);

	if (led->pwm_dev == NULL) {
		__pm8xxx_led_work(led, led->cdev.brightness);
	} else {
		rc = pm8xxx_led_pwm_work(led);
		if (rc)
			pr_err("could not configure PWM mode for LED:%d\n",
								led->id);
	}
}

static void pm8xxx_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct	pm8xxx_led_data *led;

	led = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	if (value < LED_OFF || value > led->cdev.max_brightness) {
		dev_err(led->cdev.dev, "Invalid brightness value exceeds");
		return;
	}
	if(!strcmp(led->cdev.name,"red") || !strcmp(led->cdev.name,"blue")) {
		if(value)
			value = 3;
	} else if(!strcmp(led->cdev.name,"green")) {
		if(value)
			value = 1;
	}
	led->cdev.brightness = value;
	//schedule_work(&led->work);
	__pm8xxx_led_work(led,led->cdev.brightness);
}

static int pm8xxx_set_led_mode_and_max_brightness(struct pm8xxx_led_data *led,
		enum pm8xxx_led_modes led_mode, int max_current)
{
	int rc = 0;

	switch (led->id) {
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		led->cdev.max_brightness = max_current /
						PM8XXX_ID_LED_CURRENT_FACTOR;
		if (led->cdev.max_brightness > MAX_LC_LED_BRIGHTNESS)
			led->cdev.max_brightness = MAX_LC_LED_BRIGHTNESS;
		led->reg = led_mode;
		break;
	case PM8XXX_ID_LED_KB_LIGHT:
	case PM8XXX_ID_FLASH_LED_0:
	case PM8XXX_ID_FLASH_LED_1:
		led->cdev.max_brightness = max_current /
						PM8XXX_ID_FLASH_CURRENT_FACTOR;
		if (led->cdev.max_brightness > MAX_FLASH_BRIGHTNESS)
			led->cdev.max_brightness = MAX_FLASH_BRIGHTNESS;

		switch (led_mode) {
		case PM8XXX_LED_MODE_PWM1:
		case PM8XXX_LED_MODE_PWM2:
		case PM8XXX_LED_MODE_PWM3:
			led->reg = PM8XXX_FLASH_MODE_PWM;
			break;
		case PM8XXX_LED_MODE_DTEST1:
			led->reg = PM8XXX_FLASH_MODE_DBUS1;
			break;
		case PM8XXX_LED_MODE_DTEST2:
			led->reg = PM8XXX_FLASH_MODE_DBUS2;
			break;
		default:
			led->reg = PM8XXX_LED_MODE_MANUAL;
			break;
		}
		break;
	default:
		rc = -EINVAL;
		pr_err("LED Id is invalid");
		break;
	}

	return rc;
}

static enum led_brightness pm8xxx_led_get(struct led_classdev *led_cdev)
{
	struct pm8xxx_led_data *led;

	led = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	return led->cdev.brightness;
}

static int __devinit get_init_value(struct pm8xxx_led_data *led, u8 *val)
{
	int rc, offset;
	u16 addr;

	switch (led->id) {
	case PM8XXX_ID_LED_KB_LIGHT:
		addr = SSBI_REG_ADDR_DRV_KEYPAD;
		break;
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		offset = PM8XXX_LED_OFFSET(led->id);
		addr = SSBI_REG_ADDR_LED_CTRL(offset);
		break;
	case PM8XXX_ID_FLASH_LED_0:
		addr = SSBI_REG_ADDR_FLASH_DRV0;
		break;
	case PM8XXX_ID_FLASH_LED_1:
		addr = SSBI_REG_ADDR_FLASH_DRV1;
		break;
	}

	rc = pm8xxx_readb(led->dev->parent, addr, val);
	if (rc)
		dev_err(led->cdev.dev, "can't get led(%d) level rc=%d\n",
							led->id, rc);

	return rc;
}

static int pm8xxx_led_pwm_configure(struct pm8xxx_led_data *led)
{
	int start_idx, idx_len, duty_us, rc;

	led->pwm_dev = pwm_request(led->pwm_channel,
					led->cdev.name);

	if (IS_ERR_OR_NULL(led->pwm_dev)) {
		pr_err("could not acquire PWM Channel %d, "
			"error %ld\n", led->pwm_channel,
			PTR_ERR(led->pwm_dev));
		led->pwm_dev = NULL;
		return -ENODEV;
	}

	if (led->pwm_duty_cycles != NULL) {
		start_idx = led->pwm_duty_cycles->start_idx;
		idx_len = led->pwm_duty_cycles->num_duty_pcts;

		if (idx_len >= PM_PWM_LUT_SIZE && start_idx) {
			pr_err("Wrong LUT size or index\n");
			return -EINVAL;
		}
		if ((start_idx + idx_len) > PM_PWM_LUT_SIZE) {
			pr_err("Exceed LUT limit\n");
			return -EINVAL;
		}

		rc = pm8xxx_pwm_lut_config(led->pwm_dev, led->pwm_period_us,
				led->pwm_duty_cycles->duty_pcts,
				led->pwm_duty_cycles->duty_ms,
				start_idx, idx_len, 0, 0,
				PM8XXX_LED_PWM_FLAGS);
	} else {
		duty_us = led->pwm_period_us;
		rc = pwm_config(led->pwm_dev, duty_us, led->pwm_period_us);
	}

	return rc;
}

static ssize_t led_grpfreq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pm8xxx_led_data *led = dev_get_drvdata(dev);
	u32 led_freq = 0;
	led_freq = led->pwm_period_us/ 50 / 1000;

	return snprintf(buf, 50, "%u\n", led_freq);
}

static ssize_t led_grpfreq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct pm8xxx_led_data *led = dev_get_drvdata(dev);
	ssize_t ret;
	unsigned long state = 0;
	ret = strict_strtol(buf, 10, &state);
	if (ret < 0)
		return ret;
	led->pwm_period_us = state * 50 * 1000;
	return size;
}
DEVICE_ATTR(grpfreq,0644,led_grpfreq_show,led_grpfreq_store);

static ssize_t led_pwm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pm8xxx_led_data *led = dev_get_drvdata(dev);
	u32 led_duty = 0;
	led_duty =led->on_time / (led->pwm_period_us / 255);
	return snprintf(buf, 50, "%u\n", led_duty);
}

static ssize_t led_pwm_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct pm8xxx_led_data *led = dev_get_drvdata(dev);
	ssize_t ret;
	unsigned long state = 0;
	ret = strict_strtol(buf, 10, &state);
	if (ret < 0)
		return ret;
	if(led->pwm_period_us > 16000000)
	   led->on_time = state * (led->pwm_period_us / 255);
	else
	   led->on_time = state * led->pwm_period_us / 255;
	return size;
}
DEVICE_ATTR(grppwm,0644,led_pwm_show,led_pwm_store);

static ssize_t led_blink_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pm8xxx_led_data *led = dev_get_drvdata(dev);
	u32 led_blink = 0;
	led_blink = (led->cdev.brightness) ? 1 : 0;

	return snprintf(buf, 50, "%u\n", led_blink);
}

static ssize_t led_blink_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct pm8xxx_led_data *led = dev_get_drvdata(dev);
	ssize_t ret;
	unsigned long state = 0;
	ret = strict_strtol(buf, 10, &state);
	if (ret < 0)
		return ret;
	if(state) {
		if(led->pwm_period_us == 0)
			led->pwm_period_us = PM8XXX_LED_PWM_PERIOD;
		if (led->on_time == 0)
			led->on_time = led->pwm_period_us;
		pm8xxx_led_pwm_work(led);
	} else {
		pwm_disable(led->pwm_dev);
	}
	return size;
}
DEVICE_ATTR(blink,0644,led_blink_show,led_blink_store);

static struct attribute *led_attributes[] =
{
	&dev_attr_grpfreq.attr,
	&dev_attr_grppwm.attr,
	&dev_attr_blink.attr,
	NULL,
};

static const struct attribute_group led_attr_group = {
    .attrs = led_attributes,
};

static int __devinit pm8xxx_led_probe(struct platform_device *pdev)
{
	const struct pm8xxx_led_platform_data *pdata = pdev->dev.platform_data;
	const struct led_platform_data *pcore_data;
	struct led_info *curr_led;
	struct pm8xxx_led_data *led, *led_dat;
	struct pm8xxx_led_config *led_cfg;
	int rc, i;

	if (pdata == NULL) {
		dev_err(&pdev->dev, "platform data not supplied\n");
		return -EINVAL;
	}

	pcore_data = pdata->led_core;

	if (pcore_data->num_leds != pdata->num_configs) {
		dev_err(&pdev->dev, "#no. of led configs and #no. of led"
				"entries are not equal\n");
		return -EINVAL;
	}

	led = kcalloc(pcore_data->num_leds, sizeof(*led), GFP_KERNEL);
	if (led == NULL) {
		dev_err(&pdev->dev, "failed to alloc memory\n");
		return -ENOMEM;
	}
	if(is_leds_ctl_pwm()) {
		 bl_lpm = pwm_request(0,"backlight");
		  if (bl_lpm == NULL || IS_ERR(bl_lpm)) {
			  printk("%s pwm_request() failed\n", __func__);
			  bl_lpm = NULL;
		  }
		  else
			  leds_panel_pwm_cfg();
	}

	for (i = 0; i < pcore_data->num_leds; i++) {
        if( (is_leds_ctl_tca6507()) && (0 == i))
            continue;
        
		curr_led	= &pcore_data->leds[i];
		led_dat		= &led[i];
		led_cfg		= &pdata->configs[i];

		led_dat->id     = led_cfg->id;
		led_dat->pwm_channel = led_cfg->pwm_channel;
		led_dat->pwm_period_us = led_cfg->pwm_period_us;
		led_dat->pwm_duty_cycles = led_cfg->pwm_duty_cycles;

		if (!((led_dat->id >= PM8XXX_ID_LED_KB_LIGHT) &&
				(led_dat->id <= PM8XXX_ID_FLASH_LED_1))) {
			dev_err(&pdev->dev, "invalid LED ID (%d) specified\n",
						 led_dat->id);
			rc = -EINVAL;
			goto fail_id_check;
		}

		led_dat->cdev.name		= curr_led->name;
		led_dat->cdev.default_trigger   = curr_led->default_trigger;
            /* for button-backlight */
#ifdef CONFIG_HUAWEI_KERNEL
		led_dat->cdev.gpio = curr_led->gpio;
#endif
		led_dat->cdev.brightness_set    = pm8xxx_led_set;
		led_dat->cdev.brightness_get    = pm8xxx_led_get;
		led_dat->cdev.brightness	= LED_OFF;
		led_dat->cdev.flags		= curr_led->flags;
		led_dat->dev			= &pdev->dev;

		rc =  get_init_value(led_dat, &led_dat->reg);
		if (rc < 0)
			goto fail_id_check;

		rc = pm8xxx_set_led_mode_and_max_brightness(led_dat,
					led_cfg->mode, led_cfg->max_current);
		if (rc < 0)
			goto fail_id_check;

		mutex_init(&led_dat->lock);
		INIT_WORK(&led_dat->work, pm8xxx_led_work);

		rc = led_classdev_register(&pdev->dev, &led_dat->cdev);
		if (rc) {
			dev_err(&pdev->dev, "unable to register led %d,rc=%d\n",
						 led_dat->id, rc);
			goto fail_id_check;
		}

		if (led_cfg->mode != PM8XXX_LED_MODE_MANUAL) {
			__pm8xxx_led_work(led_dat,
					led_dat->cdev.max_brightness);

			if (led_dat->pwm_channel != -1) {
				led_dat->cdev.max_brightness = LED_FULL;
				rc = pm8xxx_led_pwm_configure(led_dat);
				if (rc) {
					dev_err(&pdev->dev, "failed to "
					"configure LED, error: %d\n", rc);
					goto fail_id_check;
				}
			}
		} else {
			__pm8xxx_led_work(led_dat, LED_OFF);
		}
		rc = sysfs_create_group(&led_dat->cdev.dev->kobj, &led_attr_group);
		if (rc) {
			goto fail_id_check;
		}
	}

	platform_set_drvdata(pdev, led);

	return 0;

fail_id_check:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			mutex_destroy(&led[i].lock);
			led_classdev_unregister(&led[i].cdev);
			if (led[i].pwm_dev != NULL)
				pwm_free(led[i].pwm_dev);
		}
	}
	kfree(led);
	return rc;
}

static int __devexit pm8xxx_led_remove(struct platform_device *pdev)
{
	int i;
	const struct led_platform_data *pdata =
				pdev->dev.platform_data;
	struct pm8xxx_led_data *led = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		cancel_work_sync(&led[i].work);
		mutex_destroy(&led[i].lock);
		sysfs_remove_group(&led[i].cdev.dev->kobj, &led_attr_group);
		led_classdev_unregister(&led[i].cdev);
		if (led[i].pwm_dev != NULL)
			pwm_free(led[i].pwm_dev);
	}

	kfree(led);

	return 0;
}

static struct platform_driver pm8xxx_led_driver = {
	.probe		= pm8xxx_led_probe,
	.remove		= __devexit_p(pm8xxx_led_remove),
	.driver		= {
		.name	= PM8XXX_LEDS_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8xxx_led_init(void)
{
	return platform_driver_register(&pm8xxx_led_driver);
}
subsys_initcall(pm8xxx_led_init);

static void __exit pm8xxx_led_exit(void)
{
	platform_driver_unregister(&pm8xxx_led_driver);
}
module_exit(pm8xxx_led_exit);

MODULE_DESCRIPTION("PM8XXX LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pm8xxx-led");
