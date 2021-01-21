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

Controller::~Controller()
{
    delete pAudioCore;
}
