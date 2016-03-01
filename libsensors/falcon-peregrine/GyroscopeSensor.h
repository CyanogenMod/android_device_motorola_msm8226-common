/*
 * Copyright (C) 2016 The CyanogenMod Project
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_GYROSCOPE_SENSOR_H
#define ANDROID_GYROSCOPE_SENSOR_H

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"


#define L3G4200D_IOCTL_BASE		77
#define L3G4200D_IOCTL_SET_DELAY	_IOW(L3G4200D_IOCTL_BASE, 0, int)
#define L3G4200D_IOCTL_GET_DELAY	_IOR(L3G4200D_IOCTL_BASE, 1, int)
#define L3G4200D_IOCTL_SET_ENABLE	_IOW(L3G4200D_IOCTL_BASE, 2, int)
#define L3G4200D_IOCTL_GET_ENABLE	_IOR(L3G4200D_IOCTL_BASE, 3, int)


class GyroscopeSensor : public SensorBase {

private:
    int mEnabled;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents;
    int mPendingEventsFlushCount;

public:
            GyroscopeSensor();
    virtual ~GyroscopeSensor();
    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int flush(int32_t handle);
};

#endif  // ANDROID_GYROSCOPE_SENSOR_H
