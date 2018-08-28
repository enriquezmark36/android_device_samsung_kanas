### Device tree for Samsung Galaxy Core 2 SM-G355H

## Building instructions
Hopefully, this can be useful to you.

### Prerequisites
1. Your LineageOS compilation/building guide. You can search some from Google
2. About 55 GB of free space
   * About 15 GB for the checked out source and 28 GB for building
   * About 12 GB or more for the download from 'repo sync'
3. Follow your guide until the 'repo sync' step, then do the instructions below.

### Instructions for using this tree
1. Make sure you've completed the 'repo init' step at least once.
2. Create the directory `.repo/local_manifests/`.
   * You may do a `mkdir -p .repo/local_manifests/` on Linux to create this.
   * ".repo" directory is a 'hidden folder' so you might need to configure your file manager to show hidden files and directories.
3. Create the file local_manifest.xml in that directory and copy-paste this:



```xml
<?xml version="1.0" encoding="UTF-8"?>
<!--
/*
** Copyright 2016 The Android Open Source Project
** Copyright 2016 The CyanogenMod Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
-->
<manifest>
	<project path="device/samsung/sprd-common" name="remilia15/android_device_samsung_sprd-common" remote="github" revision="cm-13.0"/>
	<project path="hardware/sprd" name="remilia15/android_hardware_sprd" remote="github" revision="cm-13.0"/>

	<project path="vendor/samsung/kanas" name="enriquezmark36/android_vendor_samsung_kanas" remote="github" revision="cm-13.0"/>
	<project path="device/samsung/kanas" name="enriquezmark36/android_device_samsung_kanas" remote="github" revision="cm-13.0"/>
	<project path="kernel/samsung/kanas" name="enriquezmark36/android_kernel_samsung_kanas" remote="github" revision="yack-testing"/>

	<project path="external/stlport" name="LineageOS/android_external_stlport" revision="cm-13.0" remote="github" />
	<project path="packages/apps/SamsungServiceMode" name="LineageOS/android_packages_apps_SamsungServiceMode" revision="cm-13.0" remote="github" />
	<project path="hardware/samsung" name="LineageOS/android_hardware_samsung" revision="cm-13.0" remote="github"/>
</manifest>
```



4. Do the `repo sync` step.
5. Go to or `cd` to the root of your source directory
   * It's where your ".repo" folder is located.
5. Run `device/samsung/kanas/patches/apply.sh`
   * This will apply some patches
6. Follow the rest of your guide after the `repo sync` step.
   * When asked what is our device or code name, it's "lineage_kanas-userdebug".
   * It may also say to extract binary blobs by running `extract-files.sh`. You may safely ignore this. Dude, trust me. There's no "extract-files.sh" anyway.


