# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked, and stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

# Display Rotation sensor and Significant Motion sensor are mutually
# exclusive. If the following is set to true, the HAL will expose both
# the sensors, possibly leading to conflicts.
DISPLAY_ROTATION_SENSOR_ENABLED := false

LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DLOG_TAG=\"msm8226_Sensors\"

LOCAL_SRC_FILES := \
    sensors.cpp \
    SensorBase.cpp \
    LightProxSensor.cpp \
    AccelerometerSensor.cpp \
    CompOriSensor.cpp \
    InputEventReader.cpp

ifneq ($(filter peregrine, $(TARGET_DEVICE)),)
    LOCAL_CFLAGS += -DWITH_GYROSCOPE
    LOCAL_SRC_FILES += GyroscopeSensor.cpp
endif

ifeq ($(DISPLAY_ROTATION_SENSOR_ENABLED), true)
    LOCAL_CFLAGS += -DWITH_DISPLAY_ROTATION_SENSOR
endif

LOCAL_SHARED_LIBRARIES := liblog libcutils libdl

include $(BUILD_SHARED_LIBRARY)
