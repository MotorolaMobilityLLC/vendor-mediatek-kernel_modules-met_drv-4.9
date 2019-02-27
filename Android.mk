MY_KERNEL_ROOT_DIR := $(PWD)
MY_KERNEL_CONFIG_FILE := $(MY_KERNEL_ROOT_DIR)/kernel-4.9/arch/$(TARGET_ARCH)/configs/$(KERNEL_DEFCONFIG)
MY_KERNEL_CONFIG_MODULES := $(shell grep ^CONFIG_MODULES=y $(MY_KERNEL_CONFIG_FILE))
MY_TEST := CONFIG_MODULES=y

# we should not build ko for some project without define CONFIG_MODULES
LOCAL_PATH := $(call my-dir)

ifneq (,$(filter $(word 2,$(subst -, ,$(LINUX_KERNEL_VERSION))),$(subst /, ,$(LOCAL_PATH))))

include $(CLEAR_VARS)
LOCAL_MODULE := met.ko
LOCAL_STRIP_MODULE := true

ifeq (user,$(TARGET_BUILD_VARIANT))
   MET_INTERNAL_USE := $(shell test -f $(MY_KERNEL_ROOT_DIR)/vendor/mediatek/kernel_modules/met_drv_secure/4.9/init.met.rc && echo yes)
   ifeq ($(MET_INTERNAL_USE),yes)
      LOCAL_INIT_RC := ../../met_drv_secure/4.9/init.met.rc
   endif
endif

include $(MTK_KERNEL_MODULE)

endif # Kernel version matches current path
