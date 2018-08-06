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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>

#include "Accelerometer.h"

#define ACC_UNIT_CONVERSION(value) ((value) * GRAVITY_EARTH / (1024.0f))
#define ACC_INPUT_NAME  "accelerometer_sensor"
#define LOG_TAG "Sensors"

#define DEBUG 1

int write_sys_attribute(const char *path, const char *value, int bytes)
{
	int ifd, amt;

	ifd = open(path, O_WRONLY);
	if (ifd < 0) {
		ALOGE("Write_attr failed to open %s (%s)",path, strerror(errno));
		return -1;
	}

	amt = write(ifd, value, bytes);
	amt = ((amt == -1) ? -errno : 0);
	ALOGE_IF(amt < 0, "Write_int failed to write %s (%s)",
		 path, strerror(errno));
	close(ifd);
	return amt;
}
/*****************************************************************************/

Accelerometer::Accelerometer() :
	SensorBase(NULL, ACC_INPUT_NAME),
		mEnabled(1),
		mDelay(10000),
		//mLayout(1),
    	mInputReader(32),
    	mHasPendingEvent(false)
{
	mPendingEvent.version = sizeof(sensors_event_t);
	mPendingEvent.sensor = ID_A;
	mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
	memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

	if (data_fd >= 0) {
		strcpy(input_sysfs_path, "/sys/class/input/");
		strcat(input_sysfs_path, input_name);
		strcat(input_sysfs_path, "/device/");
		input_sysfs_path_len = strlen(input_sysfs_path);
		ALOGD("Accelerometer: sysfs_path=%s", input_sysfs_path);
	} else {
		input_sysfs_path[0] = '\0';
		input_sysfs_path_len = 0;
	}

	open_device();
}

Accelerometer::~Accelerometer()
{
	if (mEnabled) {
		setEnable(0, 0);
	}

	close_device();
}

bool Accelerometer::hasPendingEvents() const
{
	return mHasPendingEvent;
}

int Accelerometer::setEnable(int32_t handle, int enabled)
{
	int err = 0;
	char buffer[2]={0,0};

	/* handle check */
	if (handle != ID_A) {
		ALOGE("Accelerometer: Invalid handle (%d)", handle);
		return -EINVAL;
	}
	buffer[0]= enabled ? '1':'0';
	ALOGD("Accelerometer: enabled = %s", buffer);
	strcpy(&input_sysfs_path[input_sysfs_path_len], "enable");
	err = write_sys_attribute(input_sysfs_path, buffer, 1);
	if (err != 0) 
		return err;
	
	return err;
}

int Accelerometer::setDelay(int32_t handle, int64_t delay_ns)
{
	int err = 0;
	int32_t us; 
	char buffer[16];
	int bytes;
	/* handle check */
	if (handle != ID_A) {
		ALOGE("Accelerometer: Invalid handle (%d)", handle);
		return -EINVAL;
	}
	if (mDelay != delay_ns) {
		us = (int32_t)(delay_ns / 1000000);

		strcpy(&input_sysfs_path[input_sysfs_path_len], "poll_delay");
		bytes = sprintf(buffer, "%d", (int)us);
		err = write_sys_attribute(input_sysfs_path, buffer, bytes);
		if (err == 0) {
			mDelay = delay_ns;
			ALOGD("Accelerometer: Control set delay %f ms requetsed, ",
			      delay_ns/1000000.0f);
		}
	}
	return err;
}

int64_t Accelerometer::getDelay(int32_t handle)
{
	return (handle == ID_A) ? mDelay : 0;
}

int Accelerometer::getEnable(int32_t handle)
{
	return (handle == ID_A) ? mEnabled : 0;
}

int Accelerometer::readEvents(sensors_event_t * data, int count)
{
	if (count < 1)
		return -EINVAL;

	if (mHasPendingEvent) {
		mHasPendingEvent = false;
		mPendingEvent.timestamp = getTimestamp();
		*data = mPendingEvent;
		return mEnabled ? 1 : 0;
	}

	ssize_t n = mInputReader.fill(data_fd);
	if (n < 0)
		return n;

	int numEventReceived = 0;
	input_event const* event;

	int sensorId = ID_A;
	while (mFlushed) {
		if (mFlushed & (1 << sensorId)) { /* Send flush META_DATA_FLUSH_COMPLETE immediately */
			sensors_event_t sensor_event;
			memset(&sensor_event, 0, sizeof(sensor_event));
			sensor_event.version = META_DATA_VERSION;
			sensor_event.type = SENSOR_TYPE_META_DATA;
			sensor_event.meta_data.sensor = sensorId;
			sensor_event.meta_data.what = 0;
			*data++ = sensor_event;
			count--;
			numEventReceived++;
			mFlushed &= ~(0x01 << sensorId);
			ALOGD_IF(DEBUG, "Accelerometer: %s Flushed sensorId: %d", __func__, sensorId);
		}
		sensorId++;
	}

	while (count && mInputReader.readEvent(&event)) {
		int type = event->type;
		if (type == EV_REL) {
			float value = event->value;
			if (event->code == EVENT_TYPE_ACCEL_X) {
				mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(value);
			} else if (event->code == EVENT_TYPE_ACCEL_Y) {
				mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(value);
			} else if (event->code == EVENT_TYPE_ACCEL_Z) {
				mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(value);
			}
		} else if (type == EV_SYN) {
			mPendingEvent.timestamp = timevalToNano(event->time);
			if (mEnabled) {
				*data++ = mPendingEvent;
				count--;
				numEventReceived++;
			}
		} else {
			ALOGE("Accelerometer: unknown event (type=%d, code=%d)",
			      type, event->code);
		}
		mInputReader.next();
	}

	return numEventReceived;
}

int Accelerometer::batch(int handle, int flags, int64_t period_ns, int64_t timeout) {
	(void)handle;
	(void)flags;
	(void)period_ns;
	(void)timeout;
	return 0;
}

int Accelerometer::flush(int handle)
{
	if (mEnabled){
		mFlushed |= (1 << handle);
		ALOGD("Accelerometer: %s: handle: %d", __func__, handle);
		return 0;
	}
	return -EINVAL;
}
