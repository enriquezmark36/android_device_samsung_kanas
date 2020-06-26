/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2014 The CyanogenMod Project
 * Copyright (C) 2017 The Lineage Project
 * Copyright (C) 2014-2015 Andreas Schneider <asn@cryptomilk.org>
 * Copyright (C) 2014-2015 Christopher N. Hesse <raymanfx@gmail.com>
 * Copyright (C) 2020 Mark Enriquez <enriquezmark36@gmail.com>
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

/*
 * This contains some edits to properly work in
 * Samsung Galaxy Core 2 Duos (SM-G355H/kanas3g*****)
 * with support to my YACK kernel and the stock kernel.
 */
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#define LOG_TAG "KanasPowerHAL"
/* #define LOG_NDEBUG 0 */
#include <log/log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define PARAM_MAXLEN      10

#define CPU_SYSFS_PATH          "/sys/devices/system/cpu"
#define CPUFREQ_SYSFS_PATH      CPU_SYSFS_PATH "/cpufreq/"
#define CPU_MAX_FREQ_PATH       CPU_SYSFS_PATH "/cpu0/cpufreq/cpuinfo_max_freq"
#define CPU_MIN_FREQ_PATH       CPU_SYSFS_PATH "/cpu0/cpufreq/cpuinfo_min_freq"
#define SCALING_GOVERNOR_PATH   CPU_SYSFS_PATH "/cpu0/cpufreq/scaling_governor"
#define SCALING_MAX_FREQ_PATH   CPU_SYSFS_PATH "/cpu0/cpufreq/scaling_max_freq"
#define SCALING_MIN_FREQ_PATH   CPU_SYSFS_PATH "/cpu0/cpufreq/scaling_min_freq"
#define PANEL_BRIGHTNESS        "/sys/class/backlight/panel/brightness"

/*
 * Samsung's kernels have a cpulimit driver that enforces CPUFreq scaling
 * limits (to the point that you may not change it once active).
 * It's applied to all CPUs (even when with hotplug) and is more simpler
 * and more enforcing than the lenient scaling_m*_freq.
 *
 * For example, simply setting it to '-1' will just reset the limits
 * and even remembers whatever the user has chosen before in
 * SCALING_M*_FREQ_PATH files. It should also not conflict with
 * kernel apps that changes the permissions for themselves.
 * Present on the stock kernel and some, if not all, custom kernels.
 */
#define CPU_LIMIT_MAX     "/sys/power/cpufreq_max_limit"
#define CPU_LIMIT_MIN     "/sys/power/cpufreq_min_limit"

/*
 * Samsung's Touchwiz has an option to limit the maximum LCD refresh rate
 * which reduces the number of frames to be rendered thus decreasing
 * GPU load and clocks. It does so by using a special IOCTL request
 * but we can achieve the same effect using this sysfs knob.
 * Present on the stock kernel and custom kernels.
 */
#define LCD_FPS_PATH      "/sys/devices/virtual/dispc/dispc/fps"

/*
 * This is a special user made DFS algorithm for kanas which maps
 * Mali's pp/gp load to a tuple: (frequency,core count,capacity)
 * It can scale down to that exact capacity in one step or
 * Scale up in steps.
 * Present on custom kernels only.
 */
#define GPU_CORE_SCALER   "/sys/module/mali/parameters/mali_core_scaling"
#define GPU_CORE_TARUTIL  "/sys/module/mali/parameters/mali_core_tarutil"
#define GPU_CORE_MINLOAD  "/sys/module/mali/parameters/mali_core_minload"
#define GPU_CORE_NUM      "/sys/module/mali/parameters/mali_max_pp_cores_group_1"

/*
 * Samsung's Touchwiz has an option to limit the maximum CPU Frequency
 * to save battery power. This is the frequency it limits to,
 * which can be applied during PROFILE_BIAS_POWER_SAVE.
 * Unit is in Kilohertz(KHz)
 */
#define CPU_SUST_FREQ     "1000000"

/*
 * Android O and above has removed the support for earlysuspend.
 * That's fine for newer devices, not so much for us though.
 * wakeup_count is the future as it's the only mechanism supported.
 * Luckily, the 3.10.17 kernel this phone came with does support it
 * but with earlysuspend enabled, it brings complications.
 * wakeup_count can handle the suspend, but it can't handle
 * the resume that a lateresume will do.
 *
 * A simple short reason why we can't migrate away from earlysuspend
 * is that "vendor blobs depend on it". 'refnotify' is one example.
 *
 * A fix by the maintainer of scx35-common is a patch to add back
 * the earlysuspend support.
 *
 * A much more simple, non-intrusive, fix is to use the fact that
 * Android calls the setInteractive() or power_set_interactive()
 * of this module everytime a suspend or wake request is sent.
 *
 * So the fix? Just issue an "on" state to trigger a manual "lateresume"
 * a matter that was also done by Android before removing the support.
 * config_powerDecoupleInteractiveModeFromDisplay has to be false
 * so that this assumption is always true.
 */
#define EARLYSUSPEND_SYS_POWER_STATE "/sys/power/state"
#define EARLYSUSPEND_WAIT_FOR_FB_WAKE "/sys/power/wait_for_fb_wake"
#define EARLYSUSPEND_WAIT_FOR_FB_SLEEP "/sys/power/wait_for_fb_sleep"
/*
 * To detect whether the fix I mentioned (created by the maintainer)
 * is in use, we can simply find the symbol. When it exists,
 * just call it a day and forget the fix I wrote up above.
 */
#define LIBSUSPEND_PATH "/system/lib/libsuspend.so"
#define LIBSUSPEND_SYMBOL "autosuspend_earlysuspend_init"

/*
 * Interactive governor.
 * The cpu_interactive_read/write() helper functions are removed
 * in favor of just chdir'ing to the cpufreq governor directory.
 * It saves a bit of memory and time from allocating.
 */
#define HISPEED_FREQ_PATH       "hispeed_freq"
#define IO_IS_BUSY_PATH         "io_is_busy"
#define BOOSTPULSE_PATH         "boostpulse"

static char CPU_INTERACTIVE_PATH[80];
char g_max_freq[PARAM_MAXLEN];
char g_min_freq[PARAM_MAXLEN];
char g_has_cpufreq_limit = 0;
char g_has_gpu_core_scaler = 0;
char g_has_hispeed_freq = 0;
char g_has_boostpulse = 0;
char g_has_dispc_fps_notif = 0;
char g_need_lateresume = 0;

enum power_profile_e {
	PROFILE_POWER_SAVE = 0,
	PROFILE_BALANCED,
	PROFILE_HIGH_PERFORMANCE,
	PROFILE_BIAS_POWER_SAVE,
	PROFILE_BIAS_PERFORMANCE,
	PROFILE_MAX
};
static enum power_profile_e current_power_profile = PROFILE_MAX;

enum cpufreq_limit_e {
	CPUFREQ_LIMIT_MIN = 0,
	CPUFREQ_LIMIT_MAX,
	CPUFREQ_RESTORE_MIN,
	CPUFREQ_RESTORE_MAX,
};

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
		ALOGE("Error opening %s: %s\n", path, errno_str);
		return -1;
	}

	len = read(fd, s, num_bytes - 1);
	if (len < 0) {
		strerror_r(errno, errno_str, sizeof(errno_str));
		ALOGE("Error reading from %s: %s\n", path, errno_str);

		ret = -1;
	} else {
		s[len] = '\0';
	}

	close(fd);

	return ret;
}

static void sysfs_write(const char *path, char *s) {

	char errno_str[64];
	int len;
	int fd;

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		strerror_r(errno, errno_str, sizeof(errno_str));
		ALOGE("Error opening %s: %s\n", path, errno_str);
		return;
	}

	len = write(fd, s, strlen(s));

	if (len < 0) {
		strerror_r(errno, errno_str, sizeof(errno_str));
		ALOGE("Error writing to %s: %s\n", path, errno_str);
	}

	close(fd);
}


/*
 * Attempts to call sysfs_write() after ensuring that the
 * owner and group has a write permission.
 * Reverts the permission bits changes afterwards.
 * Assumes that this process belongs to the User or the Group.
 */
static void sysfs_write_permissive(const char *path, char *s)
{
	struct stat st;
	if (stat(path, &st) == 0) {
		ALOGW("Cannot stat %s: %s", path, strerror(errno));
		return;
	}

	if (chmod(path, st.st_mode | S_IWUSR | S_IWGRP)){
		ALOGW("Cannot add write permission: %s", strerror(errno));
		return;
	}

	sysfs_write(path, s);

	chmod(path, st.st_mode);
}

/*
 * Some kernel apps like to change the permission bits
 * from 0644 to 440. I don't know why.
 * This function is written against those selfish apps.
 *
 */
static inline void sysfs_write_patiently(const char *path, char *s) {
	if (access(path, W_OK))
		sysfs_write_permissive(SCALING_MAX_FREQ_PATH, s);
	else
		sysfs_write(SCALING_MAX_FREQ_PATH, s);
}

static int get_cpu_interactive_paths() {

	char governor[20];

	if (sysfs_read(SCALING_GOVERNOR_PATH, governor, sizeof(governor)) == -1) {
		return -1;
	} else {
		// Strip newline at the end.
		int len = strlen(governor) - 1;

		while (len >= 0 && (governor[len] == '\n' || governor[len] == '\r'))
		governor[len--] = '\0';
	}

	sprintf(CPU_INTERACTIVE_PATH, "%s%s", CPUFREQ_SYSFS_PATH, governor);

	if (strncmp(governor, "interactive", 11) == 0 ||
		strncmp(governor, "intelliactive", 13) == 0) {
		ALOGI("Current interactive governor is: %s", governor);
	} else {
		ALOGI("Current governor is: %s", governor);
	}

	if (chdir(CPU_INTERACTIVE_PATH)) { //error condition
		ALOGW("chdir(): %s: %s", CPU_INTERACTIVE_PATH, strerror(errno));
		return -1;
	}
	return 0;
}

static void send_boostpulse(void)
{
	int boostpulse_fd = open(BOOSTPULSE_PATH, O_WRONLY);

	if (boostpulse_fd < 0) {
		ALOGE("Error opening %s: %s\n", BOOSTPULSE_PATH, strerror(errno));
		return;
	}

	if (write(boostpulse_fd, "1", 1) < 0)
		ALOGE("Error writing to boostpulse path: %s", strerror(errno));

	close(boostpulse_fd);
}

/**********************************************************
 *** POWER FUNCTIONS
 **********************************************************/
static void set_cpufreq_limit(enum cpufreq_limit_e type, char *freq)
{
	char temp_freq[PARAM_MAXLEN];

	switch (type) {
		case CPUFREQ_RESTORE_MIN:
			if (g_has_cpufreq_limit)
				sysfs_write(CPU_LIMIT_MIN, "-1");
			else
				sysfs_write_patiently(SCALING_MIN_FREQ_PATH, g_min_freq);
			break;
		case CPUFREQ_RESTORE_MAX:
			if (g_has_cpufreq_limit) {
				sysfs_write(CPU_LIMIT_MAX, "-1");
			} else if (!access(HISPEED_FREQ_PATH, R_OK | F_OK)) {
				// If the governor supports it, restore from the hispeed parameter
				sysfs_read(HISPEED_FREQ_PATH, temp_freq, sizeof(temp_freq));
				sysfs_write_patiently(SCALING_MAX_FREQ_PATH, temp_freq);
			} else {
				sysfs_write_patiently(SCALING_MAX_FREQ_PATH, g_max_freq);
			}
			break;
		case CPUFREQ_LIMIT_MIN:
			if (g_has_cpufreq_limit)
				sysfs_write(CPU_LIMIT_MIN, freq);
			else
				sysfs_write_patiently(SCALING_MAX_FREQ_PATH, freq);
			break;
		case CPUFREQ_LIMIT_MAX:
			if (g_has_cpufreq_limit)
				sysfs_write(CPU_LIMIT_MAX, freq);
			else
				sysfs_write_patiently(SCALING_MIN_FREQ_PATH, freq);
			/* Fall-through */
		default:
		break;
	}
	return;
}

static void set_lcd_fps(int profile)
{
	if (!g_has_dispc_fps_notif)
		return;

	switch (profile) {
	case PROFILE_POWER_SAVE:
		ALOGV("%s: Attempting to reduce refresh rate to 33fps", __func__);
		sysfs_write(LCD_FPS_PATH, "45");
		/*
		 * Samsung's kernel does not support 33 fps but due to the way
		 * SPRD DISPC kernel driver code is written. Writing "33"
		 * will be silently ignored but will not reset the previous setting.
		 * Meaning if we'd set 45 before, it will stay at 45 fps.
		 *
		 * Why 33fps? It's the minimum fps that will need a clock still fast
		 * enough to refresh all of the lcd pixels. Anything lower than that
		 * will result to a slower clock and with the display being
		 * cut-off to white from the bottom.
		 *
		 * FYI, the highest fps possible, which results to the highest clock,
		 * without breaking the system is 69fps. Anything higher will really
		 * mess up the timing and will cause the screen to fade to white.
		 */
		sysfs_write(LCD_FPS_PATH, "33");
		break;
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

static void set_gpu_core_scaler(int profile)
{
	char *target_util, *reserved_idle_power, *scaling_enabled;
	static int current_profile = PROFILE_MAX;
	if (!g_has_gpu_core_scaler)
		return;

	if (current_profile == profile)
		return;

	/*
	 * Some background:
	 *       Every 0.3 secs, Mali rates the gpu load from 0 to 255.
	 *       The DFS will then scale that load to 0-512 using some
	 *           non-linear mapping. We'll call it as scaled_load.
	 *
	 * target_util - a knob in the DFS algorithm that
	 * indicates how high must the gpu load be before we enable more
	 * cores. A value of 256 effectively prevents the DFS from
	 * enabling cores and only allows disabling.
	 *
	 * reserved_idle_power - minimum scaled_load the DFS will see.
	 * If the actual scaled_load is 56 while the reserved_idle_power
	 * is 160, in the DFS code scaled_load will be pulled up to 160
	 * even if the gpu is actually idle.
	 */
	switch (profile) {
		case PROFILE_POWER_SAVE:
			target_util = "256"; // Do not enable cores
			reserved_idle_power = "230"; // Capacity at 1 core 312MHZ
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
			/*
			 * Uses the stock DFS, no core scaling.
			 * All 4 cores enabled, only the frequency is changed
			 */
			scaling_enabled = "0";
		break;
		case PROFILE_HIGH_PERFORMANCE:
			// Use all four cores in maximum frequency.
			target_util = "1";
			reserved_idle_power = "512";
			scaling_enabled = "1";
		break;
    }

	ALOGV("%s: Configuring GPU core scaling with these parameters:\n"
		"GPU core scaling enabled: %s\n"
		"Target GPU utilization: %s\n"
		"Reserved GPU capacity: %s\n",
		__func__, scaling_enabled[0] == '1' ? "Yes" : "No", target_util, reserved_idle_power);
	sysfs_write(GPU_CORE_TARUTIL, target_util);
	sysfs_write(GPU_CORE_MINLOAD, reserved_idle_power);
	sysfs_write(GPU_CORE_SCALER, scaling_enabled);

	current_profile = profile;
    return;
}


static void set_power_profile(enum power_profile_e profile)
{
	char temp_freq[10];

	if (current_power_profile == profile) {
		return;
	}

	ALOGV("%s: profile=%d", __func__, profile);

	switch (profile) {
	case PROFILE_POWER_SAVE:
		// Limit to min freq which was set through sysfs
		sysfs_read(SCALING_MIN_FREQ_PATH, temp_freq, sizeof(temp_freq));
		set_cpufreq_limit(CPUFREQ_LIMIT_MAX, temp_freq);
		ALOGD("%s: set powersave mode", __func__);
		break;
        case PROFILE_BIAS_POWER_SAVE:
		set_cpufreq_limit(CPUFREQ_RESTORE_MIN, 0);
		set_cpufreq_limit(CPUFREQ_LIMIT_MAX, CPU_SUST_FREQ);
		ALOGV("%s: set efficiency mode", __func__);
		break;
	case PROFILE_BALANCED:
		set_cpufreq_limit(CPUFREQ_RESTORE_MAX, 0);
		set_cpufreq_limit(CPUFREQ_RESTORE_MIN, 0);
		ALOGD("%s: set balanced mode", __func__);
		break;
	case PROFILE_BIAS_PERFORMANCE:
		ALOGV("%s: set quick mode [fake] ", __func__);
	case PROFILE_HIGH_PERFORMANCE:
		// Restore normal max freq
		sysfs_read(CPU_MAX_FREQ_PATH, temp_freq, sizeof(temp_freq));
		set_cpufreq_limit(CPUFREQ_RESTORE_MAX, 0);
		set_cpufreq_limit(CPUFREQ_LIMIT_MIN, temp_freq);
		ALOGD("%s: set performance mode", __func__);
		break;
	case PROFILE_MAX:
		break;
	}

	set_lcd_fps(profile);
	set_gpu_core_scaler(profile);
	current_power_profile = profile;
}

/**********************************************************
 *** INIT FUNCTIONS
 **********************************************************/
static void gpu_ctrl_open(void)
{
	char gpu_param[2];
	int cores, ret;

	// Check if we should enable gpu scaling on >= 2 cores
	// NOTE: This code block is written for MALI 400 MP
	// (where core count can be only up to four)
	g_has_gpu_core_scaler = 0;
	ret = sysfs_read(GPU_CORE_NUM, gpu_param, 2);
	cores = gpu_param[0] - '0'; // total number of gpu cores
	if (!ret && cores >= 2 && cores <= 4) {
		if (!access(GPU_CORE_SCALER, R_OK | W_OK | F_OK))
			g_has_gpu_core_scaler = 1;
	}
}

static int check_earlysuspend_libsuspend(void)
{
	void *handle;
	char *error;


	handle = dlopen(LIBSUSPEND_PATH, RTLD_LAZY | RTLD_LOCAL);
	if (!handle) {
		ALOGD("Failed to load %s: %s", LIBSUSPEND_PATH, dlerror());
		return -1;
	}

	// We are just locating the symbol, no need to store the address
	dlsym(handle, LIBSUSPEND_SYMBOL);
	error = dlerror();
	if (error) {
		ALOGD("Cannot find symbol: %s", error);
		ALOGD("The previous line is not an error.");
		return -1;
	}

	dlclose(handle);
	return 0;
}
/*
 * The init function performs power management setup actions at runtime
 * startup, such as to set default cpufreq parameters.  This is called only by
 * the Power HAL instance loaded by PowerManagerService.
 */
void power_init() {
	ALOGI("Initialized settings:");

	get_cpu_interactive_paths();
	ALOGI("CPUFreq governor path: %s", CPU_INTERACTIVE_PATH);

	// Store set frequencies for restore later on
	sysfs_read(CPU_MAX_FREQ_PATH, g_max_freq, sizeof(g_max_freq));
	sysfs_read(CPU_MIN_FREQ_PATH, g_min_freq, sizeof(g_min_freq));
	ALOGI("max_freq: %s", g_max_freq);
	ALOGI("min_freq: %s", g_min_freq);

	gpu_ctrl_open();
	ALOGI("Scale GPU cores: %s", g_has_gpu_core_scaler? "Yes" : strerror(errno));

	// Check if CPU Limit is supported by the kernel
	// It makes limiting much simpler
	if (access(CPU_LIMIT_MAX, R_OK | W_OK | F_OK) == 0 &&
	    access(CPU_LIMIT_MIN, R_OK | W_OK | F_OK) == 0) {
		g_has_cpufreq_limit = 1;
	}
	ALOGI("Using cpufreq_limit interface: %s",
	      g_has_cpufreq_limit ? "Yes" : strerror(errno));

	// Check if we can change the LCD display's refresh rate on the fly
	if (!access(LCD_FPS_PATH, F_OK))
		g_has_dispc_fps_notif = 1;
	ALOGI("Using DISPC dynamic fps: %s", g_has_dispc_fps_notif ? "Yes" : strerror(errno));

	// Check if we have libsuspend patched with earlysuspend
	if (check_earlysuspend_libsuspend())
		g_need_lateresume = 1;
	ALOGI("Trigger lateresume: %s", g_need_lateresume? "Yes" : "No");
}

/*
 * The setInteractive function performs power management actions upon the
 * system entering interactive state (that is, the system is awake and ready
 * for interaction, often with UI devices such as display and touchscreen
 * enabled) or non-interactive state (the system appears asleep, display
 * usually turned off).  The non-interactive state is usually entered after a
 * period of inactivity, in order to conserve battery power during such
 * inactive periods.
 *
 * Typical actions are to turn on or off devices and adjust cpufreq parameters.
 * This function may also call the appropriate interfaces to allow the kernel
 * to suspend the system to low-power sleep state when entering non-interactive
 * state, and to disallow low-power suspend when the system is in interactive
 * state.  When low-power suspend state is allowed, the kernel may suspend the
 * system whenever no wakelocks are held.
 *
 * on is non-zero when the system is transitioning to an interactive / awake
 * state, and zero when transitioning to a non-interactive / asleep state.
 *
 * This function is called to enter non-interactive state after turning off the
 * screen (if present), and called to enter interactive state prior to turning
 * on the screen.
 */
void power_set_interactive(int on)
{
	ALOGV("power_set_interactive: %d\n", on);

	if (!access(IO_IS_BUSY_PATH, W_OK | F_OK))
		sysfs_write(IO_IS_BUSY_PATH, on ? "1" : "0");

	if (g_need_lateresume && on) {
		char buff[6];
		sysfs_write(EARLYSUSPEND_SYS_POWER_STATE, "on"); // issue request
		sysfs_read(EARLYSUSPEND_WAIT_FOR_FB_WAKE, buff, sizeof(buff)); // wait
	}

	ALOGV("power_set_interactive: %d done\n", on);
}

/*
 * The powerHint function is called to pass hints on power requirements, which
 * may result in adjustment of power/performance parameters of the cpufreq
 * governor and other controls.
 *
 * The possible hints are:
 *
 * POWER_HINT_VSYNC
 *
 *     Foreground app has started or stopped requesting a VSYNC pulse
 *     from SurfaceFlinger.  If the app has started requesting VSYNC
 *     then CPU and GPU load is expected soon, and it may be appropriate
 *     to raise speeds of CPU, memory bus, etc.  The data parameter is
 *     non-zero to indicate VSYNC pulse is now requested, or zero for
 *     VSYNC pulse no longer requested.
 *
 * POWER_HINT_INTERACTION
 *
 *     User is interacting with the device, for example, touchscreen
 *     events are incoming.  CPU and GPU load may be expected soon,
 *     and it may be appropriate to raise speeds of CPU, memory bus,
 *     etc.  The data parameter is unused.
 *
 * POWER_HINT_LOW_POWER
 *
 *     Low power mode is activated or deactivated. Low power mode
 *     is intended to save battery at the cost of performance. The data
 *     parameter is non-zero when low power mode is activated, and zero
 *     when deactivated.
 *
 * POWER_HINT_CPU_BOOST
 *
 *     An operation is happening where it would be ideal for the CPU to
 *     be boosted for a specific duration. The data parameter is an
 *     integer value of the boost duration in microseconds.
 */
void power_hint(power_hint_t hint, void *data)
{
	// Check if the cpufreq governor has been changed
	if (access(CPU_INTERACTIVE_PATH, F_OK)) {
		ALOGI("CPUFreq might have been changed");
		get_cpu_interactive_paths();
	}

	switch (hint) {
		case POWER_HINT_LOW_POWER:
			ALOGV("%s: POWER_HINT_LOW_POWER", __func__);
			set_power_profile(data ? PROFILE_POWER_SAVE : PROFILE_BALANCED);
			break;
		case POWER_HINT_INTERACTION: {
			if (current_power_profile == PROFILE_POWER_SAVE) {
				return;
			}
			ALOGV("%s: POWER_HINT_INTERACTION", __func__);

			if (!access(BOOSTPULSE_PATH, W_OK | F_OK))
				send_boostpulse();
			break;
		}
		case POWER_HINT_VSYNC: {
			int state = *((intptr_t *)data);

			ALOGV("%s: POWER_HINT_VSYNC: VSYNC %s", __func__, state ? "ON" : "OFF");
			break;
		}
		case POWER_HINT_SET_PROFILE: {
			int profile = *((intptr_t *)data);

			ALOGV("%s: POWER_HINT_SET_PROFILE", __func__);
			set_power_profile(profile);
			break;
		}
		case POWER_HINT_VR_MODE:
			ALOGV("%s: POWER_HINT_VR_MODE", __func__);
			break;
		case POWER_HINT_DISABLE_TOUCH:
			ALOGV("%s: POWER_HINT_DISABLE_TOUCH", __func__);
			break;
		case POWER_HINT_LAUNCH:
			ALOGV("%s: POWER_HINT_LAUNCH", __func__);
			break;
		case POWER_HINT_SUSTAINED_PERFORMANCE:
			ALOGV("%s: POWER_HINT_SUSTAINED_PERFORMANCE", __func__);
			break;
		default:
			break;
	}
}

int get_number_of_profiles()
{
    return PROFILE_MAX;
}
