/*
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

#ifndef ANDROID_BOSCH_SENSOR_H
#define ANDROID_BOSCH_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class Accelerometer : public SensorBase {
    enum {
        Main               = 0,
        LinearAcceleration = 1,
        SignificantMotion  = 2,
        numSensors,
    };

    /*
     * Cannot turn off Accelerometer when SMD is in use
     * mEnabled is now a bit mask.
     *
     */
    int mEnabled;

    int64_t mDelay;
    uint32_t mFlushed;
    sensors_event_t mPendingEvents[numSensors];
    InputEventCircularReader mInputReader;
    char input_sysfs_path[PATH_MAX];
    int input_sysfs_path_len;

    // Bit Mask to show which sensors have a value
    int mPendingMask;

    // For Linear Acceleration
    const float alpha = 0.8f;
    float mGravity[3];

    // For Significant Motion
    bool isMotionSignificant(timeval const& t);
    int mPolls;
    int mMovements;
    long lastMovementTimeStamp;
    // when to consider it a movement
    const float movement_accel_threshold = 1.0f;
    // Window for these "movements" to occur
    const int  movement_time_window = 1000;
    // Movements required for a significant motion
    const int movements_needed = 3;
    // Polls needed before the algorithm kicks in.
    // High-pass filter is slow to adapt so we have to wait.
    const int linear_acceleration_convergence = 30;

public:
            Accelerometer();
    virtual ~Accelerometer();
    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int setEnable(int32_t handle, int enabled);
    virtual int64_t getDelay(int32_t handle);
    virtual int getEnable(int32_t handle);
    virtual int flush(int handle);
    virtual int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
};

/*****************************************************************************/

#endif  // ANDROID_BOSCH_SENSOR_H
