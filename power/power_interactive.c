/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2014 The CyanogenMod Project
 * Copyright (C) 2014-2015 Andreas Schneider <asn@cryptomilk.org>
 * Copyright (C) 2014-2017 Christopher N. Hesse <raymanfx@gmail.com>
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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>

#define LOG_TAG "SamsungPowerHAL"
// #define LOG_NDEBUG 0
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define CPU_SYSFS_PATH "/sys/devices/system/cpu/cpu0"

#define BOOST_PATH        "/boost"
#define BOOSTPULSE_PATH   "/boostpulse"

#define IO_IS_BUSY_PATH   "/io_is_busy"
#define HISPEED_FREQ_PATH "/hispeed_freq"

#define MAX_FREQ_PATH     "/cpufreq/scaling_max_freq"
#define MIN_FREQ_PATH     "/cpufreq/scaling_min_freq"
#define CPU_MAX_FREQ_PATH "/cpufreq/cpuinfo_max_freq"
#define CPU_MIN_FREQ_PATH "/cpufreq/cpuinfo_min_freq"

#define GOVNOR_NAME_PATH  "/cpufreq/scaling_governor"

#define LCD_FPS_PATH      "/sys/devices/virtual/dispc/dispc/fps"

#define GPU_CORE_SCALER   "/sys/module/mali/parameters/mali_core_scaling"
#define GPU_CORE_NUM      "/sys/module/mali/parameters/mali_max_pp_cores_group_1"
#define GPU_CORE_TARUTIL  "/sys/module/mali/parameters/mali_core_tarutil"
#define GPU_CORE_MINLOAD  "/sys/module/mali/parameters/mali_core_minload"

// Sustainable frequency
#define CPU_SUST_FREQ     "1000000"

#define CPU_LIMIT_MAX     "/sys/power/cpufreq_max_limit"
#define CPU_LIMIT_MIN     "/sys/power/cpufreq_min_limit"

#define PARAM_MAXLEN      10

#define ARRAY_SIZE(a)     sizeof(a) / sizeof(a[0])

struct samsung_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boost_fd;
    int boostpulse_fd;
    int cpufreq_limit;
    int gpu_core_scale;
    char hispeed_freq[PARAM_MAXLEN];
    char max_freq[PARAM_MAXLEN];
    char min_freq[PARAM_MAXLEN];
};

enum power_profile_e {
    PROFILE_POWER_SAVE = 0,
    PROFILE_BALANCED,
    PROFILE_HIGH_PERFORMANCE,
    PROFILE_BIAS_POWER_SAVE,
    PROFILE_BIAS_PERFORMANCE,
    PROFILE_MAX
};

enum cpufreq_limit_e {
    CPUFREQ_MIN_LIMIT = 0,
    CPUFREQ_MAX_LIMIT,
    CPUFREQ_MIN_RESTORE,
    CPUFREQ_MAX_RESTORE,
};
static enum power_profile_e current_power_profile = PROFILE_MAX;
static char cpu_governor_path[255] = "/sys/devices/system/cpu/cpufreq/";

/**********************************************************
 *** HELPER FUNCTIONS
 **********************************************************/

static int sysfs_read(char *path, char *s, int num_bytes)
{
    char errno_str[64];
    int len;
    int ret = 0;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        strerror_r(errno, errno_str, sizeof(errno_str));
        ALOGE("Error opening %s: %s", path, errno_str);

        return -1;
    }

    len = read(fd, s, num_bytes - 1);
    if (len < 0) {
        strerror_r(errno, errno_str, sizeof(errno_str));
        ALOGE("Error reading from %s: %s", path, errno_str);

        ret = -1;
    } else {
        // do not store newlines, but terminate the string instead
        if (s[len-1] == '\n') {
            s[len-1] = '\0';
        } else {
            s[len] = '\0';
        }
    }

    close(fd);

    return ret;
}

static void sysfs_write(const char *path, char *s)
{
    char errno_str[64];
    int len;
    int fd;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        strerror_r(errno, errno_str, sizeof(errno_str));
        ALOGE("Error opening %s: %s", path, errno_str);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, errno_str, sizeof(errno_str));
        ALOGE("Error writing to %s: %s", path, errno_str);
    }

    close(fd);
}

static void cpu_sysfs_read(const char *param, char s[PARAM_MAXLEN])
{
    char path[PATH_MAX];

    sprintf(path, "%s%s", CPU_SYSFS_PATH, param);
    sysfs_read(path, s, PARAM_MAXLEN);
}

static void cpu_sysfs_write(const char *param, char s[PARAM_MAXLEN])
{
    char path[PATH_MAX];

    sprintf(path, "%s%s", CPU_SYSFS_PATH, param);
    sysfs_write(path, s);
}

static void cpu_governor_read(const char *param, char s[PARAM_MAXLEN])
{
    char path[PATH_MAX];

    sprintf(path, "%s%s", cpu_governor_path, param);
    sysfs_read(path, s, PARAM_MAXLEN);
}

static void cpu_governor_write(const char *param, char s[PARAM_MAXLEN])
{
    char path[PATH_MAX];

    sprintf(path, "%s%s", cpu_governor_path, param);
    sysfs_write(path, s);
}

static void send_boost(int boost_fd, int32_t duration_us)
{
    int len;

    if (boost_fd < 0)
        return;

    len = write(boost_fd, "1", 1);
    if (len < 0) {
        ALOGE("Error writing to %s%s: %s", cpu_governor_path, BOOST_PATH, strerror(errno));
        return;
    }

    usleep(duration_us);
    len = write(boost_fd, "0", 1);
}

static void send_boostpulse(int boostpulse_fd)
{
    int len;

    if (boostpulse_fd < 0) {
        return;
    }

    len = write(boostpulse_fd, "1", 1);
    if (len < 0) {
        ALOGE("Error writing to %s%s: %s", cpu_governor_path, BOOSTPULSE_PATH,
                strerror(errno));
    }
}

/**********************************************************
 *** POWER FUNCTIONS
 **********************************************************/

static void set_cpufreq_limit(struct samsung_power_module *samsung_pwr,
                              enum cpufreq_limit_e type,
                              char *freq)
{
    (void) samsung_pwr;

    switch (type) {
        case CPUFREQ_MIN_RESTORE:
            sysfs_write(CPU_LIMIT_MIN, "-1");
            cpu_sysfs_write(MIN_FREQ_PATH, samsung_pwr->min_freq);
            break;
        case CPUFREQ_MIN_LIMIT:
            sysfs_write(CPU_LIMIT_MIN, freq);
            break;

        case CPUFREQ_MAX_RESTORE:
            sysfs_write(CPU_LIMIT_MAX, "-1");
            cpu_sysfs_write(MAX_FREQ_PATH, samsung_pwr->max_freq);
            break;
        case CPUFREQ_MAX_LIMIT:
            sysfs_write(CPU_LIMIT_MAX, freq);
        default:
            break;
    }
}

static void set_cpufreq_scaling_limit(struct samsung_power_module *samsung_pwr,
                                      enum cpufreq_limit_e type,
                                      char *freq)
{
    switch (type) {
        case CPUFREQ_MIN_LIMIT:
            cpu_sysfs_write(MAX_FREQ_PATH, freq);
            break;
        case CPUFREQ_MAX_LIMIT:
            cpu_sysfs_write(MAX_FREQ_PATH, freq);
            break;
        case CPUFREQ_MIN_RESTORE:
            cpu_sysfs_write(MIN_FREQ_PATH, samsung_pwr->min_freq);
            break;
        case CPUFREQ_MAX_RESTORE:
            cpu_sysfs_write(MAX_FREQ_PATH, samsung_pwr->max_freq);
            break;
        default:
            break;
    }
}

void (*limit_cpufreq)(struct samsung_power_module *,
                      enum cpufreq_limit_e,
                      char *) = set_cpufreq_scaling_limit;

static void set_lcd_fps(int profile)
{
    switch (profile) {
        case PROFILE_POWER_SAVE:
        case PROFILE_BIAS_POWER_SAVE:
            // Reduce the refresh rate of the lcd
            ALOGV("%s: Reduce refresh rate to 45fps", __func__);
            sysfs_write(LCD_FPS_PATH, "45");
            break;
        case PROFILE_BALANCED:
        case PROFILE_BIAS_PERFORMANCE:
        case PROFILE_HIGH_PERFORMANCE:
            // Restore back to 60fps
            ALOGV("%s: Restore refresh rate back to 60fps", __func__);
            sysfs_write(LCD_FPS_PATH, "60");
            break;
    }
        return;
}

static void set_gpu_core_scaler(struct samsung_power_module *samsung_pwr,
                                int profile)
{
    char *target_util, *reserved_idle_power, *scaling_enabled;
    if (!samsung_pwr->gpu_core_scale)
        return;

    switch (profile) {
        case PROFILE_POWER_SAVE:
                target_util = "256";
                reserved_idle_power = "230";
                scaling_enabled = "1";
            break;
        case PROFILE_BIAS_POWER_SAVE:
                target_util = "230";
                reserved_idle_power = "160";
                scaling_enabled = "1";
            break;
        case PROFILE_BALANCED:
                target_util = "200";
                reserved_idle_power = "190";
                scaling_enabled = "1";
            break;
        case PROFILE_BIAS_PERFORMANCE:
                target_util = "154";
                reserved_idle_power = "157";
                scaling_enabled = "0";
            break;
        case PROFILE_HIGH_PERFORMANCE:
                target_util = "1";
                reserved_idle_power = "512";
                scaling_enabled = "1";
            break;
    }

    sysfs_write(GPU_CORE_TARUTIL, target_util);
    sysfs_write(GPU_CORE_MINLOAD, reserved_idle_power);
    sysfs_write(GPU_CORE_SCALER, scaling_enabled);
    ALOGV("%s: Configuring GPU core scaling with these parameters:\n"
          "GPU core scaling enabled: %s\n"
          "Target GPU utilization: %s\n"
          "Reserved GPU capacity: %s\n",
        __func__, scaling_enabled[0] == '1' ? "Yes" : "No", target_util, reserved_idle_power);
    return;
}

static void set_power_profile(struct samsung_power_module *samsung_pwr,
                              int profile)
{
    int rc;

    if (profile < 0 || profile >= PROFILE_MAX)
        return;

    if (current_power_profile == (enum power_profile_e) profile)
        return;

    ALOGV("%s: profile=%d", __func__, profile);

    switch (profile) {
        case PROFILE_POWER_SAVE:
            limit_cpufreq(samsung_pwr, CPUFREQ_MIN_RESTORE, 0);
            limit_cpufreq(samsung_pwr, CPUFREQ_MAX_LIMIT, samsung_pwr->min_freq);
            ALOGV("%s: set powersave mode", __func__);
            break;
        case PROFILE_BIAS_POWER_SAVE:
            limit_cpufreq(samsung_pwr, CPUFREQ_MIN_RESTORE, 0);
            limit_cpufreq(samsung_pwr, CPUFREQ_MAX_LIMIT, CPU_SUST_FREQ);
            ALOGV("%s: set efficiency mode", __func__);
            break;
        case PROFILE_BALANCED:
            // Restore normal max freq
            limit_cpufreq(samsung_pwr, CPUFREQ_MIN_RESTORE, 0);
            limit_cpufreq(samsung_pwr, CPUFREQ_MAX_RESTORE, 0);
            ALOGV("%s: set balanced mode", __func__);
            break;
        case PROFILE_BIAS_PERFORMANCE:
            ALOGV("%s: set quick mode [fake] ", __func__);
        case PROFILE_HIGH_PERFORMANCE:
            // Restore normal max freq
            limit_cpufreq(samsung_pwr, CPUFREQ_MAX_RESTORE, 0);
            limit_cpufreq(samsung_pwr, CPUFREQ_MIN_LIMIT, samsung_pwr->max_freq);
            ALOGV("%s: set performance mode", __func__);
            break;
        default:
            ALOGW("%s: Unknown power profile: %d", __func__, profile);
            return;
    }

    set_lcd_fps(profile);
    set_gpu_core_scaler(samsung_pwr, profile);
    current_power_profile = profile;
}

/**********************************************************
 *** INIT FUNCTIONS
 **********************************************************/

static void init_cpufreqs(struct samsung_power_module *samsung_pwr)
{
    char governor_name[255];
    cpu_sysfs_read(GOVNOR_NAME_PATH, governor_name);
    strcat(cpu_governor_path, governor_name);

    cpu_governor_read(HISPEED_FREQ_PATH, samsung_pwr->hispeed_freq);
    cpu_sysfs_read(CPU_MAX_FREQ_PATH, samsung_pwr->max_freq);
    cpu_sysfs_read(CPU_MIN_FREQ_PATH, samsung_pwr->min_freq);
}

static int init_cpu_limit()
{
    char buf[2];
    if (sysfs_read(CPU_LIMIT_MAX, buf, 1) < 0 ||
        sysfs_read(CPU_LIMIT_MIN, buf, 1) < 0)
        return -1;

    limit_cpufreq = set_cpufreq_limit;
    return 0;
}

static void gpu_ctrl_open(struct samsung_power_module *samsung_pwr)
{
    char gpu_param[PARAM_MAXLEN];
    int cores;
    int ret;

    // Check if we should enable gpu scaling on >= 2 cores
    // NOTE: This code block is written for MALI 400 MP
    // (where core count can be only up to four)
    samsung_pwr->gpu_core_scale = 0;
    ret = sysfs_read(GPU_CORE_NUM, gpu_param, PARAM_MAXLEN);
    cores = gpu_param[0] - '0'; // total number of gpu cores
    if (!ret && cores >= 2 && cores <= 4) {
        // gpu core scaling switch
        ret = sysfs_read(GPU_CORE_SCALER, gpu_param, PARAM_MAXLEN);

        if (!ret)
            samsung_pwr->gpu_core_scale = 1;
    }
}

static void boost_open(struct samsung_power_module *samsung_pwr)
{
    char path[PATH_MAX];

    // the boost node is only valid for the LITTLE cluster
    sprintf(path, "%s%s", cpu_governor_path, BOOST_PATH);

    samsung_pwr->boost_fd = open(path, O_WRONLY);
    if (samsung_pwr->boost_fd < 0) {
        ALOGE("Error opening %s: %s\n", path, strerror(errno));
    }
}

static void boostpulse_open(struct samsung_power_module *samsung_pwr)
{
    char path[PATH_MAX];

    // the boostpulse node is only valid for the LITTLE cluster
    sprintf(path, "%s%s", cpu_governor_path, BOOSTPULSE_PATH);

    samsung_pwr->boostpulse_fd = open(path, O_WRONLY);
    if (samsung_pwr->boostpulse_fd < 0) {
        ALOGE("Error opening %s: %s\n", path, strerror(errno));
    }
}

static void samsung_power_init(struct power_module *module)
{
    struct samsung_power_module *samsung_pwr = (struct samsung_power_module *) module;
    int has_cpufreq_limit;

    init_cpufreqs(samsung_pwr);
    gpu_ctrl_open(samsung_pwr);


    has_cpufreq_limit = init_cpu_limit();


    // keep governor boost fds opened
    boost_open(samsung_pwr);
    boostpulse_open(samsung_pwr);

    ALOGI("Initialized settings:");
    ALOGI("max_freq: %s", samsung_pwr->max_freq);
    ALOGI("min_freq: %s", samsung_pwr->min_freq);

    ALOGI("Cpufreq governor path: %s", cpu_governor_path);
    ALOGI("hispeed_freq: %s", samsung_pwr->hispeed_freq);
    ALOGI("CPU hint boost: %s",
          samsung_pwr->boost_fd > 0 ? "Enabled" : "Disabled");
    ALOGI("Interaction hint boost: %s",
        samsung_pwr->boostpulse_fd > 0 ? "Enabled" : "Disabled");

    ALOGI("Using cpufreq_limit interface: %s",
        has_cpufreq_limit == 0 ? "Enabled" : "Disabled");
    ALOGI("Scale GPU cores: %s",
        samsung_pwr->gpu_core_scale? "Yes" : "Only soft frequency scaling");
}

/**********************************************************
 *** API FUNCTIONS
 ***
 *** Refer to power.h for documentation.
 **********************************************************/

static void samsung_power_set_interactive(struct power_module *module, int on)
{
    char ON[PARAM_MAXLEN]  = {"1"};
    char OFF[PARAM_MAXLEN] = {"0"};

    cpu_governor_write(IO_IS_BUSY_PATH, on ? ON : OFF);
    (void)module;

    ALOGV("power_set_interactive: %d done", on);
}

static void samsung_power_hint(struct power_module *module,
                                  power_hint_t hint,
                                  void *data)
{
    struct samsung_power_module *samsung_pwr = (struct samsung_power_module *) module;
    int len;
    int32_t duration_us;

    /* Bail out if low-power mode is active */
    if (current_power_profile == PROFILE_POWER_SAVE && hint != POWER_HINT_LOW_POWER
            && hint != POWER_HINT_SET_PROFILE) {
        return;
    }

    switch (hint) {
        case POWER_HINT_VSYNC:
//             ALOGV("%s: POWER_HINT_VSYNC: ", __func__);
            break;
        case POWER_HINT_INTERACTION:
            duration_us = (int32_t) data;
            ALOGV("%s: POWER_HINT_INTERACTION: %d", __func__, duration_us);
            send_boostpulse(samsung_pwr->boostpulse_fd);
            break;
        case POWER_HINT_LOW_POWER:
            ALOGV("%s: POWER_HINT_LOW_POWER", __func__);
            set_power_profile(samsung_pwr, data ? PROFILE_POWER_SAVE : PROFILE_BALANCED);
            break;
        case POWER_HINT_CPU_BOOST:
            duration_us = *((int32_t *)data);
            ALOGV("%s: POWER_HINT_CPU_BOOST: %d", __func__, duration_us);
            send_boost(samsung_pwr->boost_fd, duration_us);
            break;
        case POWER_HINT_SET_PROFILE:
            ALOGV("%s: POWER_HINT_SET_PROFILE", __func__);
            int profile = *((intptr_t *)data);
            set_power_profile(samsung_pwr, profile);
            break;
        case POWER_HINT_LAUNCH:
            ALOGV("%s: POWER_HINT_LAUNCH", __func__);
            break;
        case POWER_HINT_VR_MODE:
            ALOGV("%s: POWER_HINT_VR_MODE", __func__);
            break;
        case POWER_HINT_DISABLE_TOUCH:
            ALOGV("%s: POWER_HINT_DISABLE_TOUCH", __func__);
            break;
        case POWER_HINT_SUSTAINED_PERFORMANCE:
            ALOGV("%s: POWER_HINT_SUSTAINED_PERFORMANCE", __func__);
            break;
        default:
            ALOGW("%s: Unknown power hint: %d", __func__, hint);
            break;
    }
}

static int samsung_get_feature(struct power_module *module __unused,
                               feature_t feature)
{
    if (feature == POWER_FEATURE_SUPPORTED_PROFILES) {
        return PROFILE_MAX;
    }

    return -1;
}

static void samsung_set_feature(struct power_module *module, feature_t feature, int state __unused)
{
    struct samsung_power_module *samsung_pwr = (struct samsung_power_module *) module;

    switch (feature) {
#ifdef TARGET_TAP_TO_WAKE_NODE
        case POWER_FEATURE_DOUBLE_TAP_TO_WAKE:
            ALOGV("%s: %s double tap to wake", __func__, state ? "enabling" : "disabling");
            sysfs_write(TARGET_TAP_TO_WAKE_NODE, state > 0 ? "1" : "0");
            break;
#endif
        default:
            break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct samsung_power_module HAL_MODULE_INFO_SYM = {
    .base = {
        .common = {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = POWER_MODULE_API_VERSION_0_2,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = POWER_HARDWARE_MODULE_ID,
            .name = "Samsung Power HAL",
            .author = "The LineageOS Project",
            .methods = &power_module_methods,
        },

        .init = samsung_power_init,
        .setInteractive = samsung_power_set_interactive,
        .powerHint = samsung_power_hint,
        .getFeature = samsung_get_feature,
        .setFeature = samsung_set_feature
    },

    .lock = PTHREAD_MUTEX_INITIALIZER,
    .boostpulse_fd = -1,
};
