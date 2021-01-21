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

class AudioCore
{
public:
    AudioCore(MainWindow* pMainWindow);
    ~AudioCore();

    void addTracks   (const std::vector<std::wstring>& vFiles);
    void addTracks   (const std::wstring& sFolderPath);

    void removeTrack (const std::wstring& sAudioTitle);

private:

    std::wstring getTrackTitle(const std::wstring& sAudioPath);


    MainWindow*   pMainWindow;
    SAudioEngine* pAudioEngine;

    std::vector<XAudioFile> vAudioTracks;

    std::mutex    mtxProcess;
};
