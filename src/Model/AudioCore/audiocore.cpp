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
    pAudioEngine->init(false);
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
            vAudioTracks.back()->sTrackExtension = getTrackExtension(vFiles[i]);



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
                                        + vFiles[i] + L"\", skipping this file.", true, false);
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
            if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED && vPlayedHistory.back()->sAudioTitle == sAudioTitle)
            {
                // Playing right now.
                pCurrentTrack->stopSound();
            }


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

void AudioCore::moveUp(const std::wstring &sAudioTitle)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    for (size_t i = 0; i < vAudioTracks.size(); i++)
    {
        if (vAudioTracks[i]->sAudioTitle == sAudioTitle)
        {
            XAudioFile* pAudio = vAudioTracks[i];
            vAudioTracks.erase(vAudioTracks.begin() + i);

            if (i == 0)
            {
                vAudioTracks.push_back(pAudio);
            }
            else
            {
                vAudioTracks.insert(vAudioTracks.begin() + i - 1, pAudio);
            }

            break;
        }
    }
}

void AudioCore::moveDown(const std::wstring &sAudioTitle)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    for (size_t i = 0; i < vAudioTracks.size(); i++)
    {
        if (vAudioTracks[i]->sAudioTitle == sAudioTitle)
        {
            XAudioFile* pAudio = vAudioTracks[i];
            vAudioTracks.erase(vAudioTracks.begin() + i);

            if (i == vAudioTracks.size())
            {
                vAudioTracks.insert(vAudioTracks.begin(), pAudio);
            }
            else
            {
                vAudioTracks.insert(vAudioTracks.begin() + i + 1, pAudio);
            }

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

            if (pCurrentTrack->playSound())
            {
                return;
            }

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


            // Show track on screen.

            pMainWindow->setTrackInfo(vAudioTracks[i]->sAudioTitle, getTrackInfo(vAudioTracks[i]));


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
                                                      "could not find XAudioFile.", true, false);
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

                    break;
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

                    break;
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

void AudioCore::saveTracklist(const std::wstring &sPathToFile)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (vAudioTracks.size() == 0)
    {
        pMainWindow->showMessageBox(L"Information", L"The tracklist is empty, there is nothing to save.", false, false);

        return;
    }

    std::ofstream file(sPathToFile, std::ios::binary);


    unsigned int iTrackCount = static_cast<unsigned int>(vAudioTracks.size());
    file.write(reinterpret_cast<char*>(&iTrackCount), sizeof(iTrackCount));

    for (unsigned int i = 0; i < iTrackCount; i++)
    {
        unsigned short iPathSize = static_cast<unsigned short>(vAudioTracks[i]->sPathToAudioFile.size());
        file.write(reinterpret_cast<char*>(&iPathSize), sizeof(iPathSize));

        file.write(reinterpret_cast<char*>(const_cast<wchar_t*>(vAudioTracks[i]->sPathToAudioFile.c_str())), iPathSize * sizeof(wchar_t));
    }


    file.close();
}

void AudioCore::openTracklist(const std::wstring &sPathToFile, bool bClearCurrentTracklist)
{
    if (bClearCurrentTracklist)
    {
        clearTracklist();
    }


    mtxProcess.lock();


    std::ifstream file(sPathToFile, std::ios::binary);

    if (file.is_open() == false)
    {
        pMainWindow->showMessageBox(L"Error", L"Could not open the tracklist file.", true, false);

        mtxProcess.unlock();
        return;
    }


    unsigned int iTrackCount = 0;
    file.read(reinterpret_cast<char*>(&iTrackCount), sizeof(iTrackCount));

    std::vector<std::wstring> vFilePaths;

    for (unsigned int i = 0; i < iTrackCount; i++)
    {
        unsigned short iPathSize = 0;
        file.read(reinterpret_cast<char*>(&iPathSize), sizeof(iPathSize));

        wchar_t* pPathString = new wchar_t[iPathSize + 1];
        memset(pPathString, 0, (iPathSize + 1) * sizeof(wchar_t));

        file.read(reinterpret_cast<char*>(pPathString), iPathSize * sizeof(wchar_t));

        vFilePaths.push_back(pPathString);

        delete[] pPathString;
    }


    file.close();


    mtxProcess.unlock();
    addTracks(vFilePaths);
}

void AudioCore::searchFindPrev()
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (vSearchResult.size() > 0)
    {
        if (bFirstSearchAfterKeyChange == false)
        {
            if (iCurrentPosInSearchVec == 0)
            {
                iCurrentPosInSearchVec = vSearchResult.size() - 1;
            }
            else
            {
                iCurrentPosInSearchVec--;
            }
        }


        if ( !(bFirstSearchAfterKeyChange == false && vSearchResult.size() == 1) ) // do not select again if matches == 1 && already selected
        {
            pMainWindow->searchSetSelected ( vAudioTracks[vSearchResult[iCurrentPosInSearchVec]]->pTrackWidget );
        }


        bFirstSearchAfterKeyChange = false;
    }
}

void AudioCore::searchFindNext()
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (vSearchResult.size() > 0)
    {
        if (bFirstSearchAfterKeyChange == false)
        {
            if (iCurrentPosInSearchVec == vSearchResult.size() - 1)
            {
                iCurrentPosInSearchVec = 0;
            }
            else
            {
                iCurrentPosInSearchVec++;
            }
        }


        if ( !(bFirstSearchAfterKeyChange == false && vSearchResult.size() == 1) ) // do not select again if matches == 1 && already selected
        {
            pMainWindow->searchSetSelected ( vAudioTracks[vSearchResult[iCurrentPosInSearchVec]]->pTrackWidget );
        }


        bFirstSearchAfterKeyChange = false;
    }
}

void AudioCore::searchTextSet(const std::wstring &sKeyword)
{
    mtxProcess.lock();


    vSearchResult.clear();
    iCurrentPosInSearchVec = 0;

    if (sKeyword != L"")
    {
        for (size_t i = 0; i < vAudioTracks.size(); i++)
        {
            if ( findCaseInsensitive( vAudioTracks[i]->sAudioTitle, const_cast<std::wstring&>(sKeyword)) != std::string::npos )
            {
                vSearchResult.push_back(i);
            }
        }
    }

    bFirstSearchAfterKeyChange = true;


    mtxProcess.unlock();


    pMainWindow->setSearchMatchCount (vSearchResult.size());
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

std::wstring AudioCore::getTrackExtension(const std::wstring &sTrackPath)
{
    size_t iLastDotPos = 0;

    for (size_t i = sTrackPath.size() - 1; i >= 1; i--)
    {
        if (sTrackPath[i] == L'.')
        {
            iLastDotPos = i;

            break;
        }
    }

    std::wstring sTrackExtension = sTrackPath.substr(iLastDotPos + 1, sTrackPath.size() - (iLastDotPos + 1));

    return sTrackExtension;
}

std::wstring AudioCore::getTrackInfo(XAudioFile *pTrack)
{
    std::wstring sTrackInfo = L"";

    // Extension first.
    sTrackInfo += pTrack->sTrackExtension;


    if (pCurrentTrack == nullptr)
    {
        return L"";
    }

    SSoundInfo info;
    pCurrentTrack->getSoundInfo(info);


    // Sample rate
    if (info.iSampleRate != 0)
    {
        sTrackInfo += L", ";
        sTrackInfo += std::to_wstring(info.iSampleRate) + L" hz";
    }

    // Channels
    sTrackInfo += L", ";
    sTrackInfo += L"channels: " + std::to_wstring(info.iChannels);

    // Bits per sample
    sTrackInfo += L", ";
    sTrackInfo += std::to_wstring(info.iBitsPerSample) + L" bits";

    // Bitrate
    if (info.iBitrate != 0)
    {
        sTrackInfo += L", ";
        sTrackInfo += std::to_wstring(info.iBitrate / 1000) + L" kbit/s";
    }

    // VBR
    if (info.bUsesVariableBitRate)
    {
        sTrackInfo += L", vbr";
    }

    // File size.
    sTrackInfo += L", ";
    float fFileSizeInMB = info.iFileSizeInBytes / 1024.0f / 1024.0f;
    size_t iPrecision = 2;
    sTrackInfo += std::to_wstring(fFileSizeInMB).substr(0, std::to_wstring(fFileSizeInMB).find(L".") + iPrecision + 1) + L" MB";

    return sTrackInfo;
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

size_t AudioCore::findCaseInsensitive(std::wstring sText, std::wstring sKeyword)
{
    // All to lower case

    std::transform(sText.begin(), sText.end(), sText.begin(), ::tolower);

    std::transform(sKeyword.begin(), sKeyword.end(), sKeyword.begin(), ::tolower);

    return sText.find(sKeyword);
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
