include $(BR2_EXTERNAL_RECALBOX_PATH)/package/download-packages/download-packages.mk
include $(BR2_EXTERNAL_RECALBOX_PATH)/package/compiler-commons/compiler-commons.mk
include $(sort $(wildcard $(BR2_EXTERNAL_RECALBOX_PATH)/package/*/*.mk))
include $(sort $(wildcard $(BR2_EXTERNAL_RECALBOX_PATH)/package/audio/*/*.mk))
include $(sort $(wildcard $(BR2_EXTERNAL_RECALBOX_PATH)/package/firmware/*/*.mk))
include $(sort $(wildcard $(BR2_EXTERNAL_RECALBOX_PATH)/package/hardware/*/*.mk))
include $(sort $(wildcard $(BR2_EXTERNAL_RECALBOX_PATH)/package/libraries/*/*.mk))
include $(sort $(wildcard $(BR2_EXTERNAL_RECALBOX_PATH)/package/utilities/*/*.mk))
