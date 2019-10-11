/*
 * Copyright (C) 2012 The CyanogenMod Project
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

package com.android.internal.telephony;

import static com.android.internal.telephony.RILConstants.*;

import android.content.Context;
import android.os.AsyncResult;
import android.os.Message;
import android.os.Parcel;
import android.os.SystemProperties;
import android.telephony.Rlog;
import android.telephony.PhoneNumberUtils;
import android.telephony.ModemActivityInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.android.internal.telephony.uicc.SpnOverride;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.RILConstants;
import com.android.internal.telephony.gsm.SmsBroadcastConfigInfo;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

/**
 * Custom RIL to handle unique behavior of SPRD RIL
 *
 * {@hide}
 */
public class KanasRIL extends SamsungSPRDRIL implements CommandsInterface {

    public KanasRIL(Context context, int preferredNetworkType, int cdmaSubscription) {
        this(context, preferredNetworkType, cdmaSubscription, null);
    }

    public KanasRIL(Context context, int preferredNetworkType,
            int cdmaSubscription, Integer instanceId) {
        super(context, preferredNetworkType, cdmaSubscription, instanceId);
    }

    @Override
    public void startLceService(int reportIntervalMs, boolean pullMode, Message response) {
        riljLog("Link Capacity Estimate (LCE) service is not supported!");
        if (response != null) {
            AsyncResult.forMessage(response, null, new CommandException(
                    CommandException.Error.REQUEST_NOT_SUPPORTED));
            response.sendToTarget();
        }
    }

    @Override
    public void getHardwareConfig (Message result) {
        riljLog("Ignoring call to 'getHardwareConfig'");
        if (result != null) {
            AsyncResult.forMessage(result, null, new CommandException(
                    CommandException.Error.REQUEST_NOT_SUPPORTED));
            result.sendToTarget();
        }
    }

    @Override
    public void getModemActivityInfo(Message result) {
        if (RILJ_LOGD) riljLog("Ignoring call to getModemActivityInfo()");
        if (result != null) {
            AsyncResult.forMessage(result, null, new CommandException(
                    CommandException.Error.REQUEST_NOT_SUPPORTED));
            result.sendToTarget();
        }
    }

    @Override
    public void getCellInfoList(Message result) {
        if (RILJ_LOGD) riljLog("Ignoring call to getCellInfoList()");
        if (result != null) {
            AsyncResult.forMessage(result, null, new CommandException(
                    CommandException.Error.REQUEST_NOT_SUPPORTED));
            result.sendToTarget();
        }
    }

    @Override
    public void setUiccSubscription(int appIndex, boolean activate, Message result) {
        if (RILJ_LOGD) riljLog("setUiccSubscription: appIndex: " + appIndex + " activate: " + activate);

        // Fake response (note: should be sent before mSubscriptionStatusRegistrants or
        // SubscriptionManager might not set the readiness correctly)
        AsyncResult.forMessage(result, 0, null);
        result.sendToTarget();

        // TODO: Actually turn off/on the radio (and don't fight with the ServiceStateTracker)
        if (activate == true /* ACTIVATE */) {
            // Subscription changed: enabled
            if (mSubscriptionStatusRegistrants != null) {
                mSubscriptionStatusRegistrants.notifyRegistrants(
                        new AsyncResult (null, new int[] {1}, null));
            }
        } else if (activate == false /* DEACTIVATE */) {
            // Subscription changed: disabled
            if (mSubscriptionStatusRegistrants != null) {
                mSubscriptionStatusRegistrants.notifyRegistrants(
                        new AsyncResult (null, new int[] {0}, null));
            }
        }
    }

    @Override
    protected void notifyRegistrantsRilConnectionChanged(int rilVer) {
        super.notifyRegistrantsRilConnectionChanged(rilVer);
        if (rilVer != -1) {
            if (mInstanceId != null) {
                riljLog("Enable simultaneous data/voice on Multi-SIM");
                invokeOemRilRequestSprd((byte) 3, (byte) 1, null);
            } else {
                riljLog("Set data subscription to allow data in either SIM slot when using single SIM mode");
                setDataAllowed(true, null);
            }
        }
    }

    @Override
    public void setGsmBroadcastConfig(SmsBroadcastConfigInfo[] config, Message response) {
        /* TODO: Anyway to remove the predefined list?
         * It looks like that the modem can only support 20 different message identifier at a time.
         * The observed problem here is that on Android 7 and above the ids
         * set easily exceed 20 and the modem seem to have problems dealing with
         * the extra like being unable to show the extra IDs when queried.
         *
         * Based on the first few tests, a small contribution on the elevated
         * power consumption is observed specially on 3G, 2G seems to be unaffected.
	 *
         * The RIL also does not seem to unset the IDs when disabled from
         * Android which means once an ID is enabled, it can't be disabled
         * from the RIL (or through Android).
         * One way to unset is by issuing an AT command from elsewhere.
         *
         * This approach limits the IDs to set by matching the range to a predefined list of IDs.
         *
	 * 4352,4353,4354,4356,4370,4371,4372,4373,4374,4375,4376,4377,4378,4379,4383
         */

        ArrayList<Integer> parcel = new ArrayList<Integer>();
        int numOfConfig = config.length;
        int[] id = new int[]{4352,4353,4354,4356,4370,4371,4372,4373,4374,4375,4376,4377,4378,4379,4383};

        if (RILJ_LOGD) {
            riljLog("setGsmBroadcastConfig: called with " + numOfConfig + " configs:");

            for (int i = 0; i < numOfConfig; i++)
                riljLog(config[i].toString());

            riljLog("setGsmBroadcastConfig: filtering ranges: ");
        }

        for(int i = 0; i < numOfConfig; i++) {
            int toId = config[i].getToServiceId();
            int lastGoodFrom = config[i].getFromServiceId();
            int lastGoodTo = -1;
            int fromCodeScheme = config[i].getFromCodeScheme();
            int toCodeScheme = config[i].getToCodeScheme();
            boolean isSelected = config[i].isSelected();

            if ((lastGoodFrom > id[id.length - 1]) || (toId < id[0])) {
                if (RILJ_LOGD)
                    riljLog("Config skipped, not supported: "+ config[i].toString());
                continue;
            }

            if (toId > id[id.length - 1])
                toId = id[id.length - 1];

            for( int j = lastGoodFrom; j <= toId ; j++) {
                int indx = Arrays.binarySearch(id, j);
                if (((indx < id.length) && (indx >= 0)) && id[indx] == j) {
                    if (lastGoodTo == -1)
                        lastGoodFrom = j;
                    lastGoodTo = j;

                    if (j < toId)
                        continue;
                }

                if (lastGoodTo == -1)
                    continue;

                if (RILJ_LOGD) {
                    riljLog(
                        "SmsBroadcastConfigInfo: Id [" +
                        lastGoodFrom + ',' + lastGoodTo + "] Code [" +
                        fromCodeScheme + ',' + toCodeScheme + "] " +
                        (isSelected ? "ENABLED" : "DISABLED")
                    );

                }

                parcel.add(lastGoodFrom);
                parcel.add(lastGoodTo);
                parcel.add(fromCodeScheme);
                parcel.add(toCodeScheme);
                parcel.add(isSelected ? 1 : 0);

                lastGoodTo = -1;
            }
        }


        if (parcel.size() == 0) {
            if (RILJ_LOGD)
                riljLog("setGsmBroadcastConfig: but no configs seem to be valid.");

            AsyncResult.forMessage(response, null, new CommandException(
                    CommandException.Error.GENERIC_FAILURE));
            response.sendToTarget();
        } else {
            int totalNumOfConfig = parcel.size();
            RILRequest rr = RILRequest.obtain(RIL_REQUEST_GSM_SET_BROADCAST_CONFIG, response);

            if (RILJ_LOGD)
                riljLog(rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " with " + (totalNumOfConfig / 5) + " configs.");

            rr.mParcel.writeInt(totalNumOfConfig / 5);

            for(int i=0; i < totalNumOfConfig ; i++)
                rr.mParcel.writeInt(parcel.get(i));

            send(rr);
        }
    }

    @Override
    protected Object
    responseCallList(Parcel p) {
        int num;
        int voiceSettings;
        ArrayList<DriverCall> response;
        DriverCall dc;

        num = p.readInt();
        response = new ArrayList<DriverCall>(num);

        if (RILJ_LOGV) {
            riljLog("responseCallList: num=" + num +
                    " mEmergencyCallbackModeRegistrant=" + mEmergencyCallbackModeRegistrant +
                    " mTestingEmergencyCall=" + mTestingEmergencyCall.get());
        }
        for (int i = 0 ; i < num ; i++) {
            dc = new DriverCall();

            dc.state = DriverCall.stateFromCLCC(p.readInt());
            // & 0xff to truncate to 1 byte added for us, not in RIL.java
            dc.index = p.readInt() & 0xff;
            dc.TOA = p.readInt();
            dc.isMpty = (0 != p.readInt());
            dc.isMT = (0 != p.readInt());
            dc.als = p.readInt();
            voiceSettings = p.readInt();
            dc.isVoice = (0 != voiceSettings);
            boolean isVideo = (0 != p.readInt());
            int call_type = p.readInt();            // Samsung CallDetails
            int call_domain = p.readInt();          // Samsung CallDetails
            String csv = p.readString();            // Samsung CallDetails
            dc.isVoicePrivacy = (0 != p.readInt());
            dc.number = p.readString();
            int np = p.readInt();
            dc.numberPresentation = DriverCall.presentationFromCLIP(np);
            dc.name = p.readString();
            dc.namePresentation = p.readInt();
            int uusInfoPresent = p.readInt();
            if (uusInfoPresent == 1) {
                dc.uusInfo = new UUSInfo();
                dc.uusInfo.setType(p.readInt());
                dc.uusInfo.setDcs(p.readInt());
                byte[] userData = p.createByteArray();
                dc.uusInfo.setUserData(userData);
                riljLogv(String.format("Incoming UUS : type=%d, dcs=%d, length=%d",
                                dc.uusInfo.getType(), dc.uusInfo.getDcs(),
                                dc.uusInfo.getUserData().length));
                riljLogv("Incoming UUS : data (string)="
                        + new String(dc.uusInfo.getUserData()));
                riljLogv("Incoming UUS : data (hex): "
                        + IccUtils.bytesToHexString(dc.uusInfo.getUserData()));
            } else {
                riljLogv("Incoming UUS : NOT present!");
            }

            // Make sure there's a leading + on addresses with a TOA of 145
            dc.number = PhoneNumberUtils.stringFromStringAndTOA(dc.number, dc.TOA);

            response.add(dc);

            if (dc.isVoicePrivacy) {
                mVoicePrivacyOnRegistrants.notifyRegistrants();
                riljLog("InCall VoicePrivacy is enabled");
            } else {
                mVoicePrivacyOffRegistrants.notifyRegistrants();
                riljLog("InCall VoicePrivacy is disabled");
            }
        }

        Collections.sort(response);

        if ((num == 0) && mTestingEmergencyCall.getAndSet(false)) {
            if (mEmergencyCallbackModeRegistrant != null) {
                riljLog("responseCallList: call ended, testing emergency call," +
                            " notify ECM Registrants");
                mEmergencyCallbackModeRegistrant.notifyRegistrant();
            }
        }

        return response;
    }

    private void invokeOemRilRequestSprd(byte key, byte value, Message response) {
        invokeOemRilRequestRaw(new byte[] { 'S', 'P', 'R', 'D', key, value }, response);
    }

}

