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

    class TrackWidget* pTrackWidget = nullptr;
};

#define XANDER_VERSION "1.0.0"

#define RES_LOGO_PATH ":/logo.png"

#define DEFAULT_VOLUME 80
#define MAX_HISTORY_SIZE 50

#define EXTENSION_MP3 ".mp3"
#define EXTENSION_WAV ".wav"
#define EXTENSION_OGG ".ogg"
