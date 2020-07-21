/*
 * Copyright (c) 2015 The CyanogenMod Project
 * Copyright (c) 2019 Mark Enriquez <enriquezmark36@gmail.com>
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

#define LOG_TAG "libgpswrapper"
// #define LOG_NDEBUG 0

#define ORIGINAL_HAL_PATH "/vendor/lib/hw/gps.vendor.sc8830.so"

#include <errno.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>

#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/gps_internal.h>

static struct gps_device_t *real_gps_device;

static const GpsInterface *real_gps_interface;
static GpsInterface my_gps_interface;

static GpsCallbacks *real_gps_callbacks;
static GpsCallbacks_v1 my_gps_callbacks;

static const AGpsInterface *real_agps_interface = NULL;
static AGpsInterface my_agps_interface;

static const AGpsCallbacks *real_agps_callbacks;
static AGpsCallbacks my_agps_callbacks;

static int wrapper_data_conn_open_with_apn_ip_type(const char *apn, ApnIpType apnIpType) {
	ALOGW("[%s]: We shouldn't be here; we're using AGpsInterface_v1.", __func__);

	if (!real_agps_interface) {
		ALOGE("[%s]: Also, real_agps_callbacks is still NULL", __func__);
		return -1;
	}

	ALOGD("[%s]: Redirecting call to data_conn_open()", __func__);
	return real_agps_interface->data_conn_open(apn);
}
static const void* wrapper_get_extension(const char* name) {
	const void *ret;

	if (!(ret = real_gps_interface->get_extension(name)))
		return ret;

	if (strcmp(AGPS_INTERFACE, name) == 0) {
		real_agps_interface = (AGpsInterface *) ret;
		memcpy(&my_agps_interface, real_agps_interface, sizeof(AGpsInterface_v1));

		my_agps_interface.size = sizeof(AGpsInterface_v1);
		my_agps_interface.data_conn_open_with_apn_ip_type = wrapper_data_conn_open_with_apn_ip_type;

		ALOGI("[%s] %s: Using AGpsInterface_v1, size changes from %d to %d Bytes",
		__func__, name, real_agps_interface->size, sizeof(AGpsInterface_v1));

		return &my_agps_interface;
	}

	return ret;
}

static int wrapper_init(GpsCallbacks* callbacks) {
	real_gps_callbacks = callbacks;
	memcpy(&my_gps_callbacks, callbacks, sizeof(GpsCallbacks_v1));
	my_gps_callbacks.size = sizeof(GpsCallbacks_v1);
	ALOGI("[%s] Using GpsCallbacks_v1 struct,"
		" size changes from %d B => %d B",
		__func__, sizeof(GpsCallbacks), my_gps_callbacks.size);

	return real_gps_interface->init((GpsCallbacks* )(&my_gps_callbacks));
}

static const GpsInterface* wrapper_get_gps_interface(struct gps_device_t* dev)
{
	real_gps_interface = real_gps_device->get_gps_interface(dev);

	memcpy(&my_gps_interface, real_gps_interface, sizeof(GpsInterface));
	my_gps_interface.init = wrapper_init;
	my_gps_interface.get_extension = wrapper_get_extension;

	return &my_gps_interface;
}

static int wrapper_open(__attribute__((unused)) const hw_module_t* module,
			__attribute__((unused)) const char* name,
			hw_device_t** device)
{
	static void *dso_handle = NULL;
	struct hw_module_t *hmi;
	struct gps_device_t *my_gps_device;
	int ret = -EINVAL;
	GpsInterface* sGpsInterface = NULL;

	ALOGI("[%s] Initializing wrapper for Marvell's GPS-HAL", __func__);

	my_gps_device = calloc(1, sizeof(struct gps_device_t));
	if (!my_gps_device) {
		ALOGE("couldn't malloc");
		return -EINVAL;
	}

	ALOGD("[%s] Loading Marvell's GPS-HAL from %s", __func__, ORIGINAL_HAL_PATH);
	dso_handle = dlopen(ORIGINAL_HAL_PATH, RTLD_NOW);
	if (dso_handle == NULL) {
		char const *err_str = dlerror();
		ALOGE("%s", err_str ? err_str : "unknown");
		goto dso_fail;
	}

	const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
	hmi = (struct hw_module_t *)dlsym(dso_handle, sym);
	if (hmi == NULL) {
		ALOGE("couldn't find symbol %s", sym);
		goto dso_cleanup;
	}

	hmi->dso = dso_handle;

	ret = hmi->methods->open(hmi, GPS_HARDWARE_MODULE_ID, (hw_device_t**)&real_gps_device);
	if (ret != 0) {
		ALOGE("HAL's open() failed");
		goto dso_cleanup;
	}

	memcpy(my_gps_device, real_gps_device, sizeof(struct gps_device_t));

	my_gps_device->get_gps_interface = wrapper_get_gps_interface;

	*device = (hw_device_t*)my_gps_device;

	ALOGI("[%s] Marvell's GPS-HAL is wrapped successfuly", __func__);
	return ret;

dso_cleanup:
	dlclose(dso_handle);
	dso_handle = NULL;
dso_fail:
	ALOGE("[%s] ret: %d", __func__, ret);
	ALOGE("[%s] Unsuccessful wrap", __func__);
	return ret;
}

static struct hw_module_methods_t wrapper_module_methods = {
	.open = wrapper_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = GPS_HARDWARE_MODULE_ID,
	.name = "Marvell GPS-HAL wrapper",
	.author = "Mark Enriquez (originally by Michael Gernoth)",
	.methods = &wrapper_module_methods,
};
