/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * The workaround method of supporting earlysuspend was
 * basically just triggering the lateresume hooks when needed.
 * It's implemented in the Power hal implementation and works good
 * enough.
 *
 * This shim will reimplement the workaround by intercepting the
 * autosuspend_disable(), triggering the lateresume hooks when possible.
 *
 * When I see at least 3 users of libsuspend, maybe it's time to
 * resurrect the earlysuspend patch in libsuspend. The first user is
 * the systemserver and the second is the charger/lpm binary.
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

#include <dlfcn.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>

#define LOG_NDEBUG 0
#define LOG_TAG "liblpm_SHIM"
#include <cutils/log.h>

static int (*real_autosuspend_disable)() =
    (int (*)()) dlsym(RTLD_NEXT, "autosuspend_disable");
static int s_has_earlysuspend = -1;

static int trigger_lateresume() {
	static const char *PATH = "/sys/power/state";
	int fd, len;

	fd = open(PATH, O_WRONLY);
	if (fd < 0) {
		ALOGE("%s: cannot open %s: %s", __func__, PATH, strerror(errno));
		return -1;
	}

	len = write(fd, "on", 2); // they don't like the trailing null byte
	if (len < 0) {
		ALOGE("%s: failed to write on %s: %s", __func__, PATH, strerror(errno));
		return -1;
	}

	return 0;
}

static int wait_lateresume() {
	static const char *PATH = "/sys/power/wait_for_fb_wake";
	int fd, len;
	char tmp[6];

	fd = open(PATH, O_RDONLY);
	if (fd < 0) {
		ALOGE("%s: cannot open %s: %s", __func__, PATH, strerror(errno));
		return -1;
	}

	len = read(fd, tmp, 6);
	if (len < 0) {
		ALOGE("%s: failed to read from %s: %s", __func__, PATH, strerror(errno));
		return -1;
	}

	return 0;
}


extern "C" int autosuspend_disable(void)
{
	int ret;
	ALOGV("%s: Enter", __func__);

	if (__builtin_expect(!real_autosuspend_disable, 0)) {
		real_autosuspend_disable = (int (*)()) dlsym(RTLD_NEXT, "autosuspend_disable");
		if (!real_autosuspend_disable) {
			ALOGE("%s: Exit, dlsym failed", __func__);
			return 0;
		}
	}

	ALOGV("%s: calling autosuspend_disable()", __func__);
	ret = real_autosuspend_disable();

	if (__builtin_expect(s_has_earlysuspend < 0, 0)) {
		if (!access("/sys/power/wait_for_fb_wake", R_OK)) {
			ALOGV("%s: earlysuspend detected", __func__);
			s_has_earlysuspend = 1;
		} else {
			ALOGV("%s: earlysuspend NOT detected", __func__);
			s_has_earlysuspend = 0;
		}
	}

	if (__builtin_expect((s_has_earlysuspend == 1), 1) && !trigger_lateresume()) {
			ALOGV("%s: Waiting...()", __func__);
			wait_lateresume();
	}

	ALOGV("%s: Exit", __func__);
	return ret;
}

