/*
 * Copyright (C) 2020 The Android Open Source Project
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
#include "UsbGadget.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <unordered_map>

// #define LOG_NDEBUG 0
#if defined(LOG_NDEBUG) && (LOG_NDEBUG == 0)
#define LOG_TAG "android.hardware.usb.gadget@1.0-service.kanas.debug"
#else
#define LOG_TAG "android.hardware.usb.gadget@1.0-service.kanas"
#endif
#include <utils/Log.h>

#define ANDROID_USB_PATH "/sys/class/android_usb/android0/"
#define VENDOR_ID_PATH ANDROID_USB_PATH "idVendor"
#define PRODUCT_ID_PATH ANDROID_USB_PATH "idProduct"
#define DEVICE_CLASS_PATH ANDROID_USB_PATH "bDeviceClass"
#define DEVICE_SUB_CLASS_PATH ANDROID_USB_PATH "bDeviceSubClass"
#define DEVICE_PROTOCOL_PATH ANDROID_USB_PATH "bDeviceProtocol"
#define ENABLE_PATH ANDROID_USB_PATH "enable"
#define FUNCTIONS_PATH ANDROID_USB_PATH "functions"


namespace android {
namespace hardware {
namespace usb {
namespace gadget {
namespace V1_0 {
namespace implementation {
static V1_0::Status enableGadget(bool enable) {
  if (!WriteStringToFile(enable ? "1" : "0", ENABLE_PATH))
    return Status::ERROR;

  return Status::SUCCESS;
}

static bool getGadgetStatus(bool &status) {
  bool ret;
  std::string buf;

  ret = ReadFileToString(ENABLE_PATH, &buf, false);

  if (!ret)
    return false;

  status = buf.compare("1");

  return true;
}


static V1_0::Status setVidPid(const char *vid, const char *pid) {
  if (!WriteStringToFile(vid, VENDOR_ID_PATH))
    return Status::ERROR;

  if (!WriteStringToFile(pid, PRODUCT_ID_PATH))
    return Status::ERROR;

  return Status::SUCCESS;
}

static bool parseFunctions(uint64_t &functions) {
  static std::unordered_map<std::string, GadgetFunction> funcMap {
    { "acm", GadgetFunction::NONE },
    { "accessory", GadgetFunction::ACCESSORY},
    { "audio_source", GadgetFunction::AUDIO_SOURCE },
    { "adb", GadgetFunction::ADB },
    { "ffs", GadgetFunction::ADB },
    { "midi", GadgetFunction::MIDI },
    { "mtp", GadgetFunction::MTP },
    { "ptp", GadgetFunction::PTP },
    { "rndis", GadgetFunction::RNDIS },
    { "mass_storage", GadgetFunction::NONE }
};

  std::string buf, tok;
  std::istringstream stream;
  bool ret;

  ret = ReadFileToString(FUNCTIONS_PATH, &buf, false);
  if (!ret)
    return ret;

  functions = 0;
  stream = std::istringstream(buf);

  while (getline(stream, tok, ',')) {
    ALOGV("%s: Token %s",__func__, tok.c_str());
    auto search = funcMap.find(tok);
    if (search != funcMap.end()) {
      ALOGV("%s: found match (%llX)",__func__, (*search).second);
      functions |= (*search).second;
    }
  }

  return true;
}

/*
 * Validates the function set or configuration.
 *
 * Returns Status::SUCCESS is confiuration or the set is supported
 * and also sets outPid and outVid accordingly.
 *
 * Otherwise, returns Status::CONFIGURATION_NOT_SUPPORTED and does nothing
 * to the outPid and outVid variables.
 */
static V1_0::Status validateFunctions(uint64_t functions, const char *&outPid, const char *&outVid) {

  // default to Google's Vendor id
  outVid = "18d1";

  switch (static_cast<uint64_t>(functions)) {
    case GadgetFunction::ADB | GadgetFunction::NONE:
    case GadgetFunction::ADB | GadgetFunction::MTP:
    case GadgetFunction::MTP | GadgetFunction::NONE:
    case GadgetFunction::NONE | GadgetFunction::NONE:
      outVid = "04E8";
      outPid = "6860";
      break;
    case GadgetFunction::ADB | GadgetFunction::RNDIS:
      outVid = "04E8";
      outPid = "6864";
      break;
    case GadgetFunction::RNDIS | GadgetFunction::NONE:
      outVid = "04E8";
      outPid = "6863";
      break;
    case GadgetFunction::ADB | GadgetFunction::PTP:
      outVid = "04E8";
      outPid = "6866";
      break;
    case GadgetFunction::PTP | GadgetFunction::NONE:
      outVid = "04E8";
      outPid = "6865";
      break;

    /* Unsupported configurations but technically
     * are still supported by the kernel.
     * At these configurations, we'll be switching to Google's
     * vendor ID.
     */
    case GadgetFunction::ACCESSORY | GadgetFunction::NONE:
      outPid = "2d00";
      break;
    case GadgetFunction::ACCESSORY | GadgetFunction::ADB:
      outPid = "2D01";
      break;
    case GadgetFunction::AUDIO_SOURCE | GadgetFunction::NONE:
      outPid = "2D02";
      break;
    case GadgetFunction::AUDIO_SOURCE | GadgetFunction::ADB:
      outPid = "2D03";
      break;
    case GadgetFunction::ACCESSORY | GadgetFunction::AUDIO_SOURCE:
      outPid = "2D04";
      break;
    case GadgetFunction::ACCESSORY | GadgetFunction::AUDIO_SOURCE |
         GadgetFunction::ADB:
      outPid = "2D05";
      break;
    case GadgetFunction::MIDI | GadgetFunction::NONE:
      outPid = "4EE8";
      break;
    case GadgetFunction::MIDI | GadgetFunction::ADB:
      outPid = "4EE9";
      break;
    default:
      ALOGI("%s: Unsupported configuration (0x%llX)",__func__, functions);
      return V1_0::Status::CONFIGURATION_NOT_SUPPORTED;
  }

  ALOGV("%s: Configuration is valid! (Vid:0x%s Pid:0x%s)",
        __func__, outVid, outPid);
  return V1_0::Status::SUCCESS;
}

/*
 * Writes the string that should activate the functions described in the
 * configuration (via the functions variable).
 *
 * The configuration must be validated.
 *
 * Returns Status:SUCCESS when the strin is successfully written,
 * otherwise Status::ERROR
 */
static V1_0::Status setFunctions(uint64_t functions) {
  switch (static_cast<uint64_t>(functions)) {
    case GadgetFunction::ADB | GadgetFunction::NONE:
      if (!WriteStringToFile("adb", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::ADB | GadgetFunction::MTP:
      if (!WriteStringToFile("mtp,adb", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::MTP | GadgetFunction::NONE:
      if (!WriteStringToFile("mtp", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::ADB | GadgetFunction::RNDIS:
      if (!WriteStringToFile("rndis,adb", FUNCTIONS_PATH) ||
          !WriteStringToFile("2", DEVICE_CLASS_PATH) ||
          !WriteStringToFile("0", DEVICE_SUB_CLASS_PATH) ||
          !WriteStringToFile("0", DEVICE_PROTOCOL_PATH))
        goto error;
      break;
    case GadgetFunction::RNDIS | GadgetFunction::NONE:
      if (!WriteStringToFile("rndis", FUNCTIONS_PATH) ||
          !WriteStringToFile("2", DEVICE_CLASS_PATH) ||
          !WriteStringToFile("0", DEVICE_SUB_CLASS_PATH) ||
          !WriteStringToFile("0", DEVICE_PROTOCOL_PATH))
        goto error;
      break;
    case GadgetFunction::ADB | GadgetFunction::PTP:
      if (!WriteStringToFile("ptp,adb", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::PTP | GadgetFunction::NONE:
      if (!WriteStringToFile("ptp", FUNCTIONS_PATH))
        goto error;
      break;

    /*
     * Unsupported configurations but technically
     * are still supported by the kernel.
     * At these configurations, we'll be switching to Google's
     * vendor ID.
     */
    case GadgetFunction::ACCESSORY | GadgetFunction::NONE:
      if (!WriteStringToFile("accessory", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::ACCESSORY | GadgetFunction::ADB:
      if (!WriteStringToFile("accessory,adb", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::AUDIO_SOURCE | GadgetFunction::NONE:
      if (!WriteStringToFile("audio_source", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::AUDIO_SOURCE | GadgetFunction::ADB:
      if (!WriteStringToFile("audio_source,adb", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::ACCESSORY | GadgetFunction::AUDIO_SOURCE:
      if (!WriteStringToFile("accessory,audio_source", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::ACCESSORY | GadgetFunction::AUDIO_SOURCE |
         GadgetFunction::ADB:
      if (!WriteStringToFile("accessory,audio_source,adb", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::MIDI | GadgetFunction::ADB:
      if (!WriteStringToFile("midi,adb", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::MIDI | GadgetFunction::NONE:
      if (!WriteStringToFile("midi", FUNCTIONS_PATH))
        goto error;
      break;
    case GadgetFunction::NONE | GadgetFunction::NONE:
      if (!WriteStringToFile("", FUNCTIONS_PATH))
        goto error;
      break;
    default:
      goto error;
  }

  return V1_0::Status::SUCCESS;

error:
  return V1_0::Status::ERROR;
}


Return<void> UsbGadget::getCurrentUsbFunctions(
    const sp<V1_0::IUsbGadgetCallback> &callback)
{
  uint64_t functions = 0;
  bool active = false;

  if (!parseFunctions(functions))
        ALOGE("%s: cannot query usb gadget functions", __func__);

  if (!getGadgetStatus(active))
        ALOGE("%s: cannot query usb gadget status", __func__);

  ALOGV("%s: functions (%llX) enable (%d)", __func__, functions, active);

  if (callback) {
    Return<void> ret = callback->getCurrentUsbFunctionsCb(
                           functions, active ? Status::FUNCTIONS_APPLIED
                                             : Status::FUNCTIONS_NOT_APPLIED);
    if (!ret.isOk())
      ALOGE("%s, Call to getCurrentUsbFunctionsCb failed %s",
            __func__, ret.description().c_str());
  }

  return Void();
}

Return<void> UsbGadget::setCurrentUsbFunctions(
    uint64_t functions, const sp<V1_0::IUsbGadgetCallback> &callback,
    uint64_t /*timeout*/)
{

  V1_0::Status status = Status::SUCCESS;
  const char *pid, *vid;
  Return<void> ret;

  status = validateFunctions(functions, pid, vid);
  if (status != Status::SUCCESS) {
    ALOGE("%s: function set is invalid", __func__);
    goto out;
  }

  status = enableGadget(false);
  if (status != Status::SUCCESS) {
    ALOGE("%s: Failed to disable gadget", __func__);
    goto out;
  }

  status = setVidPid(vid, pid);
  if (status != Status::SUCCESS) {
    ALOGE("%s: Failed to set Vendor and Product IDs", __func__);
    goto out;
  }

  status = setFunctions(functions);
  if (status != Status::SUCCESS) {
    ALOGE("%s: Failed to set functions, gadget state left incomplete", __func__);
    goto out;
  }

  // When the function set is nothing, don't enable the gadget
  if (static_cast<GadgetFunction>(functions) != GadgetFunction::NONE) {
    status = enableGadget(true);
    if (status != Status::SUCCESS) {
      ALOGE("%s: Failed to enable gadget", __func__);
      status = Status::FUNCTIONS_NOT_APPLIED;
      goto out;
    }
  }

out:
  if (callback) {
    ret = callback->setCurrentUsbFunctionsCb(functions, status);
    if (!ret.isOk())
      ALOGE("%s, Error while calling setCurrentUsbFunctionsCb %s",
            __func__, ret.description().c_str());
  }

  ALOGI("%s, %s", __func__, status == Status::SUCCESS ? "Success" : "Failed");
  return Void();
}


}  // namespace implementation
}  // namespace V1_0
}  // namespace gadget
}  // namespace usb
}  // namespace hardware
}  // namespace android
