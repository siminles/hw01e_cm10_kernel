/* This file is auto-generated by tool, please don't manully modify it.*/

#include <hsad/config_boardids.h>

#include "hw_U9501L_VA_configs.c"
#include "hw_U9501L_VB_configs.c"
#include "hw_U9501L_VC_configs.c"
#include "hw_U9501L_VD_configs.c"
#include "hw_U9501L_VE_configs.c"
extern struct board_id_general_struct config_power_U9501L_VA;
extern struct board_id_general_struct config_power_U9501L_VB;
extern struct board_id_general_struct config_power_U9501L_VC;
extern struct board_id_general_struct config_power_U9501L_VD;
extern struct board_id_general_struct config_power_U9501L_VE;

/*gpio perl producted data*/
#ifdef CONFIG_HUAWEI_GPIO_UNITE
extern struct board_id_general_struct config_gpio_U9501L_VA;
extern struct board_id_general_struct config_pm_gpio_U9501L_VA;
extern struct board_id_general_struct config_gpio_U9501L_VB;
extern struct board_id_general_struct config_pm_gpio_U9501L_VB;
extern struct board_id_general_struct config_gpio_U9501L_VC;
extern struct board_id_general_struct config_pm_gpio_U9501L_VC;
extern struct board_id_general_struct config_gpio_U9501L_VD;
extern struct board_id_general_struct config_pm_gpio_U9501L_VD;
extern struct board_id_general_struct config_gpio_U9501L_VE;
extern struct board_id_general_struct config_pm_gpio_U9501L_VE;
#endif

/*total table*/
struct board_id_general_struct  *hw_ver_total_configs[] = 
{
#ifdef CONFIG_HUAWEI_GPIO_UNITE
    &config_gpio_U9501L_VA, //gpio
    &config_gpio_U9501L_VB, //gpio
    &config_gpio_U9501L_VC, //gpio
    &config_gpio_U9501L_VD, //gpio
    &config_gpio_U9501L_VE, //gpio
#endif
#ifdef CONFIG_HUAWEI_GPIO_UNITE
    &config_pm_gpio_U9501L_VA, //pm gpio
    &config_pm_gpio_U9501L_VB, //pm gpio
    &config_pm_gpio_U9501L_VC, //pm gpio
    &config_pm_gpio_U9501L_VD, //pm gpio
    &config_pm_gpio_U9501L_VE, //pm gpio
#endif
    &config_common_U9501L_VA, //common xml
    &config_power_U9501L_VA,
    &config_common_U9501L_VB, //common xml
    &config_power_U9501L_VB,
    &config_common_U9501L_VC, //common xml
    &config_power_U9501L_VC,
    &config_common_U9501L_VD, //common xml
    &config_power_U9501L_VD,
    &config_common_U9501L_VE, //common xml
    &config_power_U9501L_VE,
};
