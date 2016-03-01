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

#include <fcntl.h>
#include <string.h>
#include <cutils/log.h>

#include "GyroscopeSensor.h"

GyroscopeSensor::GyroscopeSensor()
    : SensorBase("/dev/l3g4200d", "gyroscope"),
      mEnabled(0),
      mInputReader(4),
      mPendingEventsFlushCount(0)
{
    mPendingEvents.version = sizeof(sensors_event_t);
    mPendingEvents.sensor = ID_GY;
    mPendingEvents.type = SENSOR_TYPE_GYROSCOPE;
    memset(mPendingEvents.data, 0, sizeof(mPendingEvents.data));
}

GyroscopeSensor::~GyroscopeSensor()
{
    if (mEnabled)
        enable(ID_GY, 0);
}

int GyroscopeSensor::enable(int32_t, int en)
{
    int enable = en ? 1 : 0;

    if (mEnabled == enable)
        return 0;

    open_device();
    int ret = ioctl(dev_fd, L3G4200D_IOCTL_SET_ENABLE, &enable);
    close_device();
    if (ret < 0) {
        ALOGE("Gyroscope: could not change sensor state");
        return ret;
    }
    mEnabled = enable;

    return 0;
}

int GyroscopeSensor::setDelay(int32_t, int64_t ns)
{
    int delay = ns / 1000000;
    int ret;

    ALOGV("Gyroscope: set delay=%lld ns", ns);

    open_device();
    ret = ioctl(dev_fd, L3G4200D_IOCTL_SET_DELAY, &delay);
    close_device();
    if (ret < 0)
        ALOGE("Gyroscope: could not set delay: %d", ret);

    return ret;
}

int GyroscopeSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mPendingEventsFlushCount > 0) {
        sensors_meta_data_event_t flushEvent;
        flushEvent.version = META_DATA_VERSION;
        flushEvent.type = SENSOR_TYPE_META_DATA;
        flushEvent.meta_data.what = META_DATA_FLUSH_COMPLETE;
        flushEvent.meta_data.sensor = mPendingEvents.sensor;
        flushEvent.reserved0 = 0;
        flushEvent.timestamp = getTimestamp();
        *data++ = flushEvent;
        mPendingEventsFlushCount--;
        count--;
        numEventReceived++;
    }

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_REL) {
            float value = event->value;
            if (event->code == EVENT_TYPE_GYRO_X) {
                mPendingEvents.gyro.x = value * CONVERT_G_X;
            } else if (event->code == EVENT_TYPE_GYRO_Y) {
                mPendingEvents.gyro.y = value * CONVERT_G_Y;
            } else if (event->code == EVENT_TYPE_GYRO_Z) {
                mPendingEvents.gyro.z = value * CONVERT_G_Z;
            } else {
                ALOGE("Gyroscope: unknown event (type=%d, code=%d)",
                        type, event->code);
            }
        } else if (type == EV_SYN) {
            mPendingEvents.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvents;
                count--;
                numEventReceived++;
            }
        } else {
            ALOGE("Gyroscope: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

int GyroscopeSensor::flush(int)
{
    if (!mEnabled)
        return -EINVAL;

    mPendingEventsFlushCount++;

    return 0;
}
