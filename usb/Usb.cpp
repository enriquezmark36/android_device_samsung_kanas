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
#include <assert.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <cutils/uevent.h>
#include <sys/epoll.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

#include "Usb.h"

namespace android {
namespace hardware {
namespace usb {
namespace V1_0 {
namespace implementation {

Return<void> Usb::switchRole(const hidl_string& portName,
        const PortRole& newRole) {
    Return<void> ret;

    ALOGI("Role switching is not supported.");

    if (mCallback == nullptr)
        return Void();

    ret = mCallback->notifyRoleSwitchStatus(portName, newRole, Status::SUCCESS);
    if (!ret.isOk())
        ALOGE("RoleSwitchStatus error %s", ret.description().c_str());

    return Void();
}

Return<void> Usb::queryPortStatus() {
    hidl_vec<PortStatus> currentPortStatus;
    Status status = Status::SUCCESS;
    Return<void> ret ;

    if (mCallback == nullptr)
        return Void();

    currentPortStatus.resize(1);
    currentPortStatus[0].portName = "dwc_otg";
    currentPortStatus[0].currentPowerRole = PortPowerRole::SINK;
    currentPortStatus[0].currentDataRole = PortDataRole::DEVICE;
    currentPortStatus[0].currentMode = PortMode::UFP;

    currentPortStatus[0].canChangeMode = false;
    currentPortStatus[0].canChangeDataRole = false;
    currentPortStatus[0].canChangePowerRole = false;
    currentPortStatus[0].supportedModes = PortMode::UFP;

    ret = mCallback->notifyPortStatusChange(currentPortStatus, status);
    if (!ret.isOk())
        ALOGE("PortStatusChange error %s", ret.description().c_str());

    return Void();
}

Return<void> Usb::setCallback(const sp<IUsbCallback>& callback) {
    ALOGI("registering callback");
    mCallback = callback;

    if (callback == NULL)
        mCallback = nullptr;

    return Void();
}


Usb::Usb() :
    mCallback(nullptr) {
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace usb
}  // namespace hardware
}  // namespace android
