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

void Controller::removeTrack(const std::wstring &sAudioTitle)
{
    pAudioCore->removeTrack(sAudioTitle);
}

void Controller::moveUp(const std::wstring &sAudioTitle)
{
    pAudioCore->moveUp(sAudioTitle);
}

void Controller::moveDown(const std::wstring &sAudioTitle)
{
    pAudioCore->moveDown(sAudioTitle);
}

void Controller::playTrack(const std::wstring &sTrackTitle)
{
    pAudioCore->playTrack(sTrackTitle, false);
}

void Controller::playTrack()
{
    pAudioCore->playTrack(false);
}

void Controller::pauseTrack()
{
    pAudioCore->pauseTrack();
}

void Controller::stopTrack()
{
    pAudioCore->stopTrack(false);
}

void Controller::prevTrack()
{
    pAudioCore->prevTrack();
}

void Controller::nextTrack()
{
    pAudioCore->nextTrack(false);
}

void Controller::setRandomTrack()
{
    pAudioCore->setRandomTrack();
}

void Controller::setRepeatTrack()
{
    pAudioCore->setRepeatTrack();
}

void Controller::clearTracklist()
{
    pAudioCore->clearTracklist();
}

void Controller::setVolume(int iVolume)
{
    pAudioCore->setVolume(iVolume);
}

void Controller::saveTracklist(const std::wstring &sPathToFile)
{
    pAudioCore->saveTracklist(sPathToFile);
}

void Controller::openTracklist(const std::wstring &sPathToFile, bool bClearCurrentTracklist)
{
    pAudioCore->openTracklist(sPathToFile, bClearCurrentTracklist);
}

void Controller::searchFindPrev()
{
    pAudioCore->searchFindPrev();
}

void Controller::searchFindNext()
{
    pAudioCore->searchFindNext();
}

void Controller::searchTextSet(const std::wstring &sKeyword)
{
    pAudioCore->searchTextSet(sKeyword);
}

Controller::~Controller()
{
    delete pAudioCore;
}
