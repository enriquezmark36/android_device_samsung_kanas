/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_USB_GADGET_V1_0_USBGADGET_H
#define ANDROID_HARDWARE_USB_GADGET_V1_0_USBGADGET_H

#include <android-base/file.h>
#include <android-base/properties.h>
#include <android/hardware/usb/gadget/1.0/IUsbGadget.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace usb {
namespace gadget {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::base::GetProperty;
using ::android::base::SetProperty;
using ::android::base::WriteStringToFile;
using ::android::base::ReadFileToString;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct UsbGadget : public IUsbGadget {
  UsbGadget() {};

  Return<void> setCurrentUsbFunctions(uint64_t functions,
                                      const sp<IUsbGadgetCallback>& callback,
                                      uint64_t timeout) override;

  Return<void> getCurrentUsbFunctions(
      const sp<IUsbGadgetCallback>& callback) override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace gadget
}  // namespace usb
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_USB_V1_2_USBGADGET_H
