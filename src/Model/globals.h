// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <fstream>
#include <string>


struct XAudioFile
{
    std::wstring sAudioTitle;
    std::wstring sPathToAudioFile;
    std::ifstream* pOpenedStream;

    std::wstring sTrackExtension;

    class TrackWidget* pTrackWidget = nullptr;
};

struct CurrentEffects
{
    float fPitchInSemitones = 0.0f;
    float fReverbVolume = 0.0f;
};

#define XANDER_VERSION "1.1.1a"

#define RES_LOGO_PATH ":/logo.png"

#define DEFAULT_VOLUME 80
#define MAX_HISTORY_SIZE 50

#define EXTENSION_MP3 ".mp3"
#define EXTENSION_WAV ".wav"
#define EXTENSION_OGG ".ogg"

#define PLAYED_SECTION_ALPHA 130
#define REPEAT_GRAYED_ALPHA 120
#define MAX_Y_AXIS_VALUE 1.03
#define MAX_X_AXIS_VALUE 1000

#define UPDATE_TRACK_POS_IN_MS 500

#define REPEAT_SECTION_DELTA_IN_SEC 1.0
#define TRANSITION_SLEEP_MS 1
