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

#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"

#include "Accelerometer.h"

#define DEBUG 1

/*****************************************************************************/

#define SENSORS_ACCELERATION     (1<<ID_A)

#define SENSORS_ACCELERATION_HANDLE     0
#define SENSORS_LINEAR_ACCELERATION_HANDLE      1
#define SENSORS_SIGNIFICANT_MOTION_HANDLE       2

/*****************************************************************************/

/* The SENSORS Module */
static const struct sensor_t sSensorList[] = {
	{
		"BMA254",
		"BOSCH",
		1, SENSORS_ACCELERATION_HANDLE, SENSOR_TYPE_ACCELEROMETER,
		(GRAVITY_EARTH * 2.0f), GRAVITY_EARTH/1024.0f,
		0.20f, 1000, 0,0, "", "", 0, 0, {0, 0},
	},
	{
		"Linear Acceleration sensor (Accelerometer)",
		"BOSCH",
		1, SENSORS_LINEAR_ACCELERATION_HANDLE, SENSOR_TYPE_LINEAR_ACCELERATION,
		(GRAVITY_EARTH * 2.0f), GRAVITY_EARTH/1024.0f, 0.20f, 1000, 0, 0,
		"", "", 0, 0, {0, 0},
	},
	{
		"Significant motion sensor",
		"BOSCH",
		1, SENSORS_SIGNIFICANT_MOTION_HANDLE,
		SENSOR_TYPE_SIGNIFICANT_MOTION, 1.0f, 1.0f, 0.20f, -1, 0, 0,
		"", "", 0, SENSOR_FLAG_WAKE_UP | SENSOR_FLAG_ONE_SHOT_MODE, {0, 0},
	},
};


static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
        *list = sSensorList;
        return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
        .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        .common = {
                .tag = HARDWARE_MODULE_TAG,
                .version_major = 1,
                .version_minor = 0,
                .id = SENSORS_HARDWARE_MODULE_ID,
                .name = "Samsung Sensor module",
                .author = "Samsung Electronic Company",
                .methods = &sensors_module_methods,
                .dso  = NULL,
                .reserved = {0},
        },
        .get_sensors_list = sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    sensors_poll_device_1_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int setDelay_sub(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
    int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
    int flush(int handle);

private:
    enum {
        acc          = 0,
        numSensorDrivers,
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle);
    int proxy_enable(int handle, int enabled);
    int proxy_setDelay(int handle, int64_t ns);
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    mSensors[acc] = new Accelerometer();
    mPollFds[acc].fd = mSensors[acc]->getFd();
    mPollFds[acc].events = POLLIN;
    mPollFds[acc].revents = 0;

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::handleToDriver(int handle) {
	switch (handle) {
		case ID_LA:
		case ID_SM:
		case ID_A: return acc;
	}

	return -EINVAL;
}

int sensors_poll_context_t::activate(int handle, int enabled) {
	int drv = handleToDriver(handle);
	int err;

	err = mSensors[drv]->setEnable(handle, enabled);

    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {
	return setDelay_sub(handle, ns);
}

int sensors_poll_context_t::setDelay_sub(int handle, int64_t ns) {
	int drv = handleToDriver(handle);
	int en = mSensors[drv]->getEnable(handle);
	int64_t cur = mSensors[drv]->getDelay(handle);
	int err = 0;

	if (en <= 1) {
		/* no dependencies */
		if (cur != ns) {
			err = mSensors[drv]->setDelay(handle, ns);
		}
	} else {
		/* has dependencies, choose shorter interval */
		if (cur > ns) {
			err = mSensors[drv]->setDelay(handle, ns);
		}
	}
	return err;
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

int sensors_poll_context_t::batch(int handle, int flags, int64_t period_ns, int64_t timeout) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
     return mSensors[index]->batch(handle, flags, period_ns, timeout);
}
 int sensors_poll_context_t::flush(int handle) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
     return mSensors[index]->flush(handle);
}

/*****************************************************************************/

static int device__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int device__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    ALOGD_IF(DEBUG, "+device__activate: handle=%d enabled=%d", handle, enabled);
    return ctx->activate(handle, enabled);
}

static int device__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    ALOGD_IF(DEBUG, "device__setDelay: handle:%d, ns:%lld", handle, ns);
    return ctx->setDelay(handle, ns);
}

static int device__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

static int device__batch(struct sensors_poll_device_1 *dev, int handle,
        int flags, int64_t period_ns, int64_t timeout) {
    sensors_poll_context_t* ctx = (sensors_poll_context_t*) dev;
    ALOGD_IF(DEBUG, "device__batch: handle:%d , flags:%d period_ns:%lld timeout:%lld",
             handle, flags, period_ns, timeout);
    return ctx->batch(handle, flags, period_ns, timeout);
}
 static int device__flush(struct sensors_poll_device_1 *dev, int handle) {
    sensors_poll_context_t* ctx = (sensors_poll_context_t*) dev;
    ALOGD_IF(DEBUG, "device__flush: handle:%d", handle);
    return ctx->flush(handle);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
        int status = -EINVAL;
        sensors_poll_context_t *dev = new sensors_poll_context_t();

        memset(&dev->device, 0, sizeof(sensors_poll_device_1_t));

        dev->device.common.tag      = HARDWARE_DEVICE_TAG;
        dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_3;
        dev->device.common.module   = const_cast<hw_module_t*>(module);
        dev->device.common.close    = device__close;
        dev->device.activate        = device__activate;
        dev->device.setDelay        = device__setDelay;
        dev->device.poll            = device__poll;
        dev->device.batch           = device__batch;
        dev->device.flush           = device__flush;

        *device = &dev->device.common;
        status = 0;

        return status;
}

