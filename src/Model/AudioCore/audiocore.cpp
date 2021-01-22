// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "audiocore.h"

// STL
#include <future>
#include <filesystem>
#include <functional>

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/AudioEngine/SAudioEngine/saudioengine.h"
#include "Model/AudioEngine/SSound/ssound.h"

namespace fs = std::filesystem;


AudioCore::AudioCore(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    pRndGen = new std::mt19937_64( std::random_device{}() );

    pAudioEngine = new SAudioEngine(pMainWindow);
    pAudioEngine->init();
    pAudioEngine->setMasterVolume(DEFAULT_VOLUME / 100.0f);

    pCurrentTrack = new SSound(pAudioEngine);

    std::function<void(SSound*)> f = std::bind(&AudioCore::onCurrentTrackEnded, this, std::placeholders::_1);
    pCurrentTrack->setOnPlayEndCallback(f);


    bLoadedTrackAtLeastOneTime = false;
    bRandomTrack = false;
    bRepeatTrack = false;

    currentTrackState = CTS_DELETED;
}

void AudioCore::addTracks(const std::vector<std::wstring> &vFiles)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    for (size_t i = 0; i < vFiles.size(); i++)
    {
        vAudioTracks.push_back(new XAudioFile());

        vAudioTracks.back()->pOpenedStream = new std::ifstream();
        vAudioTracks.back()->pOpenedStream->open(vFiles[i], std::ios::binary);

        if (vAudioTracks.back()->pOpenedStream->is_open())
        {
            vAudioTracks.back()->sPathToAudioFile = vFiles[i];
            vAudioTracks.back()->sAudioTitle = getTrackTitle(vFiles[i]);



            // Tell MainWindow to create widget.

            std::promise<class TrackWidget*> promiseCreateWidget;
            std::future<class TrackWidget*> future = promiseCreateWidget.get_future();

            pMainWindow->addTrackWidget(vAudioTracks.back()->sAudioTitle, &promiseCreateWidget);

            vAudioTracks.back()->pTrackWidget = future.get();
        }
        else
        {
            delete vAudioTracks.back()->pOpenedStream;
            delete vAudioTracks[i];
            vAudioTracks.pop_back();

            pMainWindow->showMessageBox(L"Error", L"An error occurred at AudioCore::addTracks(): could not open the file \""
                                        + vFiles[i] + L"\", skipping this file.", true);
        }
    }
}

void AudioCore::addTracks(const std::wstring &sFolderPath)
{
    mtxProcess.lock();

    std::vector<std::wstring> vFilePaths;

    for (const auto& entry : fs::directory_iterator(sFolderPath))
    {
        if (entry.is_directory() == false &&
                (entry.path().extension() == EXTENSION_MP3 || entry.path().extension() == EXTENSION_WAV || entry.path().extension() == EXTENSION_OGG))
        {
            vFilePaths.push_back(entry.path().wstring());
        }
    }

    mtxProcess.unlock();

    addTracks(vFilePaths);
}

void AudioCore::removeTrack(const std::wstring &sAudioTitle)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    for (size_t i = 0; i < vAudioTracks.size(); i++)
    {
        if (vAudioTracks[i]->sAudioTitle == sAudioTitle)
        {
            for (size_t j = 0; j < vPlayedHistory.size();)
            {
                // Remove from history.

                if (vPlayedHistory[j] == vAudioTracks[i])
                {
                    vPlayedHistory.erase(vPlayedHistory.begin() + j);
                }
                else
                {
                    j++;
                }
            }


            vAudioTracks[i]->pOpenedStream->close();
            delete vAudioTracks[i]->pOpenedStream;

            std::promise<bool> promiseRemoveWidget;
            std::future<bool> future = promiseRemoveWidget.get_future();

            pMainWindow->removeTrackWidget(vAudioTracks[i]->pTrackWidget, &promiseRemoveWidget);

            future.get();

            delete vAudioTracks[i];


            vAudioTracks.erase(vAudioTracks.begin() + i);


            currentTrackState = CTS_DELETED;

            break;
        }
    }
}

void AudioCore::playTrack(const std::wstring &sTrackTitle, bool bCalledFromOtherThread)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    for (size_t i = 0; i < vAudioTracks.size(); i++)
    {
        if (vAudioTracks[i]->sAudioTitle == sTrackTitle)
        {
            if (pCurrentTrack->loadAudioFile(vAudioTracks[i]->sPathToAudioFile, true))
            {
                return;
            }

            pCurrentTrack->playSound();

            bLoadedTrackAtLeastOneTime = true;
            currentTrackState = CTS_PLAYING;

            pMainWindow->changePlayButtonStyle(true, bCalledFromOtherThread);



            // Add to history.

            if (vPlayedHistory.size() > 0)
            {
                if (vPlayedHistory.back() != vAudioTracks[i])
                {
                    if (vPlayedHistory.size() == MAX_HISTORY_SIZE)
                    {
                        vPlayedHistory.erase(vPlayedHistory.begin());
                    }

                    vPlayedHistory.push_back(vAudioTracks[i]);
                }
            }
            else
            {
                vPlayedHistory.push_back(vAudioTracks[i]);
            }

            break;
        }
    }
}

void AudioCore::playTrack(bool bCalledFromOtherThread)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        if (currentTrackState == CTS_PAUSED)
        {
            pCurrentTrack->unpauseSound();

            currentTrackState = CTS_PLAYING;

            pMainWindow->changePlayButtonStyle(true, bCalledFromOtherThread);
        }
        else
        {
            pCurrentTrack->playSound();

            pMainWindow->changePlayButtonStyle(true, bCalledFromOtherThread);

            currentTrackState = CTS_PLAYING;
        }
    }
}

void AudioCore::pauseTrack()
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        if (currentTrackState == CTS_PLAYING)
        {
            pCurrentTrack->pauseSound();

            currentTrackState = CTS_PAUSED;

            pMainWindow->changePlayButtonStyle(false, false);
        }
        else if (currentTrackState == CTS_PAUSED)
        {
            pCurrentTrack->unpauseSound();

            currentTrackState = CTS_PLAYING;

            pMainWindow->changePlayButtonStyle(true, false);
        }
    }
}

void AudioCore::stopTrack(bool bCalledFromOtherThread)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        pCurrentTrack->stopSound();

        currentTrackState = CTS_STOPPED;

        pMainWindow->changePlayButtonStyle(false, bCalledFromOtherThread);
    }
}

void AudioCore::prevTrack()
{
    mtxProcess.lock();

    if (vAudioTracks.size() == 0)
    {
        mtxProcess.unlock();
        return;
    }

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        if (vPlayedHistory.size() > 1)
        {
            XAudioFile* pFind = vPlayedHistory[vPlayedHistory.size() - 2];

            bool bFound = false;

            // Should 100% find.
            for (size_t i = 0; i < vAudioTracks.size(); i++)
            {
                if (vAudioTracks[i] == pFind)
                {
                    bFound = true;
                    break;
                }
            }

            if (bFound)
            {
                vPlayedHistory.pop_back(); // pop current
                vPlayedHistory.pop_back();

                mtxProcess.unlock();
                playTrack(pFind->sAudioTitle, false);

                pMainWindow->setNewPlayingTrack(pFind->pTrackWidget, false);
            }
            else
            {
                mtxProcess.unlock();

                pMainWindow->showMessageBox(L"Error", L"An error occurred at AudioCore::prevTrack(): "
                                                      "could not find XAudioFile.", true);
            }
        }
        else
        {
            mtxProcess.unlock();
        }
    }
    else
    {
        mtxProcess.unlock();
    }
}

void AudioCore::nextTrack(bool bCalledFromOtherThread)
{
    mtxProcess.lock();

    if (vAudioTracks.size() == 0)
    {
        mtxProcess.unlock();
        return;
    }

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        if (bRandomTrack)
        {
            size_t iNextTrackIndex = 0;

            size_t iCurrentIndex = 0;

            for (size_t i = 0; i < vAudioTracks.size(); i++)
            {
                if (vAudioTracks[i] == vPlayedHistory.back())
                {
                    iCurrentIndex = i;
                }
            }


            if (vAudioTracks.size() > 1)
            {
                std::uniform_int_distribution<> uid(0, static_cast<int>(vAudioTracks.size()) - 1);

                do
                {
                    iNextTrackIndex = static_cast<size_t>(uid(*pRndGen));

                }while (iNextTrackIndex == iCurrentIndex);

                mtxProcess.unlock();

                playTrack(vAudioTracks[iNextTrackIndex]->sAudioTitle, bCalledFromOtherThread);

                pMainWindow->setNewPlayingTrack(vAudioTracks[iNextTrackIndex]->pTrackWidget, bCalledFromOtherThread);
            }
            else
            {
                mtxProcess.unlock();

                stopTrack(bCalledFromOtherThread);
                playTrack(bCalledFromOtherThread);
            }
        }
        else if (bRepeatTrack)
        {
            mtxProcess.unlock();

            stopTrack(bCalledFromOtherThread);
            playTrack(bCalledFromOtherThread);
        }
        else
        {
            size_t iCurrentIndex = 0;

            for (size_t i = 0; i < vAudioTracks.size(); i++)
            {
                if (vAudioTracks[i] == vPlayedHistory.back())
                {
                    iCurrentIndex = i;
                }
            }

            size_t iNextTrackIndex = iCurrentIndex + 1;

            if (iCurrentIndex == vAudioTracks.size() - 1)
            {
                iNextTrackIndex = 0;
            }

            mtxProcess.unlock();

            playTrack(vAudioTracks[iNextTrackIndex]->sAudioTitle, bCalledFromOtherThread);

            pMainWindow->setNewPlayingTrack(vAudioTracks[iNextTrackIndex]->pTrackWidget, bCalledFromOtherThread);
        }
    }
    else
    {
        mtxProcess.unlock();
    }
}

void AudioCore::setRandomTrack()
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    bRandomTrack = !bRandomTrack;

    pMainWindow->changeRandomButtonStyle(bRandomTrack);

    if (bRandomTrack && bRepeatTrack)
    {
        bRepeatTrack = false;
        pMainWindow->changeRepeatButtonStyle(false);
    }
}

void AudioCore::setRepeatTrack()
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    bRepeatTrack = !bRepeatTrack;

    pMainWindow->changeRepeatButtonStyle(bRepeatTrack);

    if (bRepeatTrack && bRandomTrack)
    {
        bRandomTrack = false;
        pMainWindow->changeRandomButtonStyle(false);
    }
}

void AudioCore::clearTracklist()
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        pCurrentTrack->stopSound();
    }

    for(size_t i = 0; i < vAudioTracks.size(); i++)
    {
        removeTrack(vAudioTracks[i]);
    }

    vAudioTracks.clear();

    vPlayedHistory.clear();

    currentTrackState = CTS_DELETED;

    pMainWindow->changePlayButtonStyle(false, false);
}

void AudioCore::setVolume(int iVolume)
{
    pAudioEngine->setMasterVolume(iVolume / 100.0f);
}

std::wstring AudioCore::getTrackTitle(const std::wstring &sAudioPath)
{
    size_t iTitleStartIndex = 0;
    size_t iLastDotPos = 0;

    for (size_t i = sAudioPath.size() - 1; i >= 1; i--)
    {
        if (sAudioPath[i] == L'.' && iLastDotPos == 0)
        {
            iLastDotPos = i;
        }

        if (sAudioPath[i] == L'/' || sAudioPath[i] == L'\\')
        {
            iTitleStartIndex = i + 1;
            break;
        }
    }

    std::wstring sTrackTitle = sAudioPath.substr(iTitleStartIndex, iLastDotPos - iTitleStartIndex);

    return sTrackTitle;
}

void AudioCore::removeTrack(XAudioFile *pAudio)
{
    pAudio->pOpenedStream->close();
    delete pAudio->pOpenedStream;

    std::promise<bool> promiseRemoveWidget;
    std::future<bool> future = promiseRemoveWidget.get_future();

    pMainWindow->removeTrackWidget(pAudio->pTrackWidget, &promiseRemoveWidget);

    future.get();

    delete pAudio;
}

void AudioCore::onCurrentTrackEnded(SSound* pTrack)
{
    if (pTrack->isSoundStoppedManually() == false)
    {
        if (bRepeatTrack)
        {
            stopTrack(true);
            playTrack(true);
        }
        else
        {
            nextTrack(true);
        }
    }
}

AudioCore::~AudioCore()
{
    if (currentTrackState != CTS_DELETED)
    {
        pCurrentTrack->stopSound();
    }

    delete pCurrentTrack;

    for (size_t i = 0; i < vAudioTracks.size(); i++)
    {
        removeTrack(vAudioTracks[i]);
    }

    vAudioTracks.clear();

    delete pRndGen;

    delete pAudioEngine;
}
