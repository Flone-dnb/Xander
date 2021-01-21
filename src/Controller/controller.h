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


    void addTracks(const std::vector<std::wstring>& vFiles);
    void addTracks(const std::wstring& sFolderPath);

private:

    AudioCore* pAudioCore;
};
