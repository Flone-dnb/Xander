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

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/AudioEngine/SAudioEngine/saudioengine.h"
#include "Model/AudioEngine/SSound/ssound.h"

namespace fs = std::filesystem;


AudioCore::AudioCore(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    pAudioEngine = new SAudioEngine(pMainWindow);
    pAudioEngine->init();
    pAudioEngine->setMasterVolume(DEFAULT_VOLUME / 100.0f);

    pCurrentTrack = new SSound(pAudioEngine);

    bLoadedTrackAtLeastOneTime = false;

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

void AudioCore::playTrack(const std::wstring &sTrackTitle)
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

            pMainWindow->changePlayButtonStyle(true);



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

void AudioCore::playTrack()
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        pCurrentTrack->playSound();

        pMainWindow->changePlayButtonStyle(true);

        currentTrackState = CTS_PLAYING;
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

            pMainWindow->changePlayButtonStyle(false);
        }
    }
}

void AudioCore::stopTrack()
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        pCurrentTrack->stopSound();

        currentTrackState = CTS_STOPPED;

        pMainWindow->changePlayButtonStyle(false);
    }
}

void AudioCore::prevTrack()
{
    mtxProcess.lock();

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        if (vPlayedHistory.size() > 1)
        {
            XAudioFile* pFind = vPlayedHistory[vPlayedHistory.size() - 2];

            bool bFound = false;

            do
            {
                for (size_t i = 0; i < vAudioTracks.size(); i++)
                {
                    if (vAudioTracks[i] == pFind)
                    {
                        bFound = true;
                        break;
                    }
                }

                if (bFound == false)
                {
                    vAudioTracks.pop_back();
                }

                if (vAudioTracks.size() == 0)
                {
                    break;
                }
            }while(bFound == false);

            if (bFound)
            {
                vPlayedHistory.pop_back(); // pop current
                vPlayedHistory.pop_back();

                mtxProcess.unlock();
                playTrack(pFind->sAudioTitle);

                pMainWindow->setNewPlayingTrack(pFind->pTrackWidget);
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
    else
    {
        mtxProcess.unlock();
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

    delete pAudioEngine;
}
