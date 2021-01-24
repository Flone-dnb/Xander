// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "ssound.h"

// STL
#include <fstream>

// Custom
#include "AudioEngine/SSoundMix/ssoundmix.h"


SSound::SSound(SAudioEngine* pAudioEngine)
{
    this->pAudioEngine = pAudioEngine;

    pAsyncSourceReader = nullptr;

    pSourceVoice = nullptr;

    bSoundLoaded = false;
    bUseStreaming = false;
    bSoundStoppedManually = false;
    bCurrentlyStreaming = false;

    sAudioFileDiskPath = L"";

    iCurrentSamplePosition = 0;

    bCalledOnPlayEnd = false;
    bDestroyCalled = false;
    bEffectsSet = false;

    iCurrentEffectIndex = 0;


    soundState = SS_NOT_PLAYING;


    hEventUnpauseSound = CreateEvent(NULL, FALSE, FALSE, NULL);
}

SSound::~SSound()
{
    bDestroyCalled = true;

    clearSound();

    CloseHandle(hEventUnpauseSound);
}

bool SSound::loadAudioFile(const std::wstring &sAudioFilePath, bool bStreamAudio, SSoundMix* pOutputToSoundMix)
{
    clearSound();

    bSoundLoaded = false;
    iCurrentEffectIndex = 0;
    bEffectsSet = false;


    this->pSoundMix = pOutputToSoundMix;


    IMFAttributes* pConfig = nullptr;
    pAudioEngine->initSourceReaderConfig(pConfig);
    pSourceReaderConfig = pConfig;


    WAVEFORMATEX* waveFormatEx;

    if (bStreamAudio == false)
    {
        if (loadFileIntoMemory(sAudioFilePath, vAudioData, &waveFormatEx, iWaveFormatSize))
        {
            return true;
        }

        soundFormat = *waveFormatEx;
        CoTaskMemFree(waveFormatEx);


        // Create source voice.


        XAUDIO2_VOICE_SENDS sendTo = {0};

        if (pSoundMix)
        {
            XAUDIO2_SEND_DESCRIPTOR d1;
            d1.Flags = 0;
            d1.pOutputVoice = pSoundMix->pSubmixVoice;

            XAUDIO2_SEND_DESCRIPTOR d2;
            d2.Flags = 0;
            d2.pOutputVoice = pSoundMix->pSubmixVoiceFX;

            XAUDIO2_SEND_DESCRIPTOR desc[2] = {d1, d2};

            sendTo.SendCount = 2;
            sendTo.pSends = desc;
        }
        else
        {
            XAUDIO2_SEND_DESCRIPTOR d;
            d.Flags = 0;
            d.pOutputVoice = pAudioEngine->pMasteringVoice;

            sendTo = {1, &d};
        }

        HRESULT hr = pAudioEngine->pXAudio2Engine->CreateSourceVoice(&pSourceVoice, &soundFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, &sendTo, NULL);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::loadAudioFile::CreateSourceVoice()");
            return true;
        }

        ZeroMemory(&audioBuffer, sizeof(XAUDIO2_BUFFER));
        audioBuffer.AudioBytes = static_cast<UINT32>(vAudioData.size());
        audioBuffer.pAudioData = &vAudioData[0];
        audioBuffer.pContext = nullptr;
    }
    else
    {
        if (createAsyncReader(sAudioFilePath, pAsyncSourceReader, &waveFormatEx, iWaveFormatSize))
        {
            return true;
        }

        soundFormat = *waveFormatEx;
        CoTaskMemFree(waveFormatEx);


        readSoundInfo(pAsyncSourceReader, &soundFormat);


        XAUDIO2_VOICE_SENDS sendTo = {0};

        if (pSoundMix)
        {
            XAUDIO2_SEND_DESCRIPTOR d1;
            d1.Flags = 0;
            d1.pOutputVoice = pSoundMix->pSubmixVoice;

            XAUDIO2_SEND_DESCRIPTOR d2;
            d2.Flags = 0;
            d2.pOutputVoice = pSoundMix->pSubmixVoiceFX;

            XAUDIO2_SEND_DESCRIPTOR desc[2] = {d1, d2};

            sendTo.SendCount = 2;
            sendTo.pSends = desc;
        }
        else
        {
            XAUDIO2_SEND_DESCRIPTOR d;
            d.Flags = 0;
            d.pOutputVoice = pAudioEngine->pMasteringVoice;

            sendTo = {1, &d};
        }

        // create the source voice
        HRESULT hr = pAudioEngine->pXAudio2Engine->CreateSourceVoice(&pSourceVoice, &soundFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, &sendTo, NULL);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"SSound::loadAudioFile::CreateSourceVoice() [async]");
            return true;
        }
    }

    this->sAudioFileDiskPath = sAudioFilePath;

    bSoundLoaded = true;
    bUseStreaming = bStreamAudio;


    soundState = SS_NOT_PLAYING;

    return false;
}

bool SSound::playSound()
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::playSound()", L"no sound is loaded.");
        return true;
    }


    if (bCurrentlyStreaming && bUseStreaming)
    {
        bStopStreaming = true;
    }

    if (soundState == SS_PLAYING)
    {
        stopSound();
    }


    bStopStreaming = false;
    bSoundStoppedManually = false;
    bCurrentlyStreaming = false;

    iCurrentSamplePosition = 0;


    if (onPlayEndCallback)
    {
        // Start callback wait.
        std::thread t(&SSound::onPlayEnd, this);
        t.detach();
    }


    if (bUseStreaming)
    {
        std::thread t(&SSound::streamAudioFile, this, pAsyncSourceReader);
        t.detach();
        //streamAudioFile(pAsyncSourceReader);
    }
    else
    {
        audioBuffer.Flags = XAUDIO2_END_OF_STREAM; // OnStreamEnd is triggered when XAudio2 processes an XAUDIO2_BUFFER with the XAUDIO2_END_OF_STREAM flag set.

        // Submit the audio buffer to the source voice.
        // don't delete buffer until stop.
        HRESULT hr = pSourceVoice->SubmitSourceBuffer(&audioBuffer);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::playSound::SubmitSourceBuffer()");
            return true;
        }


        hr = pSourceVoice->Start();
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::playSound::Start()");
            return true;
        }
    }


    soundState = SS_PLAYING;


    return false;
}

bool SSound::pauseSound()
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::pauseSound()", L"no sound is loaded.");
        return true;
    }

    if (soundState == SS_PAUSED || soundState == SS_NOT_PLAYING)
    {
        return false;
    }

    if (bUseStreaming == false)
    {
        XAUDIO2_VOICE_STATE state;
        pSourceVoice->GetState(&state);

        iCurrentSamplePosition = state.SamplesPlayed;
    }

    mtxSoundState.lock();

    soundState = SS_PAUSED;

    mtxSoundState.unlock();

    HRESULT hr = pSourceVoice->Stop();
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"Sound::pauseSound::Stop()");
        return true;
    }

    return false;
}

bool SSound::unpauseSound()
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::unpauseSound()", L"no sound is loaded.");
        return true;
    }

    if (soundState == SS_PLAYING || soundState == SS_NOT_PLAYING)
    {
        return false;
    }

    HRESULT hr = pSourceVoice->Start();
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"Sound::unpauseSound::Stop()");
        return true;
    }


    mtxSoundState.lock();

    soundState = SS_PLAYING;

    mtxSoundState.unlock();

    SetEvent(hEventUnpauseSound);

    return false;
}

bool SSound::stopSound()
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::stopSound()", L"no sound is loaded.");
        return true;
    }

    if (soundState == SS_NOT_PLAYING)
    {
        return false;
    }


    bSoundStoppedManually = true;

    if (onPlayEndCallback && bCalledOnPlayEnd == false)
    {
        SetEvent(voiceCallback.hStreamEnd);
    }


    if (pSourceVoice)
    {
        HRESULT hr = pSourceVoice->Stop();
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::stopSound::Stop()");
            return true;
        }
    }


    if (bUseStreaming)
    {
        // Restart the stream.
        sourceReaderCallback.restart();

        PROPVARIANT var = { 0 };
        var.vt = VT_I8;

        HRESULT hr = pAsyncSourceReader->SetCurrentPosition(GUID_NULL, var);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::playSound::SetCurrentPosition() [restart]");
            return true;
        }
    }

    iCurrentSamplePosition = 0;


    pSourceVoice->FlushSourceBuffers();


    audioBuffer.PlayBegin = 0;


    if (bCurrentlyStreaming && bUseStreaming)
    {
        bStopStreaming = true;

        if (soundState == SS_PAUSED)
        {
            mtxSoundState.lock();

            soundState = SS_PLAYING;

            mtxSoundState.unlock();

            SetEvent(hEventUnpauseSound);
        }

        stopStreaming();
    }


    soundState = SS_NOT_PLAYING;


    return false;
}

bool SSound::setPositionInSec(double dPositionInSec)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::setPositionInSec()", L"no sound is loaded.");
        return true;
    }

    if ((dPositionInSec > soundInfo.dSoundLengthInSec) || (dPositionInSec < 0.0))
    {
        pAudioEngine->showError(L"Sound::setPositionInSec()", L"the specified position is invalid.");
        return true;
    }



    if (bUseStreaming)
    {
        LONGLONG pos = static_cast<LONGLONG>(dPositionInSec * 10000000);

        PROPVARIANT var;
        HRESULT hr = InitPropVariantFromInt64(pos, &var);
        if (SUCCEEDED(hr))
        {
            double dPercent = dPositionInSec / soundInfo.dSoundLengthInSec;
            size_t iSampleCount = static_cast<size_t>(soundInfo.dSoundLengthInSec * soundInfo.iSampleRate);

            iCurrentSamplePosition = static_cast<unsigned long long>(dPercent * iSampleCount);

            hr = pAsyncSourceReader->SetCurrentPosition(GUID_NULL, var);
            PropVariantClear(&var);
        }
        else
        {
            pAudioEngine->showError(hr, L"Sound::setPositionInSec::SetCurrentPosition()");
            return true;
        }
    }
    else
    {
        HRESULT hr = pSourceVoice->Stop();
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::setPositionInSec::Stop()");
            return true;
        }

        hr = pSourceVoice->FlushSourceBuffers();
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::setPositionInSec::FlushSourceBuffers()");
            return true;
        }


        size_t iSampleCount = vAudioData.size() / ((soundInfo.iBitsPerSample / 8) * soundInfo.iChannels);
        double dPercent = dPositionInSec / soundInfo.dSoundLengthInSec;

        iCurrentSamplePosition = static_cast<unsigned long long>(dPercent * iSampleCount);


        audioBuffer.PlayBegin = static_cast<UINT32>(dPercent * iSampleCount);

        hr = pSourceVoice->SubmitSourceBuffer(&audioBuffer);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::setPositionInSec::SubmitSourceBuffer()");
            return true;
        }

        hr = pSourceVoice->Start();
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"Sound::setPositionInSec::Start()");
            return true;
        }
    }


    return false;
}

void SSound::setOnPlayEndCallback(std::function<void (SSound *)> f)
{
    onPlayEndCallback = f;
}

bool SSound::setVolume(float fVolume)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::setVolume()", L"no sound is loaded.");
        return true;
    }


    HRESULT hr = pSourceVoice->SetVolume(fVolume);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"Sound::setVolume::SetVolume()");
        return true;
    }


    return false;
}

bool SSound::setPitchInFreqRatio(float fRatio)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::setPitchInFreqRatio()", L"no sound is loaded.");
        return true;
    }

    if (fRatio > 32.0f)
    {
        fRatio = 32.0f;
    }
    else if (fRatio < 0.03125f)
    {
        fRatio = 0.03125f;
    }

    HRESULT hr = pSourceVoice->SetFrequencyRatio(fRatio);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"Sound::setPitchInFreqRatio::SetFrequencyRatio()");
        return true;
    }

    return false;
}

bool SSound::setPitchInSemitones(float fSemitones)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::setPitchInFreqRation()", L"no sound is loaded.");
        return true;
    }

    if (fSemitones > 60.0f)
    {
        fSemitones = 60.0f;
    }
    else if (fSemitones < -60.0f)
    {
        fSemitones = -60.0f;
    }

    HRESULT hr = pSourceVoice->SetFrequencyRatio(powf(2.0f, fSemitones / 12.0f));
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"Sound::setPitchInSemitones::SetFrequencyRatio()");
        return true;
    }

    return false;
}

bool SSound::setPan(float fPan)
{
    // Get speaker config.

    unsigned long iChannelMask;
    HRESULT hr = pAudioEngine->pMasteringVoice->GetChannelMask( &iChannelMask );
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"SSound::setPan::GetChannelMask()");
        return true;
    }


    // Pan of -1.0 indicates all left speaker,
    // 1.0 is all right speaker, 0.0 is split between left and right
    float fLeft = 0.5f - fPan / 2;
    float fRight = 0.5f + fPan / 2;


    float outputMatrix[8];

    for (int i = 0; i < 8; i++)
    {
        outputMatrix[i] = 0;
    }


    switch (iChannelMask)
    {

    case SPEAKER_MONO:
        outputMatrix[0] = 1.0;
        break;

    case SPEAKER_STEREO:
    case SPEAKER_2POINT1:
    case SPEAKER_SURROUND:
        outputMatrix[0] = fLeft;
        outputMatrix[1] = fRight;
        break;

    case SPEAKER_QUAD:
        outputMatrix[0] = outputMatrix[2] = fLeft;
        outputMatrix[1] = outputMatrix[3] = fRight;
        break;

    case SPEAKER_4POINT1:
        outputMatrix[ 0 ] = outputMatrix[ 3 ] = fLeft;
        outputMatrix[ 1 ] = outputMatrix[ 4 ] = fRight;
        break;

    case SPEAKER_5POINT1:
    case SPEAKER_7POINT1:
    case SPEAKER_5POINT1_SURROUND:
        outputMatrix[ 0 ] = outputMatrix[ 4 ] = fLeft;
        outputMatrix[ 1 ] = outputMatrix[ 5 ] = fRight;
        break;

    case SPEAKER_7POINT1_SURROUND:
        outputMatrix[ 0 ] = outputMatrix[ 4 ] = outputMatrix[ 6 ] = fLeft;
        outputMatrix[ 1 ] = outputMatrix[ 5 ] = outputMatrix[ 7 ] = fRight;
        break;

    }


    XAUDIO2_VOICE_DETAILS voiceDetails;
    pSourceVoice->GetVoiceDetails(&voiceDetails);

    XAUDIO2_VOICE_DETAILS sendVoiceDetails;
    if (pSoundMix)
    {
        pSoundMix->pSubmixVoice->GetVoiceDetails(&sendVoiceDetails);
    }
    else
    {
        pAudioEngine->pMasteringVoice->GetVoiceDetails(&sendVoiceDetails);
    }


    hr = pSourceVoice->SetOutputMatrix(pSoundMix ? pSoundMix->pSubmixVoice : NULL, voiceDetails.InputChannels, sendVoiceDetails.InputChannels, outputMatrix);
    hr = pSourceVoice->SetOutputMatrix(pSoundMix ? pSoundMix->pSubmixVoiceFX : NULL, voiceDetails.InputChannels, sendVoiceDetails.InputChannels, outputMatrix);

    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"SSound::setPan::SetOutputMatrix()");
        return true;
    }

    return false;
}

bool SSound::getVolume(float &fVolume)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::getVolume()", L"no sound is loaded.");
        return true;
    }


    pSourceVoice->GetVolume(&fVolume);


    return false;
}

bool SSound::getSoundInfo(SSoundInfo &soundInfo)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::getSoundFormat()", L"no sound is loaded.");
        return true;
    }

    soundInfo = this->soundInfo;

    return false;
}

bool SSound::getSoundState(SSoundState &state)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::getSoundFormat()", L"no sound is loaded.");
        return true;
    }

    state = soundState;

    return false;
}

bool SSound::getPositionInSec(double &dPositionInSec)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::getSoundFormat()", L"no sound is loaded.");
        return true;
    }


    if (bUseStreaming)
    {
        size_t iSampleCount = static_cast<size_t>(soundInfo.dSoundLengthInSec * soundInfo.iSampleRate);

        double dPercent = iCurrentSamplePosition / static_cast<double>(iSampleCount);

        dPositionInSec = dPercent * soundInfo.dSoundLengthInSec;
    }
    else
    {
        XAUDIO2_VOICE_STATE state;
        pSourceVoice->GetState(&state);

        size_t iSampleCount = vAudioData.size() / ((soundInfo.iBitsPerSample / 8) * soundInfo.iChannels);
        double dPercent = (state.SamplesPlayed + audioBuffer.PlayBegin) / static_cast<double>(iSampleCount);

        dPositionInSec = dPercent * soundInfo.dSoundLengthInSec;
    }


    return false;
}

bool SSound::getLoadedAudioDataSizeInBytes(size_t& iSizeInBytes)
{
    if (bSoundLoaded == false)
    {
        pAudioEngine->showError(L"Sound::getLoadedAudioDataSizeInBytes()", L"no sound is loaded.");
        return true;
    }


    if (bUseStreaming)
    {
        return true;
    }
    else
    {
        iSizeInBytes = vAudioData.size();
    }


    return false;
}

bool SSound::isSoundStoppedManually() const
{
    return bSoundStoppedManually;
}

void SSound::clearSound()
{
    if (bSoundLoaded)
    {
        stopSound();


        sAudioFileDiskPath = L"";

        if (pSourceVoice)
        {
            pSourceVoice->DestroyVoice();
        }

        vAudioData.clear();
        vSpeedChangedAudioData.clear();

        if (pAsyncSourceReader)
        {
            pAsyncSourceReader->Release();
            pAsyncSourceReader = nullptr;
        }
    }
}

void SSound::stopStreaming()
{
    if (bUseStreaming)
    {
        bStopStreaming = true;

        mtxStreamingSwitch.lock();

        if (bCurrentlyStreaming)
        {
            std::future<bool> future = promiseStreaming.get_future();
            mtxStreamingSwitch.unlock();
            future.get();
        }
        else
        {
            mtxStreamingSwitch.unlock();
        }
    }
}

bool SSound::readSoundInfo(IMFSourceReader* pSourceReader, WAVEFORMATEX* pFormat)
{
    soundInfo.iChannels      = pFormat->nChannels;
    soundInfo.iSampleRate    = pFormat->nSamplesPerSec;
    soundInfo.iBitsPerSample = pFormat->wBitsPerSample;


    // Get audio length.

    LONGLONG duration; // in 100-nanosecond units

    PROPVARIANT var;
    HRESULT hr = pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"SSound::readSoundInfo::GetPresentationAttribute() [duration]");
        return true;
    }

    hr = PropVariantToInt64(var, &duration);
    PropVariantClear(&var);

    soundInfo.dSoundLengthInSec = duration / 10000000.0;




    // Get audio bitrate.

    LONG bitrate;

    hr = pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_AUDIO_ENCODING_BITRATE, &var);
    if (FAILED(hr))
    {
        // Not critical (may fail on .ogg).

        //pAudioEngine->showError(hr, L"SSound::readSoundInfo::GetPresentationAttribute() [bitrate]");
        //return true;

        soundInfo.iBitrate = 0;
    }
    else
    {
        hr = PropVariantToInt32(var, &bitrate);
        PropVariantClear(&var);


        soundInfo.iBitrate = static_cast<unsigned int>(bitrate);
    }



    // Get VBR.

    LONG vbr;

    hr = pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_AUDIO_ISVARIABLEBITRATE, &var);
    if (FAILED(hr))
    {
        // Not critical (may fail on .wav).

        //pAudioEngine->showError(hr, L"SSound::readSoundInfo::GetPresentationAttribute() [vbr]");
        //return true;

        soundInfo.bUsesVariableBitRate = false;
    }
    else
    {
        hr = PropVariantToInt32(var, &vbr);
        PropVariantClear(&var);

        soundInfo.bUsesVariableBitRate = static_cast<bool>(vbr);
    }



    // Get file size.

    LONGLONG size;

    hr = pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_TOTAL_FILE_SIZE, &var);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"SSound::readSoundInfo::GetPresentationAttribute() [size]");
        return true;
    }

    hr = PropVariantToInt64(var, &size);
    PropVariantClear(&var);


    soundInfo.iFileSizeInBytes = static_cast<unsigned long long>(size);



    return false;
}

bool SSound::loadFileIntoMemory(const std::wstring &sAudioFilePath, std::vector<unsigned char> &vAudioData, WAVEFORMATEX **pFormat, unsigned int &iWaveFormatSize)
{
    if (pAudioEngine->bEngineInitialized == false)
    {
        pAudioEngine->showError(L"AudioEngine::loadFileIntoMemory()", L"the audio engine is not initialized.");
        return true;
    }


    // Check if the file exists.

    std::ifstream file(sAudioFilePath, std::ios::binary);
    if (file.is_open())
    {
        file.close();
    }
    else
    {
        pAudioEngine->showError(L"AudioEngine::loadFileIntoMemory()", L"the specified file (" + sAudioFilePath +L") does not exist.");
        return true;
    }


    IMFSourceReader* pSourceReaderOut;
    createSourceReader(sAudioFilePath, nullptr, pSourceReaderOut, pFormat, iWaveFormatSize);

    readSoundInfo(pSourceReaderOut, *pFormat);

    Microsoft::WRL::ComPtr<IMFSourceReader> pSourceReader(pSourceReaderOut);


    // Copy data into byte vector.

    Microsoft::WRL::ComPtr<IMFSample> pSample = nullptr;
    Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer = nullptr;
    unsigned char* pLocalAudioData = NULL;
    DWORD iLocalAudioDataLength = 0;


    // Audio stream index.
    DWORD iStreamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;
    HRESULT hr = S_OK;

    while (true)
    {
        DWORD flags = 0;
        hr = pSourceReader->ReadSample(iStreamIndex, 0, nullptr, &flags, nullptr, pSample.GetAddressOf());
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::loadFileIntoMemory::ReadSample()");
            return true;
        }

        // Check whether the data is still valid.
        if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
        {
            break;
        }

        // Check for eof.
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            break;
        }

        if (pSample == nullptr)
        {
            continue;
        }

        // Convert data to contiguous buffer.
        hr = pSample->ConvertToContiguousBuffer(pBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::loadFileIntoMemory::ConvertToContiguousBuffer()");
            return true;
        }

        // Lock buffer and copy data to local memory.
        hr = pBuffer->Lock(&pLocalAudioData, nullptr, &iLocalAudioDataLength);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::loadFileIntoMemory::Lock()");
            return true;
        }

        for (size_t i = 0; i < iLocalAudioDataLength; i++)
        {
            vAudioData.push_back(pLocalAudioData[i]);
        }

        // Unlock the buffer.
        hr = pBuffer->Unlock();
        pLocalAudioData = nullptr;

        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::loadFileIntoMemory::Unlock()");
            return true;
        }
    }

    pSourceReader.Reset();

    return false;
}

bool SSound::createAsyncReader(const std::wstring &sAudioFilePath, IMFSourceReader *&pSourceReader, WAVEFORMATEX **pFormat, unsigned int &iWaveFormatSize)
{
    if (pAudioEngine->bEngineInitialized == false)
    {
        pAudioEngine->showError(L"SSound::createAsyncReader()", L"the audio engine is not initialized.");
        return true;
    }


    // Check if the file exists.

    std::ifstream file(sAudioFilePath, std::ios::binary);
    if (file.is_open())
    {
        file.close();
    }
    else
    {
        pAudioEngine->showError(L"SSound::loadFileIntoMemory()", L"the specified file (" + sAudioFilePath + L") does not exist.");
        return true;
    }


    SourceReaderCallback* pCallback = &sourceReaderCallback;
    createSourceReader(sAudioFilePath, &pCallback, pSourceReader, pFormat, iWaveFormatSize);


    return false;
}

bool SSound::streamAudioFile(IMFSourceReader *pAsyncReader)
{
    mtxStreamingSwitch.lock();

    if (bStopStreaming)
    {
        return false;
    }

    bCurrentlyStreaming = true;

    mtxStreamingSwitch.unlock();


    promiseStreaming = std::promise<bool>();


    DWORD iStreamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;


    pSourceVoice->Start();


    if (loopStream(pAsyncReader, pSourceVoice))
    {
        mtxStreamingSwitch.lock();
        bCurrentlyStreaming = false;
        mtxStreamingSwitch.unlock();
        promiseStreaming.set_value(false);

        return true;
    }


    pAsyncReader->Flush(iStreamIndex);

    pSourceVoice->Stop();


    mtxStreamingSwitch.lock();
    bCurrentlyStreaming = false;
    mtxStreamingSwitch.unlock();
    promiseStreaming.set_value(false);



    return false;
}

bool SSound::loopStream(IMFSourceReader *pAsyncReader, IXAudio2SourceVoice *pSourceVoice)
{
    DWORD streamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;
    HRESULT hr = S_OK;

    DWORD iCurrentStreamBufferIndex = 0;

    size_t iSizeOfSample = (soundInfo.iBitsPerSample / 8) * soundInfo.iChannels;
    iLastReadSampleSize = 0;
    bool bSkippedFirstRead = false;

    while(true)
    {
        if (bStopStreaming)
        {
            // Exit.
            break;
        }


        if (waitForUnpause())
        {
            break;
        }


        hr = pAsyncReader->ReadSample(streamIndex, 0, nullptr, nullptr, nullptr, nullptr);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::loopStream::ReadSample()");
            return true;
        }

        WaitForSingleObject(sourceReaderCallback.hReadSampleEvent, INFINITE);

        if (sourceReaderCallback.bIsEndOfStream)
        {
            // Notify about onPlayEnd.

            XAUDIO2_VOICE_STATE state;
            pSourceVoice->GetState(&state);
            while(state.BuffersQueued > 0)
            {
                WaitForSingleObject(voiceCallback.hBufferEndEvent, INFINITE);

                pSourceVoice->GetState(&state);
            }

            if (onPlayEndCallback)
            {
                SetEvent(voiceCallback.hStreamEnd);
            }

            break;
        }



        // Sample to contiguous buffer.

        Microsoft::WRL::ComPtr<IMFMediaBuffer> pMediaBuffer;
        hr = sourceReaderCallback.sample->ConvertToContiguousBuffer(pMediaBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::loopStream::ConvertToContiguousBuffer()");
            return true;
        }



        // Read buffer.

        unsigned char* pAudioData = nullptr;
        DWORD iSampleBufferSize = 0;

        hr = pMediaBuffer->Lock(&pAudioData, nullptr, &iSampleBufferSize);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::loopStream::Lock()");
            return true;
        }



        // Recreate proper buffer if needed.

        if (vSizeOfBuffers[iCurrentStreamBufferIndex] < iSampleBufferSize)
        {
            vBuffers[iCurrentStreamBufferIndex].reset(new uint8_t[iSampleBufferSize]);
            vSizeOfBuffers[iCurrentStreamBufferIndex] = iSampleBufferSize;
        }

        if (bSkippedFirstRead == false)
        {
            bSkippedFirstRead = true;
        }

        if (bSkippedFirstRead)
        {
            iCurrentSamplePosition += iLastReadSampleSize / iSizeOfSample;
        }

        iLastReadSampleSize = vSizeOfBuffers[iCurrentStreamBufferIndex];



        // Copy data to our buffer.

        std::memcpy(vBuffers[iCurrentStreamBufferIndex].get(), pAudioData, iSampleBufferSize);


        hr = pMediaBuffer->Unlock();
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::loopStream::Unlock()");
            return true;
        }



        // Wait until there is 'iMaxBufferDuringStreaming - 1' buffers in the queue (leave 1 buffer for reader).

        XAUDIO2_VOICE_STATE state;
        while(true)
        {
            pSourceVoice->GetState(&state);

            if (state.BuffersQueued < iMaxBufferDuringStreaming - 1)
            {
                break;
            }

            WaitForSingleObject(voiceCallback.hBufferEndEvent, INFINITE);

            if (waitForUnpause())
            {
                return false;
            }
        }



        // Play audio.

        XAUDIO2_BUFFER buf = { 0 };
        buf.AudioBytes = iSampleBufferSize;
        buf.pAudioData = vBuffers[iCurrentStreamBufferIndex].get();
        pSourceVoice->SubmitSourceBuffer(&buf);



        // Next buffer.

        iCurrentStreamBufferIndex++;
        iCurrentStreamBufferIndex %= iMaxBufferDuringStreaming;
    }

    return false;
}

bool SSound::createSourceReader(const std::wstring &sAudioFilePath, SourceReaderCallback** pAsyncSourceReaderCallback, IMFSourceReader *& pOutSourceReader, WAVEFORMATEX **pFormat, unsigned int &iWaveFormatSize)
{
    HRESULT hr = S_OK;

    if (pAsyncSourceReaderCallback)
    {
        // Set the source reader to asyncronous mode.
        hr = pSourceReaderConfig->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, *pAsyncSourceReaderCallback);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::SetUnknown() [async]");
            return true;
        }
    }

    hr = MFCreateSourceReaderFromURL(sAudioFilePath.c_str(), pSourceReaderConfig.Get(), &pOutSourceReader);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::MFCreateSourceReaderFromURL()");
        return true;
    }


    // Disable all other streams except for the audio stream (read only from audio stream).

    // Disable all streams.
    hr = pOutSourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, false);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::SetStreamSelection()");
        return true;
    }

    // Audio stream index.
    DWORD iStreamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;

    // Enable audio stream.
    hr = pOutSourceReader->SetStreamSelection(iStreamIndex, true);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::SetStreamSelection()");
        return true;
    }



    // Query information about the media file.
    Microsoft::WRL::ComPtr<IMFMediaType> pNativeMediaType;
    hr = pOutSourceReader->GetNativeMediaType(iStreamIndex, 0, pNativeMediaType.GetAddressOf());
    if(FAILED(hr))
    {
        pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::GetNativeMediaType()");
        return true;
    }


    // Make sure that this is really an audio file.
    GUID majorType{};
    hr = pNativeMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
    if (majorType != MFMediaType_Audio)
    {
        pAudioEngine->showError(L"AudioEngine::createSourceReader::GetGUID() [MF_MT_MAJOR_TYPE]", L"the requested file is not an audio file.");
        return true;
    }

    // Check whether the audio file is compressed.
    GUID subType{};
    hr = pNativeMediaType->GetGUID(MF_MT_MAJOR_TYPE, &subType);
    if (subType != MFAudioFormat_Float && subType != MFAudioFormat_PCM)
    {
        // The audio file is compressed. We have to decompress it first.
        // Tell the source reader to decompress it for us. Create the media type for that.

        Microsoft::WRL::ComPtr<IMFMediaType> pPartialType = nullptr;
        hr = MFCreateMediaType(pPartialType.GetAddressOf());
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::MFCreateMediaType()");
            return true;
        }

        // Set the media type to "audio".
        hr = pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::SetGUID() [MF_MT_MAJOR_TYPE]");
            return true;
        }

        // Request uncompressed audio data.
        hr = pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::SetGUID() [MF_MT_SUBTYPE]");
            return true;
        }

        // Submit our request to the source reader.

        hr = pOutSourceReader->SetCurrentMediaType(iStreamIndex, NULL, pPartialType.Get());
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::SetCurrentMediaType()");
            return true;
        }
    }


    // Uncompress the data.
    Microsoft::WRL::ComPtr<IMFMediaType> pUncompressedAudioType = nullptr;
    hr = pOutSourceReader->GetCurrentMediaType(iStreamIndex, pUncompressedAudioType.GetAddressOf());
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::GetCurrentMediaType()");
        return true;
    }

    hr = MFCreateWaveFormatExFromMFMediaType(pUncompressedAudioType.Get(), pFormat, &iWaveFormatSize);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::MFCreateWaveFormatExFromMFMediaType()");
        return true;
    }


    // Ensure the stream is selected.
    hr = pOutSourceReader->SetStreamSelection(iStreamIndex, true);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"AudioEngine::createSourceReader::SetStreamSelection()");
        return true;
    }


    return false;
}

bool SSound::waitForUnpause()
{
    mtxSoundState.lock();
    if (soundState == SS_PAUSED)
    {
        mtxSoundState.unlock();

        WaitForSingleObject(hEventUnpauseSound, INFINITE);
    }
    else
    {
        mtxSoundState.unlock();
    }


    if (bStopStreaming)
    {
        // Exit.
        return true;
    }

    return false;
}

void SSound::onPlayEnd()
{
    do
    {
       WaitForSingleObject(voiceCallback.hStreamEnd, INFINITE);

       if (bDestroyCalled) return;

       if (bSoundStoppedManually) break;

       if (bUseStreaming == false)
       {
           break;
       }
       else if (sourceReaderCallback.bIsEndOfStream)
       {
           break;
       }
    }while(true);


    bCalledOnPlayEnd = true;
    onPlayEndCallback(this);
    bCalledOnPlayEnd = false;
}
