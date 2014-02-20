LOCAL_PATH := $(call my-dir)

## Don't change anything under here. The variables are named MSM8226_whatever
## on purpose, to avoid conflicts with similarly named variables at other
## parts of the build environment

## Imported from the original makefile...
KERNEL_CONFIG := $(KERNEL_OUT)/.config
MSM8226_DTS_NAMES := msm8226

MSM8226_DTS_FILES = $(wildcard $(TOP)/$(TARGET_KERNEL_SOURCE)/arch/arm/boot/dts/msm8226-falcon*.dts)
MSM8226_DTS_FILE = $(lastword $(subst /, ,$(1)))
DTB_FILE = $(addprefix $(KERNEL_OUT)/arch/arm/boot/,$(patsubst %.dts,%.dtb,$(call MSM8226_DTS_FILE,$(1))))
ZIMG_FILE = $(addprefix $(KERNEL_OUT)/arch/arm/boot/,$(patsubst %.dts,%-zImage,$(call MSM8226_DTS_FILE,$(1))))
KERNEL_ZIMG = $(KERNEL_OUT)/arch/arm/boot/zImage
DTC = $(KERNEL_OUT)/scripts/dtc/dtc

define append-msm8226-dtb
mkdir -p $(KERNEL_OUT)/arch/arm/boot;\
$(foreach MSM8226_DTS_NAME, $(MSM8226_DTS_NAMES), \
   $(foreach d, $(MSM8226_DTS_FILES), \
      $(DTC) -p 1024 -O dtb -o $(call DTB_FILE,$(d)) $(d); \
      cat $(KERNEL_ZIMG) $(call DTB_FILE,$(d)) > $(call ZIMG_FILE,$(d));))
endef


## Build and run dtbtool
DTBTOOL := $(HOST_OUT_EXECUTABLES)/dtbTool$(HOST_EXECUTABLE_SUFFIX)
INSTALLED_DTIMAGE_TARGET := $(PRODUCT_OUT)/dt.img

$(INSTALLED_DTIMAGE_TARGET): $(DTBTOOL) $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr $(INSTALLED_KERNEL_TARGET)
	@echo -e ${CL_CYN}"Start DT image: $@"${CL_RST}
	$(call append-msm8226-dtb)
	$(call pretty,"Target dt image: $(INSTALLED_DTIMAGE_TARGET)")
	$(hide) $(DTBTOOL) -o $(INSTALLED_DTIMAGE_TARGET) -s $(BOARD_KERNEL_PAGESIZE) -p $(KERNEL_OUT)/scripts/dtc/ $(KERNEL_OUT)/arch/arm/boot/
	@echo -e ${CL_CYN}"Made DT image: $@"${CL_RST}


## Overload bootimg generation: Same as the original, + --dt arg
$(INSTALLED_BOOTIMAGE_TARGET): $(MKBOOTIMG) $(INTERNAL_BOOTIMAGE_FILES) $(INSTALLED_DTIMAGE_TARGET)
	$(call pretty,"Target boot image: $@")
	$(hide) $(MKBOOTIMG) $(INTERNAL_BOOTIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --dt $(INSTALLED_DTIMAGE_TARGET) --output $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
	@echo -e ${CL_CYN}"Made boot image: $@"${CL_RST}
	$(call gpe_copy_root)

## Overload recoveryimg generation: Same as the original, + --dt arg
$(INSTALLED_RECOVERYIMAGE_TARGET): $(MKBOOTIMG) $(INSTALLED_DTIMAGE_TARGET) \
		$(recovery_ramdisk) \
		$(recovery_kernel)
	@echo -e ${CL_CYN}"----- Making recovery image ------"${CL_RST}
	$(hide) $(MKBOOTIMG) $(INTERNAL_RECOVERYIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --dt $(INSTALLED_DTIMAGE_TARGET) --output $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)
	@echo -e ${CL_CYN}"Made recovery image: $@"${CL_RST}
	$(call gpe_copy_recovery)

#----------------------------------------------------------------------
# Generate second boot/recovery image for GPE as it uses ext4 for data
#----------------------------------------------------------------------
GPE_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/system/gpe-boot.img
GPE_RAMDISK_TARGET := $(PRODUCT_OUT)/gpe-ramdisk.img
GPE_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/gpe-recovery.img
gpe_recovery_uncompressed_ramdisk := $(PRODUCT_OUT)/gpe-ramdisk-recovery.cpio
gpe_recovery_ramdisk := $(PRODUCT_OUT)/gpe-ramdisk-recovery.img
TARGET_GPE_ROOT_OUT := $(PRODUCT_OUT)/gpe-root
TARGET_GPE_RECOVERY_OUT := $(PRODUCT_OUT)/gpe-recovery
TARGET_GPE_RECOVERY_ROOT_OUT := $(TARGET_GPE_RECOVERY_OUT)/root
gpe_recovery_fstab := $(LOCAL_PATH)/rootdir/etc/gpe-fstab.qcom
ALL_DEFAULT_INSTALLED_MODULES += $(GPE_RAMDISK_TARGET) $(GPE_BOOTIMAGE_TARGET) $(GPE_RECOVERYIMAGE_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(GPE_RAMDISK_TARGET) $(GPE_BOOTIMAGE_TARGET) $(GPE_RECOVERYIMAGE_TARGET)

.PHONY: gpe-bootimage
gpe-bootimage: $(INSTALLED_BOOTIMAGE_TARGET) $(GPE_RAMDISK_TARGET) | $(GPE_BOOTIMAGE_TARGET)

.PHONY: gpe-recoveryimage
gpe-recoveryimage: $(INSTALLED_RECOVERYIMAGE_TARGET) | $(GPE_RECOVERYIMAGE_TARGET) $(GPE_RECOVERY_RESOURCE_ZIP)

define gpe_copy_root
	$(hide) rm -rf $(TARGET_GPE_ROOT_OUT)
	$(hide) mkdir -p $(TARGET_GPE_ROOT_OUT)
	$(hide) cp -R $(TARGET_ROOT_OUT)/* $(TARGET_GPE_ROOT_OUT)
	$(hide) cp $(LOCAL_PATH)/rootdir/etc/gpe-fstab.qcom $(TARGET_GPE_ROOT_OUT)/fstab.qcom
endef

define gpe_copy_recovery
	$(hide) rm -rf $(TARGET_GPE_RECOVERY_OUT)
	$(hide) mkdir -p $(TARGET_GPE_RECOVERY_ROOT_OUT)
	$(hide) cp -R $(TARGET_RECOVERY_OUT)/* $(TARGET_GPE_RECOVERY_OUT)
	$(hide) cp $(LOCAL_PATH)/rootdir/etc/gpe-fstab.qcom $(TARGET_GPE_RECOVERY_ROOT_OUT)/fstab.qcom
endef

GPE_RAMDISK_FILES := $(filter $(TARGET_GPE_ROOT_OUT)/%, \
	$(ALL_PREBUILT) \
	$(ALL_COPIED_HEADERS) \
	$(ALL_GENERATED_SOURCES) \
	$(ALL_DEFAULT_INSTALLED_MODULES))

$(GPE_RAMDISK_TARGET): $(MKBOOTFS) $(GPE_RAMDISK_FILES) | $(MINIGZIP)
	$(call pretty,"Target gpe ram disk: $@")
	$(hide) $(MKBOOTFS) $(TARGET_GPE_ROOT_OUT) | $(MINIGZIP) > $@

GPE_BOOTIMAGE_FILES := $(filter-out --%,$(GPE_BOOTIMAGE_ARGS))

GPE_BOOTIMAGE_ARGS := \
	--kernel $(INSTALLED_KERNEL_TARGET) \
	--ramdisk $(GPE_RAMDISK_TARGET)

BOARD_KERNEL_CMDLINE := $(strip $(BOARD_KERNEL_CMDLINE))
ifdef BOARD_KERNEL_CMDLINE
  GPE_BOOTIMAGE_ARGS += --cmdline "$(BOARD_KERNEL_CMDLINE)"
endif

BOARD_KERNEL_BASE := $(strip $(BOARD_KERNEL_BASE))
ifdef BOARD_KERNEL_BASE
  GPE_BOOTIMAGE_ARGS += --base $(BOARD_KERNEL_BASE)
endif

BOARD_KERNEL_PAGESIZE := $(strip $(BOARD_KERNEL_PAGESIZE))
ifdef BOARD_KERNEL_PAGESIZE
  GPE_BOOTIMAGE_ARGS += --pagesize $(BOARD_KERNEL_PAGESIZE)
endif

## Overload GPE bootimg generation: Same as the original, + --dt arg
$(GPE_BOOTIMAGE_TARGET): $(MKBOOTIMG) $(GPE_BOOTIMAGE_FILES) $(INSTALLED_DTIMAGE_TARGET) $(GPE_RAMDISK_TARGET) $(GPE_RECOVERYIMAGE_TARGET)
	$(call pretty,"Target GPE boot image: $@")
	$(hide) $(MKBOOTIMG) $(GPE_BOOTIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --dt $(INSTALLED_DTIMAGE_TARGET) --output $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
	@echo -e ${CL_CYN}"Made GPE boot image: $@"${CL_RST}

GPE_RECOVERYIMAGE_ARGS := \
	--kernel $(recovery_kernel) \
	--ramdisk $(gpe_recovery_ramdisk)

# Assumes this has already been stripped
ifdef BOARD_KERNEL_CMDLINE
  GPE_RECOVERYIMAGE_ARGS += --cmdline "$(BOARD_KERNEL_CMDLINE)"
endif
ifdef BOARD_KERNEL_BASE
  GPE_RECOVERYIMAGE_ARGS += --base $(BOARD_KERNEL_BASE)
endif
BOARD_KERNEL_PAGESIZE := $(strip $(BOARD_KERNEL_PAGESIZE))
ifdef BOARD_KERNEL_PAGESIZE
  GPE_RECOVERYIMAGE_ARGS += --pagesize $(BOARD_KERNEL_PAGESIZE)
endif
ifneq ($(BOARD_FORCE_RAMDISK_ADDRESS),)
  GPE_RECOVERYIMAGE_ARGS += --ramdiskaddr $(BOARD_FORCE_RAMDISK_ADDRESS)
endif

GPE_RECOVERY_FILES := $(filter $(TARGET_GPE_RECOVERY_OUT)/%, \
	$(foreach module, $(ALL_MODULES), $(ALL_MODULES.$(module).INSTALLED)) \
	$(ALL_DEFAULT_INSTALLED_MODULES))

TARGET_GPE_RECOVERY_ROOT_TIMESTAMP := $(TARGET_GPE_RECOVERY_OUT)/root.ts

$(TARGET_GPE_RECOVERY_ROOT_TIMESTAMP): $(GPE_RECOVERY_FILES) \
		$(GPE_RAMDISK_TARGET) \
		$(MKBOOTIMG) $(GPE_BOOTIMAGE_FILES) \
		$(recovery_binary) \
		$(recovery_initrc) \
		$(recovery_build_prop) $(recovery_resource_deps) \
		$(gpe_recovery_fstab) \
		$(RECOVERY_INSTALL_OTA_KEYS)
	@echo -e ${CL_CYN}"----- Making GPE recovery filesystem ------"${CL_RST}
	$(hide) mkdir -p $(TARGET_GPE_RECOVERY_OUT)
	$(hide) mkdir -p $(TARGET_GPE_RECOVERY_ROOT_OUT)/etc $(TARGET_GPE_RECOVERY_ROOT_OUT)/tmp
	@echo -e ${CL_CYN}"Copying baseline GPE ramdisk..."${CL_RST}
	$(hide) cp -R $(TARGET_ROOT_OUT) $(TARGET_GPE_RECOVERY_OUT)
	@echo -e ${CL_CYN}"Modifying GPE ramdisk contents..."${CL_RST}
	$(hide) rm -f $(TARGET_GPE_RECOVERY_ROOT_OUT)/init*.rc
	$(hide) cp -f $(recovery_initrc) $(TARGET_GPE_RECOVERY_ROOT_OUT)/init.rc

	$(hide) -cp $(TARGET_ROOT_OUT)/init.recovery.*.rc $(TARGET_GPE_RECOVERY_ROOT_OUT)/
	$(hide) cp -f $(recovery_binary) $(TARGET_GPE_RECOVERY_ROOT_OUT)/sbin/
	mkdir -p $(TARGET_GPE_RECOVERY_ROOT_OUT)/system/bin
	$(hide) cp -rf $(recovery_resources_common) $(TARGET_GPE_RECOVERY_ROOT_OUT)/
	$(hide) $(foreach item,$(recovery_resources_private), \
	  cp -rf $(item) $(TARGET_GPE_RECOVERY_ROOT_OUT)/)
	$(foreach item,$(recovery_root_private), \
	  cp -rf $(item) $(TARGET_GPE_RECOVERY_OUT)/)
	$(hide) $(foreach item,$(gpe_recovery_fstab), \
	  cp -f $(item) $(TARGET_GPE_RECOVERY_ROOT_OUT)/etc/recovery.fstab)
	$(hide) cp $(RECOVERY_INSTALL_OTA_KEYS) $(TARGET_GPE_RECOVERY_ROOT_OUT)/res/keys
	$(hide) cat $(INSTALLED_DEFAULT_PROP_TARGET) $(recovery_build_prop) \
	        > $(TARGET_GPE_RECOVERY_ROOT_OUT)/default.prop
	@echo -e ${CL_YLW}"Modifying default.prop"${CL_RST}
	$(SED_INPLACE) 's/ro.build.date.utc=.*/ro.build.date.utc=0/g' $(TARGET_GPE_RECOVERY_ROOT_OUT)/default.prop
	$(SED_INPLACE) 's/ro.adb.secure=1//g' $(TARGET_GPE_RECOVERY_ROOT_OUT)/default.prop
	@echo -e ${CL_CYN}"----- Made GPE recovery filesystem --------"$(TARGET_GPE_RECOVERY_ROOT_OUT)${CL_RST}
	@touch $(TARGET_GPE_RECOVERY_ROOT_TIMESTAMP)

$(gpe_recovery_uncompressed_ramdisk): $(MINIGZIP) \
	$(TARGET_GPE_RECOVERY_ROOT_TIMESTAMP)
	@echo -e ${CL_CYN}"----- Making GPE uncompressed recovery ramdisk ------"${CL_RST}
	$(MKBOOTFS) $(TARGET_GPE_RECOVERY_ROOT_OUT) > $@

$(gpe_recovery_ramdisk): $(MKBOOTFS) \
	$(gpe_recovery_uncompressed_ramdisk)
	@echo -e ${CL_CYN}"----- Making GPE recovery ramdisk ------"${CL_RST}
	$(MINIGZIP) < $(gpe_recovery_uncompressed_ramdisk) > $@

$(GPE_RECOVERY_RESOURCE_ZIP): $(GPE_RECOVERYIMAGE_TARGET)
	$(hide) mkdir -p $(dir $@)
	$(hide) find $(TARGET_GPE_RECOVERY_ROOT_OUT)/res -type f | sort | zip -0qrj $@ -@

## Overload GPE recoveryimg generation: Same as the original, + --dt arg
$(GPE_RECOVERYIMAGE_TARGET): $(MKBOOTIMG) $(INSTALLED_DTIMAGE_TARGET) \
		$(gpe_recovery_ramdisk) \
		$(recovery_kernel)
	@echo -e ${CL_CYN}"----- Making GPE recovery image ------"${CL_RST}
	$(hide) $(MKBOOTIMG) $(GPE_RECOVERYIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --dt $(INSTALLED_DTIMAGE_TARGET) --output $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)
	@echo -e ${CL_CYN}"Made GPE recovery image: $@"${CL_RST}
