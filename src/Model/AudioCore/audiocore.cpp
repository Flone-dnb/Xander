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

namespace fs = std::filesystem;


AudioCore::AudioCore(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    pAudioEngine = new SAudioEngine(pMainWindow);
    pAudioEngine->init();
}

void AudioCore::addTracks(const std::vector<std::wstring> &vFiles)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    for (size_t i = 0; i < vFiles.size(); i++)
    {
        vAudioTracks.push_back(XAudioFile());

        vAudioTracks.back().pOpenedStream = new std::ifstream();
        vAudioTracks.back().pOpenedStream->open(vFiles[i], std::ios::binary);

        if (vAudioTracks.back().pOpenedStream->is_open())
        {
            vAudioTracks.back().sPathToAudioFile = vFiles[i];
            vAudioTracks.back().sAudioTitle = getTrackTitle(vFiles[i]);



            // Tell MainWindow to create widget.

            std::promise<class TrackWidget*> promiseCreateWidget;
            std::future<class TrackWidget*> future = promiseCreateWidget.get_future();

            pMainWindow->addTrackWidget(vAudioTracks.back().sAudioTitle, &promiseCreateWidget);

            vAudioTracks.back().pTrackWidget = future.get();
        }
        else
        {
            delete vAudioTracks.back().pOpenedStream;
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
        if (vAudioTracks[i].sAudioTitle == sAudioTitle)
        {
            vAudioTracks[i].pOpenedStream->close();
            delete vAudioTracks[i].pOpenedStream;

            std::promise<bool> promiseRemoveWidget;
            std::future<bool> future = promiseRemoveWidget.get_future();

            pMainWindow->removeTrackWidget(vAudioTracks[i].pTrackWidget, &promiseRemoveWidget);

            future.get();


            vAudioTracks.erase(vAudioTracks.begin() + i);

            break;
        }
    }
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

AudioCore::~AudioCore()
{
    delete pAudioEngine;
}
