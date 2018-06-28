ifeq (y,$(CONFIG_MODULES))

ifeq ($(CONFIG_FTRACE),y)
    ifeq ($(CONFIG_TRACING),y)
        FTRACE_READY := y
    endif
endif

PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
MET_DIR := $(wildcard $(src)/$(PLATFORM))
MET_COMMON_USE := $(shell test -d $(src)/common && echo yes)

$(info ******** Start to build met_drv for $(PLATFORM) ********)


ifeq ($(MET_COMMON_USE),yes)
    # met common code structure, new build flow
    MET_COMMON_DIR := $(src)/common
    ifeq ($(FTRACE_READY),y)
        include $(MET_COMMON_DIR)/Kbuild
        ccflags-y += -DCONFIG_MET_MODULE
        ccflags-y += -I$(MET_COMMON_DIR)/
    else
        $(warning Not building met.ko due to CONFIG_FTRACE/CONFIG_TRACING is not set)
    endif
else ifneq ($(MET_DIR),)
    # not met common code structure, old build flow
    ifeq ($(FTRACE_READY),y)
        include $(MET_DIR)/core/Kbuild
        MET_PLF_USE := $(shell test -f $(MET_DIR)/platform/Kbuild && echo yes)
        ifeq ($(MET_PLF_USE),yes)
            include $(MET_DIR)/platform/Kbuild
        endif
    else
        $(warning Not building met.ko due to CONFIG_FTRACE/CONFIG_TRACING is not set)
    endif
else
    $(warning not support $(PLATFORM), build met default)
    MET_DEF_DIR := $(src)
    include $(MET_DEF_DIR)/default/Kbuild
endif
else
$(warning Not building met.ko due to CONFIG_MODULES is not set)
endif
