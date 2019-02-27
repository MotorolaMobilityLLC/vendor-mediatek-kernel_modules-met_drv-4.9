MY_KERNEL_ROOT_DIR := $(PWD)
# we should not build ko for some project without define CONFIG_MODULES
LOCAL_PATH := $(call my-dir)

ifneq (,$(filter $(word 2,$(subst -, ,$(LINUX_KERNEL_VERSION))),$(subst /, ,$(LOCAL_PATH))))

include $(CLEAR_VARS)
LOCAL_MODULE := met.ko
LOCAL_STRIP_MODULE := true

include $(MTK_KERNEL_MODULE)

endif # Kernel version matches current path
