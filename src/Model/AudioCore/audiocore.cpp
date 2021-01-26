// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "audiocore.h"

// STL
#include <filesystem>
#include <functional>

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/AudioEngine/SAudioEngine/saudioengine.h"
#include "Model/AudioEngine/SSound/ssound.h"
#include "Model/AudioEngine/SSoundMix/ssoundmix.h"

namespace fs = std::filesystem;


AudioCore::AudioCore(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    pRndGen = new std::mt19937_64( std::random_device{}() );

    pAudioEngine = new SAudioEngine(pMainWindow);
    pAudioEngine->init(false);
    pAudioEngine->setMasterVolume(DEFAULT_VOLUME / 100.0f);

    pAudioEngine->createSoundMix(pMix);

    // Add eq.
    std::vector<SAudioEffect> vEffects;


    SAudioEffect eq(pAudioEngine, S_EFFECT_TYPE::ET_EQ, true);
    FXEQ_PARAMETERS eqparams;

    eqparams.FrequencyCenter0 = FXEQ_DEFAULT_FREQUENCY_CENTER_0;
    eqparams.Gain0 = 0.8f;
    eqparams.Bandwidth0 = FXEQ_DEFAULT_BANDWIDTH;
    eqparams.FrequencyCenter1 = FXEQ_DEFAULT_FREQUENCY_CENTER_1;
    eqparams.Gain1 = 2.5f;
    eqparams.Bandwidth1 = FXEQ_DEFAULT_BANDWIDTH;
    eqparams.FrequencyCenter2 = FXEQ_DEFAULT_FREQUENCY_CENTER_2;
    eqparams.Gain2 = 0.5f;
    eqparams.Bandwidth2 = FXEQ_DEFAULT_BANDWIDTH;
    eqparams.FrequencyCenter3 = FXEQ_DEFAULT_FREQUENCY_CENTER_3;
    eqparams.Gain3 = 1.0f;
    eqparams.Bandwidth3 = FXEQ_DEFAULT_BANDWIDTH;

    eq.setEQParameters(eqparams);

    vEffects.push_back(eq);

    // Add reverb.
    SAudioEffect reverb(pAudioEngine, S_EFFECT_TYPE::ET_REVERB, true);
    FXREVERB_PARAMETERS params;
    params.RoomSize = FXREVERB_MAX_ROOMSIZE;
    params.Diffusion = FXREVERB_MAX_DIFFUSION;

    reverb.setReverbParameters(params);
    vEffects.push_back(reverb);

    // Set disabled.
    pMix->setFXVolume(0.0f);
    pMix->setAudioEffects(&vEffects);




    pCurrentTrack = new SSound(pAudioEngine);

    std::function<void(SSound*)> f = std::bind(&AudioCore::onCurrentTrackEnded, this, std::placeholders::_1);
    pCurrentTrack->setOnPlayEndCallback(f);


    bLoadedTrackAtLeastOneTime = false;
    bRandomTrack = false;
    bRepeatTrack = false;
    bDrawingGraph = false;
    bDestroyCalled = false;
    bMonitorRunning = false;

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

                waitForGraphToStop();
                pMainWindow->clearGraph();

                currentTrackState = CTS_DELETED;

                pMainWindow->changePlayButtonStyle(false, false);
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

            break;
        }
    }
}

void AudioCore::setTrackPos(double x)
{
    std::lock_guard<std::mutex> lock(mtxProcess);

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        SSoundInfo info;
        pCurrentTrack->getSoundInfo(info);

        double dPercent = x / pMainWindow->getMaxXPosOnGraph();
        double dResultPosInSec = dPercent * info.dSoundLengthInSec;

        pCurrentTrack->setPositionInSec(dResultPosInSec);
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


    if (bMonitorRunning == false)
    {
        std::thread t (&AudioCore::monitorTrackPosition, this);
        t.detach();
    }


    waitForGraphToStop();


    for (size_t i = 0; i < vAudioTracks.size(); i++)
    {
        if (vAudioTracks[i]->sAudioTitle == sTrackTitle)
        {
            if (pCurrentTrack->loadAudioFile(vAudioTracks[i]->sPathToAudioFile, true, pMix))
            {
                return;
            }

            applyAudioEffects();


            if (pCurrentTrack->playSound())
            {
                return;
            }

            bLoadedTrackAtLeastOneTime = true;
            currentTrackState = CTS_PLAYING;

            pMainWindow->changePlayButtonStyle(true, bCalledFromOtherThread);



            // Add to history.

            bool bNewTrack = false;

            if (vPlayedHistory.size() > 0)
            {
                if (vPlayedHistory.back() != vAudioTracks[i])
                {
                    if (vPlayedHistory.size() == MAX_HISTORY_SIZE)
                    {
                        vPlayedHistory.erase(vPlayedHistory.begin());
                    }

                    vPlayedHistory.push_back(vAudioTracks[i]);

                    bNewTrack = true;
                }
            }
            else
            {
                vPlayedHistory.push_back(vAudioTracks[i]);

                bNewTrack = true;
            }


            // Show track on screen.

            pMainWindow->setTrackInfo(vAudioTracks[i]->sAudioTitle, getTrackInfo(vAudioTracks[i]));



            if (bNewTrack)
            {
                // New track: start drawing graph.

                promiseFinishDrawGraph = std::promise<bool>();

                std::thread t (&AudioCore::drawGraph, this);
                t.detach();
            }


            break;
        }
    }
}

void AudioCore::playTrack(bool bCalledFromOtherThread)
{
    mtxProcess.lock();

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

        mtxProcess.unlock();
    }
    else if (vAudioTracks.size() > 0)
    {
        mtxProcess.unlock();

        playTrack(vAudioTracks[0]->sAudioTitle, bCalledFromOtherThread);

        pMainWindow->setNewPlayingTrack(vAudioTracks[0]->pTrackWidget, bCalledFromOtherThread);
        pMainWindow->changePlayButtonStyle(true, bCalledFromOtherThread);
    }
    else
    {
        mtxProcess.unlock();
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

        pMainWindow->setCurrentPos(0.0, "");
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
        if (vAudioTracks.size() == 1)
        {
            mtxProcess.unlock();

            stopTrack(bCalledFromOtherThread);
            playTrack(bCalledFromOtherThread);

            return;
        }

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



    waitForGraphToStop();
    pMainWindow->clearGraph();
}

void AudioCore::setVolume(int iVolume)
{
    pAudioEngine->setMasterVolume(iVolume / 100.0f);
}

CurrentEffects *AudioCore::getCurrentEffects()
{
    return &effects;
}

void AudioCore::setPitch(float fPitch)
{
    effects.fPitchInSemitones = fPitch;

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        std::lock_guard<std::mutex> lock(mtxProcess);
        applyAudioEffects();
    }
}

void AudioCore::setReverbVolume(float fVolume)
{
    effects.fReverbVolume = fVolume;

    if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
    {
        std::lock_guard<std::mutex> lock(mtxProcess);
        applyAudioEffects();
    }
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

void AudioCore::drawGraph()
{
    SSoundInfo info;
    pCurrentTrack->getSoundInfo(info);

    if (info.iBitsPerSample != 16 && info.iBitsPerSample != 24 && info.iBitsPerSample != 32)
    {
        pMainWindow->showMessageBox(L"Error", L"An error occurred at AudioCore::drawGraph(): unsupported sample format, "
                                              "the sample size is " + std::to_wstring(info.iBitsPerSample) + L" bits.", true, true);
        return;
    }


    mtxDrawGraph.lock();

    bDrawingGraph = true;

    mtxDrawGraph.unlock();


    pMainWindow->clearGraph();





    unsigned int iDivideSampleCount = 100;
    // so we calculate 'iSamplesInOne' like this:
    // 3000 (samples in 1 graph sample) - 6000 (sec.)
    // x    (samples in 1 graph sample) - track length (in sec.)
    iDivideSampleCount = static_cast<unsigned int>(3000 * info.dSoundLengthInSec / 6000);


    unsigned int iSampleCount = static_cast<unsigned int>(info.dSoundLengthInSec * info.iSampleRate);
    iSampleCount /= iDivideSampleCount; // approximate amount (will be corrected later)

    pMainWindow->setMaxXToGraph(iSampleCount);



    bool bReachedEOF = true;
    bool bEOF = false;
    bool bClearVector = true;

    std::vector<unsigned char> vWaveData;
    std::vector<float> vSamplesForGraph;
    unsigned int iActualSampleCount = 0;
    unsigned int iSampleReadCountInOneRead = 30;
    unsigned int iCurrentSampleReadCount = 0;

    do
    {
        if (bClearVector)
        {
            vWaveData.clear();
        }
        else
        {
            bClearVector = true;
        }

        bool bExit = false;
        iCurrentSampleReadCount = 0;
        do
        {
            if (pCurrentTrack->readWaveData(&vWaveData, bEOF))
            {
                bReachedEOF = false;
                bExit = true;

                break;
            }
            else
            {
                iCurrentSampleReadCount++;

                if (bEOF)
                {
                    break;
                }
            }
        }while(iSampleReadCountInOneRead != iCurrentSampleReadCount);

        if (bExit) break;


        if ((vWaveData.size() < (info.iChannels * iDivideSampleCount)) && (bEOF == false))
        {
            bClearVector = false;
        }

        if (bEOF && vWaveData.size() == 0)
        {
            break;
        }


        if (bClearVector)
        {
            std::vector<float> vSampleData;

            for (size_t i = 0; i < vWaveData.size(); i += (info.iBitsPerSample / 8) * info.iChannels)
            {
                std::vector<float> vSamplesChannels;

                for (unsigned short j = 0; j < info.iChannels; j++)
                {
                    float fSample = 0.0f;

                    if (info.iBitsPerSample == 16)
                    {
                        fSample = read16bitSample(vWaveData[i + (info.iBitsPerSample / 8) * j], vWaveData[i + 1 + (info.iBitsPerSample / 8) * j]);
                    }
                    else if (info.iBitsPerSample == 24)
                    {
                        fSample = read24bitSample(vWaveData[i +     (info.iBitsPerSample / 8) * j],
                                                  vWaveData[i + 1 + (info.iBitsPerSample / 8) * j],
                                                  vWaveData[i + 2 + (info.iBitsPerSample / 8) * j]);
                    }
                    else // 32 bit
                    {
                        fSample = read32bitSample(vWaveData[i +     (info.iBitsPerSample / 8) * j],
                                                  vWaveData[i + 1 + (info.iBitsPerSample / 8) * j],
                                                  vWaveData[i + 2 + (info.iBitsPerSample / 8) * j],
                                                  vWaveData[i + 3 + (info.iBitsPerSample / 8) * j]);
                    }

                    vSamplesChannels.push_back(fSample);
                }

                // sample is normalized in [-1.0f, 1.0f]
                // combine all channels into 1 sample

                vSampleData.push_back(vSamplesChannels[0]);

                for (size_t k = 1; k < vSamplesChannels.size(); k++)
                {
                    if (abs(vSamplesChannels[k]) > abs(vSampleData.back()))
                    {
                        vSampleData.back() = vSamplesChannels[k];
                    }
                }
            }




            bool bPlusOnly = vSampleData[0] > 0.0f ? true : false;

            for (size_t i = 0; i < vSampleData.size();)
            {
                float fResultSample = vSampleData[i];
                i++;

                for (size_t j = 1; (j < iDivideSampleCount) && (i < vSampleData.size()); j++, i++)
                {
                    if (bPlusOnly)
                    {
                        if (vSampleData[i] > fResultSample)
                        {
                            fResultSample = vSampleData[i];
                        }
                    }
                    else
                    {
                        if (vSampleData[i] < fResultSample)
                        {
                            fResultSample = vSampleData[i];
                        }
                    }
                }

                bPlusOnly = !bPlusOnly;

                vSamplesForGraph.push_back((fResultSample / 2) + 0.5f); // convert to [0.0f; 1.0f]
            }



            iActualSampleCount += vSamplesForGraph.size();


            pMainWindow->addWaveDataToGraph(vSamplesForGraph);
            vSamplesForGraph.clear();



            if (bEOF)
            {
                break;
            }
        }

        mtxDrawGraph.lock();
        if (bDrawingGraph == false)
        {
            mtxDrawGraph.unlock();

            bReachedEOF = false;

            break;
        }
        else
        {
            mtxDrawGraph.unlock();
        }
    }while(true);


    if (bReachedEOF)
    {
        if (vSamplesForGraph.size() > 0)
        {
            pMainWindow->addWaveDataToGraph(vSamplesForGraph);
        }

        pMainWindow->setMaxXToGraph(iActualSampleCount);
    }


    mtxDrawGraph.lock();

    bDrawingGraph = false;

    mtxDrawGraph.unlock();


    promiseFinishDrawGraph.set_value(false);
}

void AudioCore::waitForGraphToStop()
{
    // Wait for the draw thread to finish.
    mtxDrawGraph.lock();

    if (bDrawingGraph)
    {
        std::future<bool> f = promiseFinishDrawGraph.get_future();

        bDrawingGraph = false;
        mtxDrawGraph.unlock();

        f.get();
    }
    else
    {
        mtxDrawGraph.unlock();
    }
}

void AudioCore::applyAudioEffects()
{
    pCurrentTrack->setPitchInSemitones(effects.fPitchInSemitones);
    pMix->setFXVolume(effects.fReverbVolume);
}

void AudioCore::monitorTrackPosition()
{
    promiseFinishMonitorTrackPos = std::promise<bool>();

    bMonitorRunning = true;

    while (bDestroyCalled == false)
    {
        mtxProcess.lock();

        if (bLoadedTrackAtLeastOneTime && currentTrackState != CTS_DELETED)
        {
            if (currentTrackState == CTS_PLAYING)
            {
                double dPos;
                pCurrentTrack->getPositionInSec(dPos);

                SSoundInfo info;
                pCurrentTrack->getSoundInfo(info);


                std::string sTime = getTimeString(dPos);


                pMainWindow->setCurrentPos(dPos / info.dSoundLengthInSec, sTime);
            }
        }

        mtxProcess.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_TRACK_POS_IN_MS));
    }

    bMonitorRunning = false;

    promiseFinishMonitorTrackPos.set_value(false);
}

std::string AudioCore::getTimeString(double dTimeInSec)
{
    unsigned int iSeconds = static_cast<unsigned int>(std::round(dTimeInSec));
    unsigned int iMinutes = iSeconds / 60;
    iSeconds -= (iMinutes * 60);

    std::string sTime = "";
    sTime += std::to_string(iMinutes);
    sTime += ":";
    if (iSeconds < 10) sTime += "0";
    sTime += std::to_string(iSeconds);

    return sTime;
}

float AudioCore::read16bitSample(unsigned char iByte1, unsigned char iByte2)
{
    float fSample = 0.0f;

    short int iSample = 0;

    std::memcpy(reinterpret_cast<char*>(&iSample),     &iByte1, 1);
    std::memcpy(reinterpret_cast<char*>(&iSample) + 1, &iByte2, 1);

    // normalize to [-1.0f, 1.0f]
    fSample = static_cast<float>(iSample) / SHRT_MAX;

    return fSample;
}

float AudioCore::read24bitSample(unsigned char iByte1, unsigned char iByte2, unsigned char iByte3)
{
    float fSample = 0.0f;

    // interpret 24 bit as int 32
    int iSample = ( (iByte3 << 24) | (iByte2 << 16) | (iByte1 << 8) ) >> 8;

    // normalize to [-1.0f, 1.0f]
    fSample = static_cast<float>(iSample) / 16777216; // 2^24

    return fSample;
}

float AudioCore::read32bitSample(unsigned char iByte1, unsigned char iByte2, unsigned char iByte3, unsigned char iByte4)
{
    float fSample = 0.0f;

    int iSample = 0;

    std::memcpy(reinterpret_cast<char*>(&iSample),     &iByte1, 1);
    std::memcpy(reinterpret_cast<char*>(&iSample) + 1, &iByte2, 1);
    std::memcpy(reinterpret_cast<char*>(&iSample) + 2, &iByte3, 1);
    std::memcpy(reinterpret_cast<char*>(&iSample) + 3, &iByte4, 1);

    // normalize to [-1.0f, 1.0f]
    fSample = static_cast<float>(iSample) / INT_MAX;

    return fSample;
}

AudioCore::~AudioCore()
{
    std::future<bool> f = promiseFinishMonitorTrackPos.get_future();

    bDestroyCalled = true;

    if (currentTrackState != CTS_DELETED)
    {
        pCurrentTrack->stopSound();
    }

    if (bMonitorRunning)
    {
        f.get(); // wait for monitorTrackPosition() thread to finish.
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
