MY_KERNEL_ROOT_DIR := $(PWD)
MY_KERNEL_CONFIG_FILE := $(MY_KERNEL_ROOT_DIR)/kernel-4.9/arch/$(TARGET_ARCH)/configs/$(KERNEL_DEFCONFIG)
MY_KERNEL_CONFIG_MODULES := $(shell grep ^CONFIG_MODULES=y $(MY_KERNEL_CONFIG_FILE))
MY_TEST := CONFIG_MODULES=y

# we should not build ko for some project without define CONFIG_MODULES
LOCAL_PATH := $(call my-dir)

ifneq (true,$(strip $(TARGET_NO_KERNEL)))
ifeq ($(LINUX_KERNEL_VERSION),kernel-$(lastword $(subst /, ,$(LOCAL_PATH))))

ifeq (,$(KERNEL_OUT))
include $(LINUX_KERNEL_VERSION)/kenv.mk
endif

include $(CLEAR_VARS)
LOCAL_MODULE := met.ko
LOCAL_STRIP_MODULE := true

ifeq (user,$(TARGET_BUILD_VARIANT))
   MET_INTERNAL_USE := $(shell test -f $(MY_KERNEL_ROOT_DIR)/vendor/mediatek/kernel_modules/met_drv_secure/4.9/init.met.rc && echo yes)
   ifeq ($(MET_INTERNAL_USE),yes)
      LOCAL_INIT_RC := ../../met_drv_secure/4.9/init.met.rc
   endif
endif

LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := first
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/modules
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f \( -name '*.[choS]' -o -name Kbuild \))) Makefile
LOCAL_POST_INSTALL_CMD := $(hide)$(TARGET_STRIP) --strip-unneeded $(LOCAL_MODULE_PATH)/$(LOCAL_MODULE)

include $(BUILD_SYSTEM)/base_rules.mk

LOCAL_GENERATED_SOURCES := $(addprefix $(intermediates)/,$(LOCAL_SRC_FILES))

$(LOCAL_GENERATED_SOURCES): $(intermediates)/% : $(LOCAL_PATH)/% | $(ACP)
	@echo "Copy: $@"
	$(copy-file-to-target)

$(KERNEL_OUT)/scripts/sign-file: $(KERNEL_ZIMAGE_OUT);

$(LOCAL_BUILT_MODULE): KOUT := $(KERNEL_OUT)
$(LOCAL_BUILT_MODULE): OPTS := \
  $(KERNEL_MAKE_OPTION) M=$(abspath $(intermediates))
$(LOCAL_BUILT_MODULE): CERT_PATH := $(LINUX_KERNEL_VERSION)/certs
$(LOCAL_BUILT_MODULE): $(wildcard $(LINUX_KERNEL_VERSION)/certs/ko_prvk.pem)
$(LOCAL_BUILT_MODULE): $(wildcard $(LINUX_KERNEL_VERSION)/certs/ko_pubk.x509.der)
$(LOCAL_BUILT_MODULE): $(wildcard vendor/mediatek/proprietary/scripts/kernel_tool/rm_ko_sig.py)
$(LOCAL_BUILT_MODULE): $(LOCAL_GENERATED_SOURCES) $(KERNEL_OUT)/scripts/sign-file
	@echo $@: $^
	$(MAKE) -C $(KOUT) $(OPTS)
	$(hide) $(call sign-kernel-module,$(KOUT)/scripts/sign-file,$(CERT_PATH)/ko_prvk.pem,$(CERT_PATH)/ko_pubk.x509.der)

endif # Kernel version matches current path
endif # TARGET_NO_KERNEL != true
