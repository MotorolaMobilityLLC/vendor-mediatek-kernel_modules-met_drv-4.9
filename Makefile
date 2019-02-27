ifeq (y,$(CONFIG_MODULES))
	MET_COMMON_DIR := $(src)/common
	ifeq ($(CONFIG_FTRACE),y)
		ifeq ($(CONFIG_TRACING),y)
			include $(MET_COMMON_DIR)/Kbuild
			MET_DIR := $(wildcard $(src)/$(subst ",,$(CONFIG_MTK_PLATFORM)))
			ifneq ($(MET_DIR),)
				MET_PLF_USE := $(shell test -f $(MET_DIR)/platform/Kbuild && echo yes)
				ifeq ($(MET_PLF_USE),yes)
					include $(MET_DIR)/platform/Kbuild
					ccflags-y += -DMET_PLF_USE
				endif
			endif
		endif
	endif
else
	$(warning Not building met.ko due to CONFIG_MODULES is not set)
endif
