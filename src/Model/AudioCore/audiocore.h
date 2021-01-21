// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

// Custom
#include "Model/globals.h"


class MainWindow;
class SAudioEngine;
class SSound;

enum CURRENT_TRACK_STATE
{
    CTS_STOPPED = 0,
    CTS_DELETED = 1,
    CTS_PLAYING = 2,
    CTS_PAUSED = 3
};

class AudioCore
{
public:
    AudioCore(MainWindow* pMainWindow);
    ~AudioCore();

    void addTracks   (const std::vector<std::wstring>& vFiles);
    void addTracks   (const std::wstring& sFolderPath);
    void removeTrack (const std::wstring& sAudioTitle);

    void playTrack   (const std::wstring& sTrackTitle);
    void playTrack   ();
    void pauseTrack  ();
    void stopTrack   ();

    void setVolume   (int iVolume);

private:

    std::wstring getTrackTitle(const std::wstring& sAudioPath);


    MainWindow*   pMainWindow;
    SAudioEngine* pAudioEngine;
    SSound*       pCurrentTrack;


    std::vector<XAudioFile> vAudioTracks;

    std::mutex    mtxProcess;

    bool          bLoadedTrackAtLeastOneTime;
    CURRENT_TRACK_STATE currentTrackState;
};
