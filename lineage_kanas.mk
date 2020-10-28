#
# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Inherit device configuration
$(call inherit-product, $(LOCAL_PATH)/device_kanas.mk)
# Specify phone tech before including full_phone
$(call inherit-product, vendor/lineage/config/telephony.mk)
# Inherit some common Lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Override build date
PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0

# Device identifier
PRODUCT_DEVICE := kanas
PRODUCT_NAME := lineage_kanas
PRODUCT_BRAND := samsung
PRODUCT_MODEL := SM-G355H
PRODUCT_MANUFACTURER := samsung
PRODUCT_CHARACTERISTICS := phone
PRODUCT_GMS_CLIENTID_BASE := android-samsung

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=kanas3gxx \
    PRIVATE_BUILD_DESC="kanas3gxx-user 4.4.2 KOT49H G355HXXS0AQD1 test-keys"

# Inherit device post configuration, can be use to override anything from the earlier makefiles
$(call inherit-product, $(LOCAL_PATH)/device_kanas_post.mk)

# Build fingerprint
BUILD_FINGERPRINT := samsung/kanas3gxx/kanas:4.4.2/KOT49H/G355HXXS0AQD1:user/release-keys

