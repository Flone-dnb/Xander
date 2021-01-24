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

class MainWindow;
class AudioCore;

class Controller
{
public:
    Controller(MainWindow* pMainWindow);
    ~Controller();


    void addTracks  (const std::vector<std::wstring>& vFiles);
    void addTracks  (const std::wstring& sFolderPath);
    void removeTrack(const std::wstring& sAudioTitle);


    void moveUp     (const std::wstring& sAudioTitle);
    void moveDown   (const std::wstring& sAudioTitle);


    void playTrack  (const std::wstring& sTrackTitle);
    void playTrack  ();
    void pauseTrack ();
    void stopTrack  ();
    void prevTrack  ();
    void nextTrack  ();


    void setRandomTrack();
    void setRepeatTrack();


    void clearTracklist();


    void setVolume   (int iVolume);


    class CurrentEffects* getCurrentEffects();
    void setPitch       (float fPitch);
    void setReverbVolume(float fVolume);


    void saveTracklist  (const std::wstring& sPathToFile);
    void openTracklist  (const std::wstring& sPathToFile, bool bClearCurrentTracklist);


    void searchFindPrev ();
    void searchFindNext ();
    void searchTextSet  (const std::wstring& sKeyword);

private:

    AudioCore* pAudioCore;
};
