// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>
#include <string>
#include <functional>
#include <future>

// XAudio2
#include <xaudio2.h>
#include <xaudio2fx.h>

// Custom
#include "AudioEngine/SAudioEngine/saudioengine.h"


class SAudioEngine;
class SSoundMix;

struct SSamplePack
{
    std::vector<int16_t> vChannels;
};

enum SSoundState
{
    SS_NOT_PLAYING = 0,
    SS_PLAYING = 1,
    SS_PAUSED = 2
};


struct SSoundInfo
{
    unsigned long long iFileSizeInBytes;
    double         dSoundLengthInSec;
    unsigned int   iBitrate;
    unsigned short iChannels;
    unsigned long  iSampleRate;
    unsigned short iBitsPerSample;
    bool           bUsesVariableBitRate;
};

class SSound
{
public:

    SSound(SAudioEngine* pAudioEngine);
    ~SSound();


    // Resets all sound effects (audio effects (reverb, echo...), volume, pan, pitch, etc.)
    bool loadAudioFile(const std::wstring& sAudioFilePath, bool bStreamAudio, SSoundMix* pOutputToSoundMix = nullptr);


    // will restart the sound if playing, unpause if paused
    bool playSound    ();
    bool pauseSound   ();
    bool unpauseSound ();
    bool stopSound    ();
    bool setPositionInSec (double dPositionInSec);


    // Effects
    // will reset all previous effects, pass empty vector to clear effects
    bool setAudioEffects      (std::vector<SAudioEffect> vEffects);
    bool setEnableAudioEffect (unsigned int iEffectIndex, bool bEnable);
    bool setAudioEffectParameters  (unsigned int iEffectIndex, SAudioEffect& params);


    bool setVolume        (float fVolume);
    // [-32, 32] so [-5, 5] octaves
    bool setPitchInFreqRatio(float fRatio);
    // [-5, 5] octaves so [-60, 60]
    bool setPitchInSemitones(float fSemitones);
    bool setPan           (float fPan);


    void setOnPlayEndCallback(std::function<void(SSound*)> f);


    bool getVolume        (float& fVolume);
    bool getSoundInfo     (SSoundInfo& soundInfo);
    bool getSoundState    (SSoundState& state);
    bool getPositionInSec (double& dPositionInSec);
    // returns valid value only if not using streaming
    bool getLoadedAudioDataSizeInBytes(size_t& iSizeInBytes);
    bool isSoundStoppedManually() const;

private:

    bool loadFileIntoMemory(const std::wstring& sAudioFilePath, std::vector<unsigned char>& vAudioData, WAVEFORMATEX** pFormat, unsigned int& iWaveFormatSize);

    bool createAsyncReader(const std::wstring& sAudioFilePath, IMFSourceReader*& pSourceReader, WAVEFORMATEX** pFormat, unsigned int& iWaveFormatSize);
    bool streamAudioFile(IMFSourceReader* pAsyncReader);
    bool loopStream(IMFSourceReader* pAsyncReader, IXAudio2SourceVoice* pSourceVoice);

    bool createSourceReader(const std::wstring& sAudioFilePath, SourceReaderCallback** pAsyncSourceReaderCallback, IMFSourceReader*& pOutSourceReader, WAVEFORMATEX** pFormat, unsigned int& iWaveFormatSize);

    bool waitForUnpause();


    void onPlayEnd();


    void clearSound();
    void stopStreaming();
    bool readSoundInfo(IMFSourceReader* pSourceReader, WAVEFORMATEX* pFormat);





    SAudioEngine*         pAudioEngine;
    IXAudio2SourceVoice*  pSourceVoice;
    SSoundMix*            pSoundMix;


    Microsoft::WRL::ComPtr<IMFAttributes> pSourceReaderConfig;


    // User callbacks.
    std::function<void(SSound*)> onPlayEndCallback;


    IMFSourceReader*       pAsyncSourceReader;
    SourceReaderCallback   sourceReaderCallback;
    VoiceCallback          voiceCallback;
    bool bStopStreaming = false;
    static const int iMaxBufferDuringStreaming = 3; // see XAUDIO2_MAX_QUEUED_BUFFERS


    // Used in sync mode.
    std::vector<unsigned char> vAudioData;
    std::vector<unsigned char> vSpeedChangedAudioData;
    // Used in async mode (streaming).
    size_t vSizeOfBuffers[iMaxBufferDuringStreaming] = { 0 };
    std::unique_ptr<uint8_t[]> vBuffers[iMaxBufferDuringStreaming];


    std::promise<bool> promiseStreaming;


    HANDLE         hEventUnpauseSound;


    std::mutex     mtxStreamingSwitch;
    std::mutex     mtxSoundState;


    std::wstring   sAudioFileDiskPath;


    XAUDIO2_BUFFER audioBuffer;
    WAVEFORMATEX   soundFormat;
    unsigned int   iWaveFormatSize;
    unsigned long long iCurrentSamplePosition;
    size_t         iLastReadSampleSize;

    SSoundInfo     soundInfo;

    SSoundState    soundState;

    size_t         iCurrentEffectIndex;

    bool           bUseStreaming;
    bool           bCurrentlyStreaming;
    bool           bSoundLoaded;
    bool           bSoundStoppedManually;
    bool           bCalledOnPlayEnd;
    bool           bDestroyCalled;
    bool           bEffectsSet;
};
