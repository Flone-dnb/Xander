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

    void playTrack  (const std::wstring& sTrackTitle);
    void playTrack  ();
    void pauseTrack ();
    void stopTrack  ();
    void prevTrack  ();

    void clearTracklist();

    void setVolume   (int iVolume);

private:

    AudioCore* pAudioCore;
};
