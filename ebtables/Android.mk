#Avoid building ebtables for emulator
ifeq ($(call is-vendor-board-platform,QCOM),true)
    include $(call all-subdir-makefiles)
else
    $(info "ebtables is disabled on this build")
endif
