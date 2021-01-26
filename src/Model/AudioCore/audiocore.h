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
#include <future>

// Custom
#include "Model/globals.h"


class MainWindow;
class SAudioEngine;
class SSound;
class SSoundMix;

enum CURRENT_TRACK_STATE
{
    CTS_STOPPED = 0,
    CTS_DELETED = 1,
    CTS_PLAYING = 2,
    CTS_PAUSED = 3
};

enum REPEAT_SECTION_STATE
{
    RSS_CLEARED = 0,
    RSS_LEFT_SET = 1,
    RSS_RIGHT_SET = 2
};

class AudioCore
{
public:
    AudioCore(MainWindow* pMainWindow);
    ~AudioCore();


    void addTracks   (const std::vector<std::wstring>& vFiles);
    void addTracks   (const std::wstring& sFolderPath);
    void removeTrack (const std::wstring& sAudioTitle);
    void setTrackPos (double x);


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


    CurrentEffects* getCurrentEffects();
    void setPitch        (float fPitch);
    void setReverbVolume (float fVolume);


    void saveTracklist   (const std::wstring& sPathToFile);
    void openTracklist   (const std::wstring& sPathToFile, bool bClearCurrentTracklist);


    void searchFindPrev  ();
    void searchFindNext  ();
    void searchTextSet   (const std::wstring& sKeyword);

private:

    std::wstring getTrackTitle (const std::wstring& sAudioPath);
    std::wstring getTrackExtension(const std::wstring& sTrackPath);
    std::wstring  getTrackInfo (XAudioFile* pTrack);

    void removeTrack           (XAudioFile* pAudio);
    void onCurrentTrackEnded   (SSound* pTrack);

    size_t findCaseInsensitive (std::wstring sText, std::wstring sKeyword);

    void drawGraph             ();
    void waitForGraphToStop    ();
    void applyAudioEffects     ();

    void monitorTrackPosition  ();
    std::string getTimeString  (double dTimeInSec);

    float read16bitSample      (unsigned char iByte1, unsigned char iByte2);
    float read24bitSample      (unsigned char iByte1, unsigned char iByte2, unsigned char iByte3);
    float read32bitSample      (unsigned char iByte1, unsigned char iByte2, unsigned char iByte3, unsigned char iByte4);


    MainWindow*   pMainWindow;
    SAudioEngine* pAudioEngine;
    SSound*       pCurrentTrack;
    SSoundMix*    pMix;

    std::mt19937_64*  pRndGen;


    std::vector<XAudioFile*> vAudioTracks;
    std::vector<XAudioFile*> vPlayedHistory;


    CurrentEffects      effects;


    std::promise<bool>  promiseFinishDrawGraph;
    std::mutex          mtxDrawGraph;
    bool                bDrawingGraph;


    std::promise<bool>  promiseFinishMonitorTrackPos;
    bool                bMonitorRunning;


    // Search
    std::vector<size_t> vSearchResult;
    size_t              iCurrentPosInSearchVec;
    bool                bFirstSearchAfterKeyChange;


    std::mutex    mtxProcess;


    bool          bLoadedTrackAtLeastOneTime;
    CURRENT_TRACK_STATE currentTrackState;


    bool          bRandomTrack;
    bool          bRepeatTrack;
    bool          bDestroyCalled;
};
