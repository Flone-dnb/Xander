// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "controller.h"

// Custom
#include "Model/AudioCore/audiocore.h"

Controller::Controller(MainWindow* pMainWindow)
{
    pAudioCore = new AudioCore(pMainWindow);
}

void Controller::addTracks(const std::vector<std::wstring> &vFiles)
{
    pAudioCore->addTracks(vFiles);
}

void Controller::addTracks(const std::wstring &sFolderPath)
{
    pAudioCore->addTracks(sFolderPath);
}

Controller::~Controller()
{
    delete pAudioCore;
}
