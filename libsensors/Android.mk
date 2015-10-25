ifneq ($(filter titan thea, $(TARGET_DEVICE)),)
include $(call all-named-subdir-makefiles,bst)
else ifneq ($(filter falcon peregrine, $(TARGET_DEVICE)),)
include $(call all-named-subdir-makefiles, falcon-peregrine)
endif
