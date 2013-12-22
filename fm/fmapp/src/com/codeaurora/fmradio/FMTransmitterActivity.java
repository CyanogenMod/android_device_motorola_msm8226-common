/*
 * Copyright (c) 2009-2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.codeaurora.fmradio;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemClock;
import android.media.AudioManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView; //import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.text.TextUtils; //import android.app.ProgressDialog;

import java.lang.ref.WeakReference; //import java.text.DecimalFormat;
import java.util.HashMap;
import java.util.ArrayList;

import com.codeaurora.utils.FrequencyPicker;
import com.codeaurora.utils.FrequencyPickerDialog;
import android.content.ServiceConnection;

import qcom.fmradio.FmConfig;
import android.os.ServiceManager;

public class FMTransmitterActivity extends Activity {
        public static final String LOGTAG = "FMTransmitterActivity";

        /* menu Identifiers */
        private static final int MENU_SCAN_START = Menu.FIRST + 1;
        private static final int MENU_SCAN_STOP = Menu.FIRST + 2;
        private static final int MENU_SETTINGS = Menu.FIRST + 3;

        /* Dialog Identifiers */
        private static final int DIALOG_PRESET_LIST_AUTO_SET = 1;
        private static final int DIALOG_PICK_FREQUENCY = 2;
        private static final int DIALOG_PRESET_OPTIONS = 3;
        private static final int DIALOG_PROGRESS_PROGRESS = 5;
        private static final int DIALOG_CMD_FAILED_HDMI_ON = 14;
        /* Activity Return ResultIdentifiers */
        private static final int ACTIVITY_RESULT_SETTINGS = 1;

        private static final int MAX_PRESETS = 6;
        private static IFMTransmitterService mService = null;
        private static FmSharedPreferences mPrefs;

        /* Button Resources */
        private ImageButton mOnOffButton;
        /* 6 Preset Buttons */
        private Button[] mPresetButtons = { null, null, null, null, null, null };
        private int[] mPresetFrequencies = { 0, 0, 0, 0, 0, 0 };
        // private ImageButton mSearchButton;
        private ImageView mForwardButton;
        private ImageView mBackButton;

        /* row in the station info layout */
        private TextView mTransmitStaticMsgTV;
        private TextView mTuneStationFrequencyTV;

        /* Bottom row in the station info layout */
        private TextView mRadioTextTV;

        /* Current Status Indicators */
        private static boolean mIsSearching = false;
        private static boolean mSearchingResultStatus = false;


        private boolean mInternalAntennaAvailable = false;

        private Animation mAnimation = null;
        private ScrollerText mRadioTextScroller = null;

        private static int mTunedFrequency = 0;;
        private int mPresetButtonIndex = -1;
        private String mMetaData = null;
        private String mPSData = null;

        /* Radio Vars */
        final Handler mHandler = new Handler();
        private final Handler enableRadioHandler = new Handler();
        private final Handler disableRadioHandler = new Handler();

        /* Search Progress Dialog */
        private ProgressDialog mProgressDialog = null;

        private LoadedDataAndState SavedDataAndState = null;

        /** Called when the activity is first created. */
        @Override
        public void onCreate(Bundle savedInstanceState) {

                super.onCreate(savedInstanceState);
                mPrefs = new FmSharedPreferences(this);
                Log.d(LOGTAG, "onCreate - Height : "
                                + getWindowManager().getDefaultDisplay().getHeight()
                                + " - Width  : "
                                + getWindowManager().getDefaultDisplay().getWidth());
                setContentView(R.layout.fmtransmitter);
                SavedDataAndState = (LoadedDataAndState)getLastNonConfigurationInstance();
                mAnimation = AnimationUtils.loadAnimation(this, R.anim.preset_select);

                mOnOffButton = (ImageButton) findViewById(R.id.btn_onoff);
                if (mOnOffButton != null) {
                   mOnOffButton.setOnClickListener(mTurnOnOffClickListener);
                }

                mForwardButton = (ImageView) findViewById(R.id.btn_forward);
                if (mForwardButton != null) {
                   mForwardButton.setOnClickListener(mForwardClickListener);
                }

                mBackButton = (ImageView) findViewById(R.id.btn_back);
                if (mBackButton != null) {
                   mBackButton.setOnClickListener(mBackClickListener);
                }

                /* Preset Buttons */
                mPresetButtons[0] = (Button) findViewById(R.id.presets_button_1);
                mPresetButtons[1] = (Button) findViewById(R.id.presets_button_2);
                mPresetButtons[2] = (Button) findViewById(R.id.presets_button_3);
                mPresetButtons[3] = (Button) findViewById(R.id.presets_button_4);
                mPresetButtons[4] = (Button) findViewById(R.id.presets_button_5);
                mPresetButtons[5] = (Button) findViewById(R.id.presets_button_6);
                for (int nButton = 0; nButton < MAX_PRESETS; nButton++) {
                    if (mPresetButtons[nButton] != null) {
                       mPresetButtons[nButton]
                               .setOnClickListener(mPresetButtonClickListener);
                       mPresetButtons[nButton]
                                .setOnLongClickListener(mPresetButtonOnLongClickListener);
                    }
                }

                mTransmitStaticMsgTV = (TextView) findViewById(R.id.transmit_msg_tv);
                String str = getString(R.string.transmit_msg_string);
                if(null != mPSData ) {
                    str = mPSData.concat("\n").concat(str);
                }
                if (mTransmitStaticMsgTV != null) {
                   mTransmitStaticMsgTV.setText(str);
                }

                mTuneStationFrequencyTV = (TextView) findViewById(R.id.prog_frequency_tv);
                if (mTuneStationFrequencyTV != null) {
                        mTuneStationFrequencyTV
                                        .setOnLongClickListener(mFrequencyViewClickListener);
                }

                mRadioTextTV = (TextView) findViewById(R.id.radio_text_tv);

                if ((mRadioTextScroller == null) && (mRadioTextTV != null)) {
                        mRadioTextScroller = new ScrollerText(mRadioTextTV);
                }

                enableRadioOnOffUI(false);
                //HDMI and FM concurrecny is not supported.
                if (isHdmiOn())
                {
                    showDialog(DIALOG_CMD_FAILED_HDMI_ON);
                }
                else {
                    if (false == bindToService(this, osc))
                    {
                        Log.d(LOGTAG, "onCreate: Failed to Start Service");
                    } else
                    {
                        Log.d(LOGTAG, "onCreate: Start Service completed successfully");
                    }
                }
        }

        @Override
        public void onRestart() {
                Log.d(LOGTAG, "onRestart");
                super.onRestart();
        }

        @Override
        public void onStop() {
                Log.d(LOGTAG, "onStop");
                super.onStop();
        }

        @Override
        public void onStart() {
                super.onStart();
                Log.d(LOGTAG, "onStart");
                try {
                        if (mService != null) {
                                mService.registerCallbacks(mServiceCallbacks);
                        }
                } catch (RemoteException e) {
                        e.printStackTrace();
                }

        }

        @Override
        protected void onPause() {
                super.onPause();
                mRadioTextScroller.stopScroll();
                SavePreferences();
        }

        private static final String TX_PREF_LAST_TUNED_FREQUENCY = "last_tx_frequency";
        private static final String TX_PRESET_FREQUENCY = "tx_preset_freq_";
    private static final int DEFAULT_NO_FREQUENCY = 98100;

        public void SavePreferences() {
                Log.d(LOGTAG, "Save preferences ");
                SharedPreferences sp = getPreferences(Context.MODE_PRIVATE);
                SharedPreferences.Editor ed = sp.edit();

                ed.putInt(TX_PREF_LAST_TUNED_FREQUENCY, mTunedFrequency);

                for (int presetIndex = 0; presetIndex < MAX_PRESETS; presetIndex++) {
                        ed.putInt(TX_PRESET_FREQUENCY + presetIndex,
                                        mPresetFrequencies[presetIndex]);
                }
                ed.commit();
        }

        public void LoadPreferences() {
                SharedPreferences sp = getPreferences(Context.MODE_PRIVATE);
        mTunedFrequency = sp.getInt(TX_PREF_LAST_TUNED_FREQUENCY, DEFAULT_NO_FREQUENCY);
                for (int presetIndex = 0; presetIndex < MAX_PRESETS; presetIndex++) {
                        mPresetFrequencies[presetIndex] =
                                sp.getInt(TX_PRESET_FREQUENCY + presetIndex, DEFAULT_NO_FREQUENCY);
                }
        }

        @Override
        public void onResume() {
                super.onResume();
                LoadPreferences();
                try {
                    if((null != mService )&&mService.isFmOn()) {
                        mHandler.post(mUpdateRadioText);
                    }
                } catch(RemoteException ex){
                    Log.d(LOGTAG,"expection for service");
                }
                mHandler.post(mUpdateRadioText);
                updateStationInfoToUI();
                enableRadioOnOffUI();
        }

        @Override
        public void onDestroy() {
                unbindFromService(this);
                mService = null;

                Log.d(LOGTAG, "onDestroy: unbindFromService completed");
                super.onDestroy();
        }

        private class LoadedDataAndState {
                public LoadedDataAndState(){};
                public boolean onOrOff;
        }

        @Override
        public Object onRetainNonConfigurationInstance() {
                final LoadedDataAndState data = new LoadedDataAndState();
                if(mService != null) {
                   try {
                       data.onOrOff = mService.isFmOn();
                   } catch(RemoteException e) {
                       data.onOrOff = false;
                       e.printStackTrace();
                   }
                } else {
                   data.onOrOff = false;
                }
                return data;
        }

        @Override
        public boolean onCreateOptionsMenu(Menu menu) {
                super.onCreateOptionsMenu(menu);
                MenuItem item;
                boolean radioOn = isFmOn();
                boolean searchActive = isSearchActive();

                item = menu.add(0, MENU_SCAN_START, 0, R.string.menu_scan_for_preset)
                                .setIcon(R.drawable.ic_btn_search);
                if (item != null) {
                        item.setVisible(!searchActive && radioOn);
                }
                item = menu.add(0, MENU_SCAN_STOP, 0, R.string.menu_scan_stop).setIcon(
                                R.drawable.ic_btn_search);
                if (item != null) {
                        item.setVisible(searchActive && radioOn);
                }

                /* Settings can be active */
                item = menu.add(0, MENU_SETTINGS, 0, R.string.menu_settings).setIcon(
                                android.R.drawable.ic_menu_preferences);
                return true;
        }

        @Override
        public boolean onPrepareOptionsMenu(Menu menu) {
                super.onPrepareOptionsMenu(menu);

                MenuItem item;
                boolean radioOn = isFmOn();
                boolean searchActive = isSearchActive();

                item = menu.findItem(MENU_SCAN_START);
                if (item != null) {
                        item.setVisible(!searchActive && radioOn);
                }
                item = menu.findItem(MENU_SCAN_STOP);
                if (item != null) {
                        item.setVisible(searchActive && radioOn);
                }
                return true;
        }

        @Override
        public boolean onOptionsItemSelected(MenuItem item) {
                switch (item.getItemId()) {
                case MENU_SETTINGS:
                 Intent launchPreferencesIntent = new Intent().setClass(this,
                     Settings.class);
                        launchPreferencesIntent.putExtra(Settings.RX_MODE,false);
                        startActivityForResult(launchPreferencesIntent,
                        ACTIVITY_RESULT_SETTINGS);

                        // startActivity(launchPreferencesIntent);
                        return true;

                case MENU_SCAN_START:
                        showDialog(DIALOG_PRESET_LIST_AUTO_SET);
                        return true;

                case MENU_SCAN_STOP:
                        cancelSearch();
                        return true;

                default:
                        break;
                }
                return super.onOptionsItemSelected(item);
        }

        private boolean isHdmiOn() {
            //HDMI and FM concurrecny is not supported.
                try {
                    String hdmiUserOption = android.provider.Settings.System.getString(
                                            getContentResolver(), "HDMI_USEROPTION");
                }
                catch (Exception ex){
                    Log.d(LOGTAG,"Get HDMI open failed");
                }
                return false;
        }
        @Override
        protected Dialog onCreateDialog(int id) {
                switch (id) {
                case DIALOG_PRESET_LIST_AUTO_SET: {
                        return createPresetListAutoSelectWarnDlg(id);
                }
                case DIALOG_PICK_FREQUENCY: {
                        FmConfig fmConfig = FmSharedPreferences.getFMConfiguration();
                        return new FrequencyPickerDialog(this, fmConfig, mTunedFrequency,
                                        mFrequencyChangeListener);
                }
                case DIALOG_PROGRESS_PROGRESS: {
                        return createProgressDialog(id);
                }
                case DIALOG_PRESET_OPTIONS: {
                        return createPresetOptionsDlg(id);
                }
                default:
                        break;
                }
                return null;
        }

        @Override
        protected void onPrepareDialog(int id, Dialog dialog) {
                super.onPrepareDialog(id, dialog);
                switch (id) {
                case DIALOG_PICK_FREQUENCY: {
                        ((FrequencyPickerDialog) dialog).UpdateFrequency(mTunedFrequency);
                        break;
                }
                case DIALOG_PRESET_OPTIONS: {
                        AlertDialog alertDlg = ((AlertDialog) dialog);
                        if ((alertDlg != null) && (mPresetButtonIndex >= 0)
                                        && (mPresetButtonIndex <= MAX_PRESETS)) {
                                int frequency = mPresetFrequencies[mPresetButtonIndex];
                                alertDlg.setTitle(PresetStation.getFrequencyString(frequency));
                        }
                        break;
                }

                default:
                        break;
                }
        }

        @Override
        protected void onActivityResult(int requestCode, int resultCode, Intent data) {
                super.onActivityResult(requestCode, resultCode, data);
                Log.d(LOGTAG, "onActivityResult : requestCode -> " + requestCode);
                Log.d(LOGTAG, "onActivityResult : resultCode -> " + resultCode);
              if (requestCode ==ACTIVITY_RESULT_SETTINGS)
              {
                 if (resultCode == RESULT_OK)
                 {
                    /* */
                    if (data != null)
                    {
                       String action = data.getAction();
                       if (action != null)
                       {
                          if (action.equals(Settings.RESTORE_FACTORY_DEFAULT_ACTION))
                          {
                             RestoreDefaults();
                             enableRadioOnOffUI();
                             tuneRadio(mTunedFrequency);
                          }
                       }
                    }
                 } //if ACTIVITY_RESULT_SETTINGS
              }//if (resultCode == RESULT_OK)
        }




        /**
         * @return true if a internal antenna is available.
         *
         */
        boolean isInternalAntennaAvailable() {
                return mInternalAntennaAvailable;
        }

        private Dialog createPresetOptionsDlg(int id) {
                if ((mPresetButtonIndex >= 0) && (mPresetButtonIndex <= MAX_PRESETS)) {
                        int frequency = mPresetFrequencies[mPresetButtonIndex];
                        AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(this);
                        dlgBuilder.setTitle(PresetStation.getFrequencyString(frequency));
                        ArrayList<String> arrayList = new ArrayList<String>();
                        arrayList.add(getResources().getString(R.string.preset_tune));
                        arrayList.add(getResources().getString(R.string.preset_replace));
                        arrayList.add(getResources().getString(R.string.preset_delete));
                        dlgBuilder.setCancelable(true);
                        dlgBuilder
                                        .setOnCancelListener(new DialogInterface.OnCancelListener() {
                                                public void onCancel(DialogInterface dialog) {
                                                        mPresetButtonIndex = -1;
                                                        removeDialog(DIALOG_PRESET_OPTIONS);
                                                }
                                        });
                        String[] items = new String[arrayList.size()];
                        arrayList.toArray(items);
                        dlgBuilder.setItems(items, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int item) {
                                        if ((mPresetButtonIndex >= 0)
                                                        && (mPresetButtonIndex <= MAX_PRESETS)) {
                                                int frequency = mPresetFrequencies[mPresetButtonIndex];
                                                switch (item) {
                                                case 0: {
                                                        // Tune to this station
                                                        mPresetButtonIndex = -1;
                                                        tuneRadio(frequency);
                                                        break;
                                                }
                                                case 1: {
                                                        // Replace preset Station with currently tuned
                                                        // station
                                                        Log.d(LOGTAG, "Replace station - " + frequency
                                                                        + " with " + mTunedFrequency);
                                                        mPresetFrequencies[mPresetButtonIndex] = mTunedFrequency;
                                                        mPresetButtonIndex = -1;
                                                        setupPresetLayout();
                                                        SavePreferences();
                                                        break;
                                                }
                                                case 2: {
                                                        // Delete
                                                        mPresetFrequencies[mPresetButtonIndex] = 0;
                                                        mPresetButtonIndex = -1;
                                                        setupPresetLayout();
                                                        SavePreferences();
                                                        break;
                                                }
                                                default: {
                                                        // Should not happen
                                                        mPresetButtonIndex = -1;
                                                        break;
                                                }
                                                }// switch item
                                        }// if(mPresetButtonStation != null)
                                        removeDialog(DIALOG_PRESET_OPTIONS);
                                }// onClick
                        });
                        return dlgBuilder.create();
                }
                return null;
        }

        private Dialog createProgressDialog(int id) {
                String msgStr = "";
                String titleStr = "";
                boolean bSearchActive = false;

                if (isSearchActive()) {
                        msgStr = getString(R.string.msg_weak_searching);
                        titleStr = getString(R.string.msg_searching_title);
                        bSearchActive = true;
                }

                if (bSearchActive) {
                        mProgressDialog = new ProgressDialog(FMTransmitterActivity.this);
                        if (mProgressDialog != null) {
                           mProgressDialog.setTitle(titleStr);
                           mProgressDialog.setMessage(msgStr);
                           mProgressDialog.setIcon(R.drawable.ic_launcher_fm_tx);
                           mProgressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                           mProgressDialog.setButton(DialogInterface.BUTTON_POSITIVE,
                                        getText(R.string.button_text_stop),
                                        new DialogInterface.OnClickListener() {
                                                public void onClick(DialogInterface dialog,
                                                                int whichButton) {
                                                        cancelSearch();
                                                }
                                        });
                           mProgressDialog
                                        .setOnCancelListener(new DialogInterface.OnCancelListener() {
                                                public void onCancel(DialogInterface dialog) {
                                                        cancelSearch();
                                                }
                                        });
                        }

                        Message msg = new Message();
                        msg.what = TIMEOUT_PROGRESS_DLG;
                        mSearchProgressHandler.sendMessageDelayed(msg, SHOWBUSY_TIMEOUT);
                }
                return mProgressDialog;
        }

        private Dialog createPresetListAutoSelectWarnDlg(int id) {
                AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(this);
                dlgBuilder.setIcon(R.drawable.alert_dialog_icon).setTitle(
                                R.string.presetlist_autoselect_title);
                dlgBuilder.setMessage(getString(R.string.fmtx_autoselect_name));

                dlgBuilder.setPositiveButton(R.string.alert_dialog_ok,
                                new DialogInterface.OnClickListener() {
                                        public void onClick(DialogInterface dialog, int whichButton) {
                                                /*
                                                 * Since the presets will be changed, reset the page
                                                 * number
                                                 */
                                                initiateSearchList();
                                                setupPresetLayout();
                                                SavePreferences();
                                                removeDialog(DIALOG_PRESET_LIST_AUTO_SET);
                                        }
                                });

                dlgBuilder.setNegativeButton(R.string.alert_dialog_cancel,
                                new DialogInterface.OnClickListener() {
                                        public void onClick(DialogInterface dialog, int whichButton) {
                                                removeDialog(DIALOG_PRESET_LIST_AUTO_SET);
                                        }
                                });
                return (dlgBuilder.create());
        }

        private void RestoreDefaults() {
                for (int index = 0; index < MAX_PRESETS; index++) {
                        mPresetFrequencies[index] = FmSharedPreferences.getLowerLimit();
                }
                mTunedFrequency = FmSharedPreferences.getLowerLimit();
                SavePreferences();
        }

        private View.OnLongClickListener mFrequencyViewClickListener = new View.OnLongClickListener() {
                public boolean onLongClick(View v) {
                        showDialog(DIALOG_PICK_FREQUENCY);
                        return true;
                }
        };

        private View.OnClickListener mForwardClickListener = new View.OnClickListener() {
                public void onClick(View v) {
                        int frequency = FmSharedPreferences
                                        .getNextTuneFrequency(mTunedFrequency);
                        Log.d(LOGTAG, "Tune Up: to " + frequency);
                        tuneRadio(frequency);
                }
        };

        private View.OnClickListener mBackClickListener = new View.OnClickListener() {
                public void onClick(View v) {
                        int frequency = FmSharedPreferences
                                        .getPrevTuneFrequency(mTunedFrequency);
                        Log.d(LOGTAG, "Tune Down: to " + frequency);
                        tuneRadio(frequency);
                }
        };

        private View.OnClickListener mPresetButtonClickListener = new View.OnClickListener() {
                public void onClick(View view) {
                        Integer indexInteger = (Integer) view.getTag();
                        int index = indexInteger.intValue();
                        if ((index >= 0) && (index <= MAX_PRESETS)) {
                                Log.d(LOGTAG, "station - " + index);
                                if(mPresetFrequencies[index] != 0)
                                {
                                        mTunedFrequency = mPresetFrequencies[index];
                                        tuneRadio(mTunedFrequency);
                                        view.startAnimation(mAnimation);
                                }
                        }
                }
        };

        private View.OnLongClickListener mPresetButtonOnLongClickListener = new View.OnLongClickListener() {
                public boolean onLongClick(View view) {
                        Integer indexInteger = (Integer) view.getTag();
                        int index = indexInteger.intValue();
                        if ((index >= 0) && (index <= MAX_PRESETS)) {
                                int frequency = mPresetFrequencies[index];
                                mPresetButtonIndex = index;
                                if (frequency > 0) {
                                        showDialog(DIALOG_PRESET_OPTIONS);
                                } else {
                                        mPresetFrequencies[index] = mTunedFrequency;
                                        view.startAnimation(mAnimation);
                                        setupPresetLayout();
                                        SavePreferences();
                                }
                        }
                        return true;
                }
        };

        final FrequencyPickerDialog.OnFrequencySetListener mFrequencyChangeListener = new FrequencyPickerDialog.OnFrequencySetListener() {
                public void onFrequencySet(FrequencyPicker view, int frequency) {
                        Log.d(LOGTAG, "mFrequencyChangeListener: onFrequencyChanged to : "
                                        + frequency);
                        tuneRadio(frequency);
                }
        };

        private View.OnClickListener mTurnOnOffClickListener = new View.OnClickListener() {
                public void onClick(View v) {

                        if (isFmOn()) {
                                enableRadioHandler.removeCallbacks(enableRadioTask);
                                disableRadioHandler.removeCallbacks(disableRadioTask);
                                disableRadioHandler.postDelayed(disableRadioTask, 0);
                        } else {
                                enableRadioHandler.removeCallbacks(enableRadioTask);
                                disableRadioHandler.removeCallbacks(disableRadioTask);
                                enableRadioHandler.postDelayed(enableRadioTask, 0);
                        }
                        setTurnOnOffButtonImage();
                }
        };

        private void setTurnOnOffButtonImage() {
                if (isFmOn() == true) {
                        mOnOffButton.setImageResource(R.drawable.ic_btn_onoff);
                } else {
                        /* Find a icon to indicate off */
                        mOnOffButton.setImageResource(R.drawable.ic_btn_onoff);
                }
        }

        private void enableRadioOnOffButton() {
                if(mOnOffButton != null) {
                   mOnOffButton.setEnabled(true);
                   mOnOffButton.setClickable(true);
                }
        }

        private void disableRadioOnOffButton() {
                if(mOnOffButton != null) {
                   mOnOffButton.setEnabled(false);
                   mOnOffButton.setClickable(false);
                }
        }
        private void setHsPluggedInMsg() {
                if (mRadioTextTV != null) {
                    mRadioTextTV.setVisibility(View.VISIBLE);
                    mRadioTextTV.setText(getString(R.string.msg_headsetpluggedin));
                }
        }
        private Runnable enableRadioTask = new Runnable() {
                public void run() {
                     enableRadio();
                }
        };

        private Runnable disableRadioTask = new Runnable() {
                public void run() {
                     disableRadio();
                }
        };

        private void enableRadio() {
                mIsSearching = false;
                disableRadioOnOffButton();
                if (mService != null) {
                        try {
                                if(mService.isHeadsetPlugged()) {
                                   setHsPluggedInMsg();
                                   enableRadioOnOffButton();
                                } else if(mService.isCallActive()) {
                                   enableRadioOnOffButton();
                                } else if(!mService.fmOn()) {
                                   enableRadioOnOffButton();
                                }
                        } catch (RemoteException e) {
                                enableRadioOnOffButton();
                                e.printStackTrace();
                        }
                }

        }

        private void disableRadio() {
                enableRadioOnOffUI(false);
                cancelSearch();
                if (mService != null) {
                        try {
                               if(!mService.fmOff()) {
                                  enableRadioOnOffButton();
                               }
                        } catch (RemoteException e) {
                                enableRadioOnOffButton();
                                e.printStackTrace();
                        }
                }
        }

        public static void fmConfigure() {
                if (mService != null) {
                        try {
                                mService.fmReconfigure();
                        } catch (RemoteException e) {
                                e.printStackTrace();
                        }
                }
        }

        private void enableRadioOnOffUI() {
                boolean bEnable = true;
                //decide enable or disable UI based on
                //Tx Service status.This is valid only
                // when search is not in progress. When
                // search is in progress UI should be active
                if( false == isSearchActive() ) {
                    bEnable = isFmOn();
                }
                /* Disable if no antenna/headset is available */
                if (!readInternalAntennaAvailable()) {
                    Log.d(LOGTAG,"finding internal antenna avialable as false");
                        bEnable = false;
                }
                enableRadioOnOffUI(bEnable);
        }

        private void enableRadioOnOffUI(boolean bEnable) {
                if (bEnable) {
                        if (mTuneStationFrequencyTV != null) {
                           mTuneStationFrequencyTV
                                        .setOnLongClickListener(mFrequencyViewClickListener);
                        }
                        if (mRadioTextScroller != null) {
                           mRadioTextScroller.startScroll();
                        }
                } else {
                        if (mTuneStationFrequencyTV != null) {
                           mTuneStationFrequencyTV.setOnLongClickListener(null);
                        }
                        if (mRadioTextScroller != null) {
                           mRadioTextScroller.stopScroll();
                        }
                }

                if (mForwardButton != null) {
                   mForwardButton.setVisibility(((bEnable == true) ? View.VISIBLE
                                : View.INVISIBLE));
                }
                if (mBackButton != null) {
                   mBackButton.setVisibility(((bEnable == true) ? View.VISIBLE
                                : View.INVISIBLE));
                }
                if (mTransmitStaticMsgTV != null) {
                   mTransmitStaticMsgTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                : View.INVISIBLE));
                }
                if (mTuneStationFrequencyTV != null) {
                   mTuneStationFrequencyTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                : View.INVISIBLE));
                }
                if (mRadioTextTV != null) {
                   mRadioTextTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                : View.INVISIBLE));
                }

                try {
                    if( null != mService) {
                        if ( mService.isHeadsetPlugged()) {
                            Log.d(LOGTAG,"headset plugged in");
                            if (mRadioTextTV != null) {
                               mRadioTextTV.setVisibility(View.VISIBLE);
                               mRadioTextTV.setText(getString(R.string.msg_headsetpluggedin));
                            }
                            if (mOnOffButton != null) {
                               mOnOffButton.setEnabled(false);
                            }
                        } else if (mService.isCallActive()) {
                            Log.d(LOGTAG,"call active");
                            if (mRadioTextTV != null) {
                               mRadioTextTV.setVisibility(View.VISIBLE);
                               mRadioTextTV.setText(getString(R.string.msg_callactive));
                            }
                            if (mOnOffButton != null) {
                               mOnOffButton.setEnabled(false);
                            }
                        } else {
                            if (mRadioTextTV != null) {
                               mRadioTextTV.setText("");
                            }
                            if (mOnOffButton != null) {
                               mOnOffButton.setEnabled(true);
                            }
                        }
                    } else {
                        Log.d(LOGTAG,"Service null");
                        if (mRadioTextTV != null) {
                           mRadioTextTV.setText("");
                        }
                        if (mOnOffButton != null) {
                           mOnOffButton.setEnabled(true);
                        }
                    }
                } catch (RemoteException e) {
                    e.printStackTrace();
                }

                for (int nButton = 0; nButton < MAX_PRESETS; nButton++) {
                     if (mPresetButtons[nButton] != null) {
                        mPresetButtons[nButton].setEnabled(bEnable);
                     }
                }
        }

        private void updateSearchProgress() {
                boolean searchActive = isSearchActive();
                if (searchActive) {
                        synchronized (this) {
                                if (mProgressDialog == null) {
                                        showDialog(DIALOG_PROGRESS_PROGRESS);
                                } else {
                                        Message msg = new Message();
                                        msg.what = UPDATE_PROGRESS_DLG;
                                        mSearchProgressHandler.sendMessage(msg);
                                }
                        }
                } else {
                        Message msg = new Message();
                        msg.what = END_PROGRESS_DLG;
                        mSearchProgressHandler.sendMessage(msg);
                }
        }

        private void resetSearchProgress() {
                Message msg = new Message();
                msg.what = END_PROGRESS_DLG;
                mSearchProgressHandler.sendMessage(msg);
        }

        private void setupPresetLayout() {
                /*
                 * For every station, save the station as a tag and update the display
                 * on the preset Button.
                 */
                for (int buttonIndex = 0; (buttonIndex < MAX_PRESETS); buttonIndex++) {
                        if (mPresetButtons[buttonIndex] != null) {
                                String display = "";
                                int frequency = mPresetFrequencies[buttonIndex];
                                if (frequency != 0) {
                                        display = PresetStation.getFrequencyString(frequency);
                                }
                                mPresetButtons[buttonIndex].setText(display);
                                mPresetButtons[buttonIndex].setTag(new Integer(buttonIndex));
                        }
                }
        }

        private void updateStationInfoToUI() {
                mTuneStationFrequencyTV.setText(PresetStation.getFrequencyString(mTunedFrequency));
                mRadioTextTV.setText("");
                setupPresetLayout();
        }

        private boolean isFmOn() {
                boolean bOn = false;
                if (mService != null) {
                        try {
                                bOn = mService.isFmOn();
                        } catch (RemoteException e) {
                                e.printStackTrace();
                        }
                }
                return (bOn);
        }

        private boolean isSearchActive() {
                return (mIsSearching);
        }

        public static int getCurrentTunedFrequency() {
                return mTunedFrequency;
        }

        private void cancelSearch() {
                synchronized (this) {
                        if (mService != null) {
                                try {
                                        if (mIsSearching == true) {
                                                if (true == mService.cancelSearch()) {
                                                }
                                        }
                                } catch (RemoteException e) {
                                        e.printStackTrace();
                                }
                        }
                }
                updateSearchProgress();
        }

        /** get Weakest Stations */
        private void initiateSearchList() {
                synchronized (this) {
                        mIsSearching = true;
                        if (mService != null) {
                                try {
                                        mSearchingResultStatus = false;
                                        mIsSearching = mService.searchWeakStationList(MAX_PRESETS);
                                } catch (RemoteException e) {
                                        e.printStackTrace();
                                }
                                updateSearchProgress();
                        }
                }
        }

        /** get Internal Antenna Available mode Stations */
        private boolean readInternalAntennaAvailable() {
                mInternalAntennaAvailable = false;
                if (mService != null) {
                        try {
                                mInternalAntennaAvailable = mService
                                                .isInternalAntennaAvailable();
                        } catch (RemoteException e) {
                                e.printStackTrace();
                        }
                }
                Log.e(LOGTAG, "readInternalAntennaAvailable: internalAntenna : "
                                + mInternalAntennaAvailable);
                return mInternalAntennaAvailable;
        }

        private static final int UPDATE_PROGRESS_DLG = 1;
        private static final int END_PROGRESS_DLG = 2;
        private static final int TIMEOUT_PROGRESS_DLG = 3;
        private static final int SHOWBUSY_TIMEOUT = 300000;
        private Handler mSearchProgressHandler = new Handler() {
                public void handleMessage(Message msg) {
                        if (msg.what == UPDATE_PROGRESS_DLG) {
                                // Log.d(LOGTAG, "mTimeoutHandler: UPDATE_PROGRESS_DLG");
                                if (mProgressDialog != null) {
                                        mTuneStationFrequencyTV.setText(PresetStation.getFrequencyString(mTunedFrequency));
                                        String titleStr = getString( R.string.msg_search_title,
                                                                             PresetStation.getFrequencyString(mTunedFrequency));

                                        mProgressDialog.setTitle(titleStr);
                                }
                        } else if (msg.what == END_PROGRESS_DLG) {
                                // Log.d(LOGTAG, "mTimeoutHandler: END_PROGRESS_DLG");
                                mSearchProgressHandler.removeMessages(END_PROGRESS_DLG);
                                mSearchProgressHandler.removeMessages(UPDATE_PROGRESS_DLG);
                                mSearchProgressHandler.removeMessages(TIMEOUT_PROGRESS_DLG);
                                removeDialog(DIALOG_PROGRESS_PROGRESS);
                                mProgressDialog = null;
                        } else if (msg.what == TIMEOUT_PROGRESS_DLG) {
                                cancelSearch();
                        }
                }
        };

        private void tuneRadio(int frequency) {
                if (mService != null) {
                        try {
                                // Set Tune Frequency
                                mService.tune(frequency);
                                updateStationInfoToUI();
                        } catch (RemoteException e) {
                                e.printStackTrace();
                        }
                }
        }

        private void resetFMStationInfoUI() {
                mRadioTextTV.setText("");
                mRadioTextScroller.stopScroll();
                updateStationInfoToUI();
        }


        final Runnable mUpdateStationInfo = new Runnable() {
                public void run() {
                        updateSearchProgress();
                        resetFMStationInfoUI();
                }
        };

        final Runnable mSearchListComplete = new Runnable() {
                public void run() {
                        Log.d(LOGTAG, "mSearchListComplete: ");
                        mIsSearching = false;

                        /* Now get the list */
                        if (mService != null) {
                          try {
                               if(mSearchingResultStatus) {
                                  int[] searchList = mService.getSearchList();
                                  if (searchList != null) {
                                     for (int station = 0; (station < searchList.length)
                                          && (station < MAX_PRESETS); station++) {
                                          Log.d(LOGTAG, "mSearchListComplete: [" + station
                                          + "] = " + searchList[station]);
                                         mPresetFrequencies[station] = searchList[station];
                                     }
                                  }
                               }
                               /* Restart FM into Transmit mode */
                               if(!mService.isHeadsetPlugged()) {
                                  mService.fmRestart();
                                  /* Tune to last tuned frequency */
                                  tuneRadio(mTunedFrequency);
                                  updateSearchProgress();
                                  resetFMStationInfoUI();
                                  setupPresetLayout();
                                  SavePreferences();
                               } else {
                                  updateSearchProgress();
                                  enableRadioHandler.removeCallbacks(enableRadioTask);
                                  disableRadioHandler.removeCallbacks(disableRadioTask);
                                  disableRadioHandler.postDelayed(disableRadioTask, 0);
                               }
                          } catch (RemoteException e) {
                               e.printStackTrace();
                          }
                       }
                }
        };



        final Runnable mUpdateRadioText = new Runnable() {
                public void run() {
                        String str = "";
                        if ((mService != null) && isFmOn()) {
                                try {
                                        /* Get Radio Text and update the display */
                                        str = mService.getRadioText();
                                        if(null != mMetaData) {
                                            Log.d(LOGTAG,"meta data is "+mMetaData);
                                            str = str.concat(mMetaData);
                                        } else {
                                            str = str.concat("...");
                                        }
                                        /* Update only if all the characters are printable */
                                        if (TextUtils.isGraphic(str))
                                        {
                                            Log.d(LOGTAG, "mUpdateRadioText: Updatable string: ["
                                                            + str + "]");
                                            mRadioTextTV.setText(str);
                                        }
                                        /* Rest the string to empty */
                                        else if (TextUtils.isEmpty(str)) {
                                                mRadioTextTV.setText("");
                                        } else {
                                            Log.d(LOGTAG, "RDS has non printable stuff");
                                            mRadioTextTV.setText("");
                                        }

                                        mRadioTextScroller.startScroll();
                                        String szRTStr = getString(R.string.transmit_msg_string);
                                        mPSData = mService.getPSData();
                                        if(null != mPSData ) {
                                            szRTStr = mPSData.concat("\n").concat(szRTStr);
                                        }
                                        else
                                            Log.d(LOGTAG, "mPSData is NULL");
                                        mTransmitStaticMsgTV.setText(szRTStr);
                                } catch (RemoteException e) {
                                        e.printStackTrace();
                                }
                        }
                }
        };

        private void DebugToasts(String str, int duration) {
                Toast.makeText(this, str, duration).show();
        }

        /**
         * This Handler will scroll the text view. On startScroll, the scrolling
         * starts after SCROLLER_START_DELAY_MS The Text View is scrolled left one
         * character after every SCROLLER_UPDATE_DELAY_MS When the entire text is
         * scrolled, the scrolling will restart after SCROLLER_RESTART_DELAY_MS
         */
        private static final class ScrollerText extends Handler {

                private static final byte SCROLLER_STOPPED = 0x51;
                private static final byte SCROLLER_STARTING = 0x52;
                private static final byte SCROLLER_RUNNING = 0x53;

                private static final int SCROLLER_MSG_START = 0xF1;
                private static final int SCROLLER_MSG_TICK = 0xF2;
                private static final int SCROLLER_MSG_RESTART = 0xF3;

                private static final int SCROLLER_START_DELAY_MS = 1000;
                private static final int SCROLLER_RESTART_DELAY_MS = 3000;
                private static final int SCROLLER_UPDATE_DELAY_MS = 200;

                private final WeakReference<TextView> mView;

                private byte mStatus = SCROLLER_STOPPED;
                String mOriginalString;
                int mStringlength = 0;
                int mIteration = 0;

                ScrollerText(TextView v) {
                        mView = new WeakReference<TextView>(v);
                }

                /**
                 * Scrolling Message Handler
                 */
                @Override
                public void handleMessage(Message msg) {
                        switch (msg.what) {
                        case SCROLLER_MSG_START:
                                mStatus = SCROLLER_RUNNING;
                                updateText();
                                break;
                        case SCROLLER_MSG_TICK:
                                updateText();
                                break;
                        case SCROLLER_MSG_RESTART:
                                if (mStatus == SCROLLER_RUNNING) {
                                        startScroll();
                                }
                                break;
                        }
                }

                /**
                 * Moves the text left by one character and posts a delayed message for
                 * next update after SCROLLER_UPDATE_DELAY_MS. If the entire string is
                 * scrolled, then it displays the entire string and waits for
                 * SCROLLER_RESTART_DELAY_MS for scrolling restart
                 */
                void updateText() {
                        if (mStatus != SCROLLER_RUNNING) {
                                return;
                        }

                        removeMessages(SCROLLER_MSG_TICK);

                        final TextView textView = mView.get();
                        if (textView != null) {
                                String szStr2 = "";
                                if (mStringlength > 0) {
                                        mIteration++;
                                        if (mIteration >= mStringlength) {
                                                mIteration = 0;
                                                sendEmptyMessageDelayed(SCROLLER_MSG_RESTART,
                                                                SCROLLER_RESTART_DELAY_MS);
                                        } else {
                                                sendEmptyMessageDelayed(SCROLLER_MSG_TICK,
                                                                SCROLLER_UPDATE_DELAY_MS);
                                        }
                                        // String szStr1 = mOriginalString.substring(0, mTick);
                                        szStr2 = mOriginalString.substring(mIteration);
                                }
                                // textView.setText(szStr2+"     "+szStr1);
                                textView.setText(szStr2);
                        }
                }

                /**
                 * Stops the scrolling The textView will be set to the original string.
                 */
                void stopScroll() {
                        mStatus = SCROLLER_STOPPED;
                        removeMessages(SCROLLER_MSG_TICK);
                        removeMessages(SCROLLER_MSG_RESTART);
                        removeMessages(SCROLLER_MSG_START);
                        resetScroll();
                }

                /**
                 * Resets the scroll to display the original string.
                 */
                private void resetScroll() {
                        mIteration = 0;
                        final TextView textView = mView.get();
                        if (textView != null) {
                                textView.setText(mOriginalString);
                        }
                }

                /**
                 * Starts the Scrolling of the TextView after a delay of
                 * SCROLLER_START_DELAY_MS Starts only if Length > 0
                 */
                void startScroll() {
                        final TextView textView = mView.get();
                        if (textView != null) {
                                mOriginalString = (String) textView.getText();
                                mStringlength = mOriginalString.length();
                                if (mStringlength > 0) {
                                        mStatus = SCROLLER_STARTING;
                                        sendEmptyMessageDelayed(SCROLLER_MSG_START,
                                                        SCROLLER_START_DELAY_MS);
                                }
                        }
                }
        }

        public static IFMTransmitterService sService = null;
        private static HashMap<Context, ServiceBinder> sConnectionMap = new HashMap<Context, ServiceBinder>();

        public static boolean bindToService(Context context) {
                Log.e(LOGTAG, "bindToService: Context");
                return bindToService(context, null);
        }

        public static boolean bindToService(Context context,
                        ServiceConnection callback) {
                Log.e(LOGTAG, "bindToService: Context with serviceconnection callback");
                context.startService(new Intent(context, FMTransmitterService.class));
                ServiceBinder sb = new ServiceBinder(callback);
                sConnectionMap.put(context, sb);
                return context.bindService((new Intent()).setClass(context,
                                FMTransmitterService.class), sb, 0);
        }

        public static void unbindFromService(Context context) {
                ServiceBinder sb = (ServiceBinder) sConnectionMap.remove(context);
                Log.e(LOGTAG, "unbindFromService: Context");
                if (sb == null) {
                        Log.e(LOGTAG, "Trying to unbind for unknown Context");
                        return;
                }
                context.unbindService(sb);
                if (sConnectionMap.isEmpty()) {
                        // presumably there is nobody interested in the service at this
                        // point,
                        // so don't hang on to the ServiceConnection
                        sService = null;
                }
        }

        private static class ServiceBinder implements ServiceConnection {
                ServiceConnection mCallback;

                ServiceBinder(ServiceConnection callback) {
                        mCallback = callback;
                }

                public void onServiceConnected(ComponentName className,
                                android.os.IBinder service) {
                        sService = IFMTransmitterService.Stub.asInterface(service);
                        if (mCallback != null) {
                                Log.e(LOGTAG, "onServiceConnected: mCallback");
                                mCallback.onServiceConnected(className, service);
                        }
                }

                public void onServiceDisconnected(ComponentName className) {
                        if (mCallback != null) {
                                mCallback.onServiceDisconnected(className);
                        }
                        sService = null;
                }
        }

        private ServiceConnection osc = new ServiceConnection() {
                public void onServiceConnected(ComponentName classname, IBinder obj) {
                        mService = IFMTransmitterService.Stub.asInterface(obj);
                        Log.e(LOGTAG, "ServiceConnection: onServiceConnected: ");
                        if (mService != null) {
                            try {
                                  mService.registerCallbacks(mServiceCallbacks);
                                  if(false == mService.isHeadsetPlugged()) {
                                      Log.e(LOGTAG, "return for isHeadsetPlugged is false");
                                      if (SavedDataAndState == null) {
                                          enableRadioHandler.removeCallbacks(enableRadioTask);
                                          disableRadioHandler.removeCallbacks(disableRadioTask);
                                          enableRadioHandler.postDelayed(enableRadioTask, 0);
                                      } else if (SavedDataAndState.onOrOff) {
                                          enableRadioOnOffUI(true);
                                      } else {
                                          enableRadioOnOffUI(false);
                                      }
                                  } else {
                                      enableRadioOnOffUI(false);
                                  }
                            } catch (RemoteException e) {
                                  e.printStackTrace();
                            }
                            return;
                        } else {
                                Log.e(LOGTAG,
                                        "IFMTransmitterService onServiceConnected failed");
                        }
                        // startPlayback();
                        // Service is dead or not playing anything. If we got here as part
                        // of a "play this file" Intent, exit. Otherwise go to the Music
                        // app start screen.
                        if (getIntent().getData() == null) {
                            Intent intent = new Intent(Intent.ACTION_MAIN);
                            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            intent.setClass(FMTransmitterActivity.this,
                                                FMTransmitterActivity.class);
                            startActivity(intent);
                        }
                        finish();
                }

                public void onServiceDisconnected(ComponentName classname) {
                }
        };

        private IFMTransmitterServiceCallbacks.Stub mServiceCallbacks = new IFMTransmitterServiceCallbacks.Stub() {

                public void onDisabled() throws RemoteException {
                    mHandler.post(mRadioStateUpdated);
                }

                public void onRadioReset() throws RemoteException {
                    mHandler.post(mRadioReset);
                }

                public void onEnabled(boolean status) throws RemoteException {
                    mHandler.post(mRadioStateUpdated);
                }

                public void onRadioTextChanged() throws RemoteException {
                }

                public void onSearchListComplete(boolean status) throws RemoteException {
                    mIsSearching = false;
                    mSearchingResultStatus = status;
                    mHandler.post(mSearchListComplete);
                }

                public void onTuneStatusChanged(int frequency) throws RemoteException {
                        mTunedFrequency = frequency;
                        Log.d(LOGTAG, "onTuneStatusChanged: Frequency : " + mTunedFrequency);
                        FmSharedPreferences.setTunedFrequency(mTunedFrequency);
                        SavePreferences();
                        mHandler.post(mUpdateStationInfo);
                }
                public void onReconfigured() throws RemoteException {
                    RestoreDefaults();
                }
                public void onMetaDataChanged(String metaStr)  throws RemoteException {
                    Log.d(LOGTAG,"meta data is "+metaStr);
                    mMetaData = new String (metaStr);
                    mHandler.post(mUpdateRadioText);
                }
                public void onPSInfoSent(String psStr ) throws RemoteException {
                    Log.d(LOGTAG,"PS String data is "+psStr);
                    mPSData = new String (psStr);
                    mHandler.post(mUpdateRadioText);
                }
        };
        final Runnable mRadioStateUpdated = new Runnable() {
            public void run() {
                enableRadioOnOffButton();
               /* Update UI to FM On State */
                if(isFmOn()) {
                    enableRadioOnOffUI(true);
                    /* Tune to the last tuned frequency */
                    LoadPreferences();
                    tuneRadio(mTunedFrequency);
                } else {
                    /* Save the existing frequency */
                    FmSharedPreferences.setTunedFrequency(mTunedFrequency);
                    SavePreferences();
                    removeDialog(DIALOG_PRESET_LIST_AUTO_SET);
                    enableRadioOnOffUI(false);
                }

             }
          };
        final Runnable mRadioReset = new Runnable() {
            public void run() {
                    /* Save the existing frequency */
                    resetSearchProgress();
                    FmSharedPreferences.setTunedFrequency(mTunedFrequency);
                    SavePreferences();
                    enableRadioOnOffUI(false);
             }
          };
}
