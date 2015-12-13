ifneq ($(filter titan thea, $(TARGET_DEVICE)),)
include $(call all-named-subdir-makefiles,bst)
endif
