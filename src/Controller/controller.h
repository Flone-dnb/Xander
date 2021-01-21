// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

class MainWindow;
class AudioCore;

class Controller
{
public:
    Controller(MainWindow* pMainWindow);
    ~Controller();

private:

    AudioCore* pAudioCore;
};
