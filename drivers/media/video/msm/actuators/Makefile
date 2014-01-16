GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
EXTRA_CFLAGS += -Idrivers/media/video/msm
EXTRA_CFLAGS += -Idrivers/media/video/msm/io
obj-$(CONFIG_MSM_ACTUATOR) += msm_actuator.o
obj-$(CONFIG_IMX074_ACT) += imx074_act.o
obj-$(CONFIG_DW9712_ACT) += dw9712_act.o

obj-$(CONFIG_HUAWEI_CAMERA_SENSOR_IMX091_LITEON_ACT) += imx091_liteon_act.o
obj-$(CONFIG_HUAWEI_CAMERA_SENSOR_IMX091_SUNNY_ACT) += imx091_sunny_act.o