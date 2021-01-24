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
#include <random>

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


    void moveUp     (const std::wstring& sAudioTitle);
    void moveDown   (const std::wstring& sAudioTitle);


    void playTrack   (const std::wstring& sTrackTitle, bool bCalledFromOtherThread);
    void playTrack   (bool bCalledFromOtherThread);
    void pauseTrack  ();
    void stopTrack   (bool bCalledFromOtherThread);
    void prevTrack   ();
    void nextTrack   (bool bCalledFromOtherThread);


    void setRandomTrack();
    void setRepeatTrack();


    void clearTracklist();


    void setVolume   (int iVolume);


    void searchFindPrev  ();
    void searchFindNext  ();
    void searchTextSet   (const std::wstring& sKeyword);

private:

    std::wstring getTrackTitle(const std::wstring& sAudioPath);
    std::wstring getTrackExtension(const std::wstring& sTrackPath);
    std::wstring  getTrackInfo (XAudioFile* pTrack);

    void removeTrack(XAudioFile* pAudio);
    void onCurrentTrackEnded(SSound* pTrack);

    size_t findCaseInsensitive(std::wstring sText, std::wstring sKeyword);


    MainWindow*   pMainWindow;
    SAudioEngine* pAudioEngine;
    SSound*       pCurrentTrack;

    std::mt19937_64*  pRndGen;


    std::vector<XAudioFile*> vAudioTracks;
    std::vector<XAudioFile*> vPlayedHistory;

    // Search
    std::vector<size_t> vSearchResult;
    size_t              iCurrentPosInSearchVec;
    bool                bFirstSearchAfterKeyChange;

    std::mutex    mtxProcess;

    bool          bLoadedTrackAtLeastOneTime;
    CURRENT_TRACK_STATE currentTrackState;

    bool          bRandomTrack;
    bool          bRepeatTrack;
};
