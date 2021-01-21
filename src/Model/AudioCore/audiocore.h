// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

class MainWindow;
class SAudioEngine;

class AudioCore
{
public:
    AudioCore(MainWindow* pMainWindow);
    ~AudioCore();

private:

    MainWindow*   pMainWindow;
    SAudioEngine* pAudioEngine;
};
