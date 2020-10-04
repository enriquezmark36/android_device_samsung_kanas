/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "android.hardware.usb@1.0-service.kanas"

#include <hidl/HidlTransportSupport.h>
#include "Usb.h"
#include "UsbGadget.h"

using android::sp;

// libhwbinder:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

// Generated HIDL files
using android::hardware::usb::V1_0::IUsb;
using android::hardware::usb::V1_0::implementation::Usb;
using android::hardware::usb::gadget::V1_0::IUsbGadget;
using android::hardware::usb::gadget::V1_0::implementation::UsbGadget;

using android::OK;
using android::status_t;
int main() {
  android::sp<IUsb> service = new Usb();
  android::sp<IUsbGadget> service2 = new UsbGadget();
  status_t status;

  configureRpcThreadpool(2, true /*callerWillJoin*/);

  status = service->registerAsService();
  if (status != OK) {
    ALOGE("Cannot register USB HAL service");
    return 1;
  }
  ALOGI("USB HAL Ready.");

  status = service2->registerAsService();
  if (status != OK) {
    ALOGE("Cannot register USB Gadget HAL service");
    return 1;
  }
  ALOGI("USB Gadget HAL Ready.");

  joinRpcThreadpool();
  // Under noraml cases, execution will not reach this line.
  ALOGI("USB service failed to join thread pool.");
  return 1;
}