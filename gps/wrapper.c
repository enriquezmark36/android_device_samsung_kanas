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

static const AGpsRilInterface *real_agps_ril_interface = NULL;
static AGpsRilInterface my_agps_ril_interface;

// From libdl.so
extern void android_set_application_target_sdk_version(int target);

static int wrapper_data_conn_open_with_apn_ip_type(const char *apn, ApnIpType apnIpType) {
	ALOGW("[%s]: We shouldn't be here; we're using AGpsInterface_v1.", __func__);

	if (!real_agps_interface) {
		ALOGE("[%s]: Also, real_agps_callbacks is still NULL", __func__);
		return -1;
	}

	ALOGD("[%s]: Redirecting call to data_conn_open()", __func__);
	(void) apnIpType;
	return real_agps_interface->data_conn_open(apn);
}

static void wrapper_set_ref_location(const AGpsRefLocation *inputType, size_t sz_struct) {

	typedef struct {
		AGpsRefLocationType type;
		uint16_t mcc;
		uint16_t mnc;
		uint16_t lac;
		uint16_t psc;
		uint32_t cid;
		uint16_t tac;
		uint16_t pcid;
	} SamsungAGpsRefLocationCellID;

	typedef struct {
		AGpsRefLocationType type;
		union {
			SamsungAGpsRefLocationCellID cellID;
			AGpsRefLocationMac mac;
		} u;
	} SamsungAGpsRefLocation;

	if ((sz_struct == sizeof(AGpsRefLocationCellID)) &&
	    (sz_struct != sizeof(SamsungAGpsRefLocationCellID))) {

		SamsungAGpsRefLocation outputType;
		outputType.type = inputType->type;
		outputType.u.cellID = (SamsungAGpsRefLocationCellID) {
			.type = inputType->u.cellID.type,
			.mcc = inputType->u.cellID.mcc,
			.mnc = inputType->u.cellID.mnc,
			.lac = inputType->u.cellID.lac,
			.psc = (uint16_t) -1,
			.cid = inputType->u.cellID.cid,
			.tac = inputType->u.cellID.tac,
			.pcid = inputType->u.cellID.pcid,
		};

		real_agps_ril_interface->
		     set_ref_location((AGpsRefLocation *)&outputType, sz_struct);

		return;
	}

	real_agps_ril_interface->set_ref_location(inputType, sz_struct);
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
	} else if (strcmp(AGPS_RIL_INTERFACE, name) == 0) {
		real_agps_ril_interface = (AGpsRilInterface *) ret;
		memcpy(&my_agps_ril_interface, real_agps_ril_interface, sizeof(AGpsRilInterface));

		my_agps_ril_interface.set_ref_location = wrapper_set_ref_location;
		return &my_agps_ril_interface;
	}

	return ret;
}

void wrapper_gps_sv_status_callback (GpsSvStatus *sv_info) {
	typedef struct {
		size_t          size;
		int     prn;
		float   snr;
		float   elevation;
		float   azimuth;
		int used; // WTH Samsung, used where ?!
	} SamsungGpsSvInfo;

	typedef struct {
		size_t size;
		int num_svs;
		SamsungGpsSvInfo sv_list[GPS_MAX_SVS];
		uint32_t ephemeris_mask;
		uint32_t almanac_mask;
		uint32_t used_in_fix_mask;
	} SamsungGpsSvStatus;

	/*
	 * Since the newer GpsSvStatus is _smaller_ than of the newer one
	 * We have to allocate memory to prevent overruns/overflows
	 */
	SamsungGpsSvStatus *inputType = (SamsungGpsSvStatus *)sv_info;
	GpsSvStatus outputType; // stack allocation go brrrr

	if (!sv_info) {
		real_gps_callbacks->sv_status_cb(sv_info);
		return;
	}

	// Verify if we really need to do a conversion
	if (sizeof(GpsSvStatus) == sv_info->size) {
		real_gps_callbacks->sv_status_cb(sv_info);
		return;
	}

	if (sizeof(SamsungGpsSvStatus) != sv_info->size) {
		ALOGE("[%s] Unexpected sv_info, neither of size %u or %u, instead its %u",
		      __func__, sizeof (GpsSvStatus), sizeof(SamsungGpsSvStatus), sv_info->size);
		real_gps_callbacks->sv_status_cb(sv_info); // live dangerously
		return;
	}

	// Upon this stage, we need to do a conversion
	// First copy the first two items: size_t, int
	outputType.size = sizeof(GpsSvStatus);
	outputType.num_svs = inputType->num_svs;
	outputType.ephemeris_mask = inputType->ephemeris_mask;
	outputType.almanac_mask = inputType->almanac_mask;
	outputType.used_in_fix_mask = inputType->used_in_fix_mask;

	// Second, copy the masks.
	memcpy(&outputType.ephemeris_mask, &inputType->ephemeris_mask,
	       sizeof(uint32_t)*3);

	// Do a deep copy of the sv_list
	for (int i = sv_info->num_svs; i-- > 0; )
		memcpy(&outputType.sv_list[i], &inputType->sv_list[i],
		       sizeof(GpsSvInfo));

	// Conversion complete, run the original callback...
	real_gps_callbacks->sv_status_cb((GpsSvStatus*) &outputType);
}

static void wrapper_gps_location_callback (GpsLocation *location) {
	/*
	 * Account the GPS week rollover last August, 2019.
	 * Compensate for the rollover in milliseconds time.
	 *
	 * NOTE: A total number of GPS weeks last exactly 1024 weeks but this
	 * device for some reason always return a location timestamped
	 * earlier than the present time.
	 * NOTE: Ignore the last note, make it so that the GPS Time is about 18
	 * seconds ahead of UTC.
	 */
	location->timestamp += 619315200000;
	real_gps_callbacks->location_cb(location);
}

static int wrapper_init(GpsCallbacks* callbacks) {
	real_gps_callbacks = callbacks;
	memcpy(&my_gps_callbacks, callbacks, sizeof(GpsCallbacks_v1));
	my_gps_callbacks.size = sizeof(GpsCallbacks_v1);
	ALOGI("[%s] Using GpsCallbacks_v1 struct,"
		" size changes from %d B => %d B",
		__func__, sizeof(GpsCallbacks), my_gps_callbacks.size);

	my_gps_callbacks.sv_status_cb = wrapper_gps_sv_status_callback;
	my_gps_callbacks.location_cb = wrapper_gps_location_callback;
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

	ALOGI("[%s] Initializing wrapper for Marvell's GPS-HAL", __func__);

	// Marvell's GPS blob uses text relocations,
	// Set this sdk version to 19 (KitKat) so we don't get smited by
	// bionic libc.
	android_set_application_target_sdk_version(19);

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
