#pragma once

#define PLUG_MFR "lkjb"
#define PLUG_NAME "QRange"

#define PLUG_CLASS_NAME QRange

#define BUNDLE_MFR "lkjb"
#define BUNDLE_NAME "QRange"

#define PLUG_ENTRY QRange_Entry
#define PLUG_VIEW_ENTRY QRange_ViewEntry

#define PLUG_ENTRY_STR "QRange_Entry"
#define PLUG_VIEW_ENTRY_STR "QRange_ViewEntry"

#define VIEW_CLASS QRange_View
#define VIEW_CLASS_STR "QRange_View"

// Format        0xMAJR.MN.BG - in HEX! so version 10.1.5 would be 0x000A0105
#define PLUG_VER 0x00020000
#define VST3_VER_STR "2.0.0"

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes. At least one capital letter
#define PLUG_UNIQUE_ID 'LqrG'
// make sure this is not the same as BUNDLE_MFR
#define PLUG_MFR_ID 'Lkjb'

// ProTools stuff

#if (defined(AAX_API) || defined(RTAS_API)) && !defined(_PIDS_)
  #define _PIDS_
  const int PLUG_TYPE_IDS[2] = {'EFN1', 'EFN2'};
  const int PLUG_TYPE_IDS_AS[2] = {'EFA1', 'EFA2'}; // AudioSuite
#endif

#define PLUG_MFR_PT "lkjb\nlkjb"
#define PLUG_NAME_PT "QRange"
#define PLUG_TYPE_PT "EQ"
#define PLUG_DOES_AUDIOSUITE 1

/* PLUG_TYPE_PT can be "None", "EQ", "Dynamics", "PitchShift", "Reverb", "Delay", "Modulation", 
"Harmonic" "NoiseReduction" "Dither" "SoundField" "Effect" 
instrument determined by PLUG _IS _INST
*/

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0

// if this is 0 RTAS can't get tempo info
#define PLUG_DOES_MIDI 0
#define AU_MIDI_FX 0

#define PLUG_DOES_STATE_CHUNKS 1

// Unique IDs for each image resource.
#define BUTTON_ID 101
#define BACKGROUND_ID 102
#define BUTTONNARROW_ID 103
#define SETTINGSICON_ID 104

// Image resource locations for this plug.
#define BUTTON_FN "resources/img/button.png"
#define BACKGROUND_FN "resources/img/background.png"
#define BUTTONNARROW_FN "resources/img/button_narrow.png"
#define SETTINGSICON_FN "resources/img/settings_icon.png"
  
// GUI default dimensions
#define GUI_WIDTH 953
#define GUI_HEIGHT 608

// on MSVC, you must define SA_API in the resource editor preprocessor macros as well as the c++ ones
#if defined(SA_API)
#include "app_wrapper/app_resource.h"
#endif

// vst3 stuff
#define MFR_URL "www.kvraudio.com/developer/lkjb"
#define MFR_EMAIL "lkjb.dsp@gmail.com"
#define EFFECT_TYPE_VST3 "Fx|EQ"


// Parameters and presets stuff ------------------------------------------------------------------------
#define kNumPrograms 1
//#define kNumPublicParams 1
#define kNumPrivateParams 0
  
// Public parameters should go on top and private on the bottom.
// You can freely add or remove parameters from the bottom while preserving the compatibility.
enum EParam
{
	kEnabled,
	kType,
	kFreq,
	kGain,
	kQ,
    kCutOrder,
	kRouting,
	kNumParametersPerBand,

	kNumBands = 12,
	kBandParametersEnd = kNumBands * kNumParametersPerBand - 1, // EQ Parameters

    kOutputGain,
    kBypass,

	kNumPublicParams
};
