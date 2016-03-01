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

#include "AccelerometerSensor.h"

AccelerometerSensor::AccelerometerSensor()
    : SensorBase("/dev/lis3dh", "accelerometer"),
      mEnabled(0),
      mOriEnabled(false),
      mInputReader(8),
      mPendingEventsMask(0)
{
    mPendingEvents[ACC].version = sizeof(sensors_event_t);
    mPendingEvents[ACC].sensor = ID_A;
    mPendingEvents[ACC].type = SENSOR_TYPE_ACCELEROMETER;
    memset(mPendingEvents[ACC].data, 0, sizeof(mPendingEvents[ACC].data));
    mPendingEventsFlushCount[ACC] = 0;

    mPendingEvents[SO].version = sizeof(sensors_event_t);
    mPendingEvents[SO].sensor = ID_SO;
    mPendingEvents[SO].type = SENSOR_TYPE_SCREEN_ORIENTATION;
    memset(mPendingEvents[SO].data, 0, sizeof(mPendingEvents[SO].data));
    mPendingEventsFlushCount[SO] = 0;

    mPendingEvents[SM].version = sizeof(sensors_event_t);
    mPendingEvents[SM].sensor = ID_SM;
    mPendingEvents[SM].type = SENSOR_TYPE_SIGNIFICANT_MOTION;
    memset(mPendingEvents[SM].data, 0, sizeof(mPendingEvents[SM].data));
    mPendingEventsFlushCount[SM] = 0;
}

AccelerometerSensor::~AccelerometerSensor()
{
    if (mEnabled & MODE_ACCEL)
        enable(ID_A, 0);

    if (mEnabled & MODE_ROTATE)
        enable(ID_SO, 0);

    if (mEnabled & MODE_MOVEMENT)
        enable(ID_SM, 0);

    if (mOriEnabled)
        enable(ID_O, 0);
}

int AccelerometerSensor::enable(int32_t handle, int en)
{
    uint32_t enable = en ? 1 : 0;
    uint32_t flag = mEnabled;
    int mask;

    switch (handle) {
    case ID_A:
        ALOGV("Accelerometer (ACC): enable=%d", en);
        mask = MODE_ACCEL;
        break;
    case ID_SO:
        ALOGV("Accelerometer (SO): enable=%d", en);
        mask = MODE_ROTATE;
        break;
    case ID_SM:
        ALOGV("Accelerometer (SM): enable=%d", en);
        mask = MODE_MOVEMENT;
        break;
    case ID_O:
        ALOGV("Accelerometer (ORI): enable=%d", en);
        mOriEnabled = !!en;
        /* We are not enabling/disabling an actual sensor */
        return 0;
    default:
        ALOGE("Accelerometer: unknown handle %d", handle);
        return -1;
    }

    if ((mEnabled & mask) == enable)
        return 0;

    if (enable)
        flag |= mask;
    else
        flag &= ~mask;

    open_device();
    int ret = ioctl(dev_fd, LIS3DH_IOCTL_SET_ENABLE, &flag);
    close_device();
    if (ret < 0) {
        ALOGE("Accelerometer: could not change sensor state");
        return ret;
    }
    mEnabled = flag;

    return 0;
}

void AccelerometerSensor::writeAkmAccel(float x, float y, float z)
{
    int16_t buf[3];
    int ret;
    int fd;

    buf[0] = (int16_t) (x * CONVERT_ACC);
    buf[1] = (int16_t) (y * CONVERT_ACC);
    buf[2] = (int16_t) (z * CONVERT_ACC);

    fd = open("/sys/devices/virtual/compass/akm8963/accel", O_WRONLY);
    if (fd < 0) {
        ALOGE("Accelerometer: could not open accel file");
        return;
    }

    ret = write(fd, buf, sizeof(buf));
    if (ret < 0)
        ALOGE("Accelerometer: could not write accel data");

    close(fd);
}

int AccelerometerSensor::setDelay(int32_t handle, int64_t ns)
{
    int delay = ns / 1000000;
    int ret;

    switch (handle) {
    case ID_A:
        ALOGV("Accelerometer (ACC): delay=%lld", ns);
        break;
    case ID_SO:
        ALOGV("Accelerometer (SO): ignoring delay=%lld", ns);
        return 0;
    case ID_SM:
        /* Significant motion sensors should not set any delay */
        ALOGV("Accelerometer (SM): ignoring delay=%lld", ns);
        return 0;
    }

    if (delay > ACC_MAX_DELAY_MS)
        delay = ACC_MAX_DELAY_MS;

    switch (handle) {
    case ID_A:
        ALOGV("Accelerometer (ACC): delay=%d", delay);
        break;
    case ID_SO:
        ALOGV("Accelerometer (SO): delay=%d", delay);
        return 0;
    case ID_SM:
        /* Significant motion sensors should not set any delay */
        ALOGV("Accelerometer (SM): delay=%d", delay);
        return 0;
    }

    open_device();
    ret = ioctl(dev_fd, LIS3DH_IOCTL_SET_DELAY, &delay);
    close_device();
    if (ret < 0)
        ALOGE("Accelerometer: could not set delay: %d", ret);

    return ret;
}

int AccelerometerSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    for (int i = 0; i < NUM_SENSORS; i++) {
        if (!count)
            break;
        if (!mPendingEventsFlushCount[i])
            continue;
        sensors_meta_data_event_t flushEvent;
        flushEvent.version = META_DATA_VERSION;
        flushEvent.type = SENSOR_TYPE_META_DATA;
        flushEvent.meta_data.what = META_DATA_FLUSH_COMPLETE;
        flushEvent.meta_data.sensor = mPendingEvents[i].sensor;
        flushEvent.reserved0 = 0;
        flushEvent.timestamp = getTimestamp();
        *data++ = flushEvent;
        mPendingEventsFlushCount[i]--;
        count--;
        numEventReceived++;
    }

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            float value = event->value;
            if (event->code == EVENT_TYPE_ACCEL_X) {
                mPendingEventsMask |= 1 << ACC;
                mPendingEvents[ACC].acceleration.x = value * CONVERT_A_X;
            } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                mPendingEventsMask |= 1 << ACC;
                mPendingEvents[ACC].acceleration.y = value * CONVERT_A_Y;
            } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                mPendingEventsMask |= 1 << ACC;
                mPendingEvents[ACC].acceleration.z = value * CONVERT_A_Z;
            } else {
                ALOGE("Accelerometer: unknown event (type=%d, code=%d)",
                        type, event->code);
            }
        } else if (type == EV_MSC) {
            if (event->code == EVENT_TYPE_SO) {
                mPendingEventsMask |= 1 << SO;
                mPendingEvents[SO].data[0] = event->value;
            } else if (event->code == EVENT_TYPE_SM) {
                mPendingEventsMask |= 1 << SM;
                mPendingEvents[SM].data[0] = 1.0f;
            } else {
                ALOGE("Accelerometer: unknown event (type=%d, code=%d)",
                        type, event->code);
            }
        } else if (type == EV_SYN) {
            for (int i = 0; count && mPendingEventsMask && i < NUM_SENSORS; i++) {
                if (mPendingEventsMask & (1 << i)) {
                    mPendingEventsMask &= ~(1 << i);
                    mPendingEvents[i].timestamp = timevalToNano(event->time);
                    if (mEnabled & indexToMask(i)) {
                        *data++ = mPendingEvents[i];
                        count--;
                        numEventReceived++;

                        if (i == SM)
                            /* Disable sensor automatically */
                            enable(ID_SM, 0);
                    }

                    if (i == ACC && mOriEnabled)
                        writeAkmAccel(mPendingEvents[ACC].acceleration.x,
                                      mPendingEvents[ACC].acceleration.y,
                                      mPendingEvents[ACC].acceleration.z);
                }
            }
        } else {
            ALOGE("Accelerometer: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

int AccelerometerSensor::flush(int handle)
{
    int id;

    switch (handle) {
    case ID_A:
        id = ACC;
        break;
    case ID_SO:
        id = SO;
        break;
    case ID_SM:
        /* One-shot sensors must return -EINVAL */
        return -EINVAL;
    default:
        ALOGE("Accelerometer: unknown handle %d", handle);
        return -EINVAL;
    }

    if (!(mEnabled & indexToMask(id)))
        return -EINVAL;

    mPendingEventsFlushCount[id]++;

    return 0;
}
