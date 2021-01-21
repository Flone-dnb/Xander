// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "saudioengine.h"

// STL
#include <fstream>

// Custom
#include "AudioEngine/SSoundMix/ssoundmix.h"
#include "View/MainWindow/mainwindow.h"


SAudioEngine::SAudioEngine(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    pMasteringVoice = nullptr;

    bEngineInitialized = false;
}

bool SAudioEngine::init()
{
    bEngineInitialized = false;

    if (initXAudio2())
    {
        return true;
    }

    if (initWMF())
    {
        return true;
    }

    bEngineInitialized = true;

    return false;
}

bool SAudioEngine::createSoundMix(SSoundMix *&pOutSoundMix, bool bMonoOutput)
{
    pOutSoundMix = new SSoundMix(this);

    if (pOutSoundMix->init(bMonoOutput))
    {
        delete pOutSoundMix;

        return true;
    }


    mtxSoundMix.lock();

    vCreatedSoundMixes.push_back(pOutSoundMix);

    mtxSoundMix.unlock();


    return false;
}

bool SAudioEngine::setMasterVolume(float fVolume)
{
    if (bEngineInitialized == false)
    {
        showError(L"AudioEngine::setMasterVolume()", L"the audio engine is not initialized.");
        return true;
    }

    pMasteringVoice->SetVolume(fVolume);

    return false;
}

bool SAudioEngine::getMasterVolume(float &fVolume)
{
    if (bEngineInitialized == false)
    {
        showError(L"AudioEngine::getMasterVolume()", L"the audio engine is not initialized.");
        return true;
    }

    pMasteringVoice->GetVolume(&fVolume);

    return false;
}

SAudioEngine::~SAudioEngine()
{
    mtxSoundMix.lock();

    for (size_t i = 0; i < vCreatedSoundMixes.size(); i++)
    {
        delete vCreatedSoundMixes[i];
    }

    mtxSoundMix.unlock();



    pMasteringVoice->DestroyVoice();


    pXAudio2Engine->StopEngine();


    pXAudio2Engine->Release();

    MFShutdown();
}

bool SAudioEngine::initXAudio2()
{
    UINT32 iFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
    iFlags = XAUDIO2_DEBUG_ENGINE;
#endif

    CoInitializeEx( 0, COINIT_MULTITHREADED );

    HRESULT hr = XAudio2Create(&pXAudio2Engine, iFlags);
    if (FAILED(hr))
    {
        showError(hr, L"AudioEngine::initXAudio2::XAudio2Create()");
        return true;
    }

#if defined(DEBUG) || defined(_DEBUG)
    XAUDIO2_DEBUG_CONFIGURATION debugConfig = {0};
    debugConfig.BreakMask = XAUDIO2_LOG_ERRORS;
    debugConfig.TraceMask = XAUDIO2_LOG_ERRORS;
    debugConfig.LogFileline = 1;

    pXAudio2Engine->SetDebugConfiguration(&debugConfig);
#endif


    hr = pXAudio2Engine->CreateMasteringVoice(&pMasteringVoice);
    if (FAILED(hr))
    {
        showError(hr, L"AudioEngine::initXAudio2::CreateMasteringVoice()");
        return true;
    }


    return false;
}

bool SAudioEngine::initWMF()
{
    // Initialize WMF.

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        showError(hr, L"AudioEngine::initWMF::MFStartup()");
        return true;
    }


    return false;
}

bool SAudioEngine::initSourceReaderConfig(IMFAttributes*& pSourceReaderConfig)
{
    // Create source reader config.
    // configure to low latency
    HRESULT hr = MFCreateAttributes(&pSourceReaderConfig, 1);
    if (FAILED(hr))
    {
        showError(hr, L"AudioEngine::initSourceReaderConfig::MFCreateAttributes()");
        return true;
    }

    // Enables low-latency processing
    // "Low latency is defined as the smallest possible delay from when the media data is generated (or received) to when it is rendered.
    // Low latency is desirable for real-time communication scenarios.
    // For other scenarios, such as local playback or transcoding, you typically should not enable low-latency mode, because it can affect quality."
    hr = pSourceReaderConfig->SetUINT32(MF_LOW_LATENCY, true);
    if (FAILED(hr))
    {
        showError(hr, L"AudioEngine::initSourceReaderConfig::MFCreateAttributes()");
        return true;
    }


    return false;
}

void SAudioEngine::showError(HRESULT hr, const std::wstring &sPathToFunc)
{
    LPTSTR errorText = NULL;

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPTSTR)&errorText, 0, NULL);

    if (errorText != NULL)
    {
        pMainWindow->showMessageBox(L"Error", L"An error occurred at " + sPathToFunc + L": " + std::wstring(errorText), true);

        LocalFree(errorText);
    }
    else
    {
        pMainWindow->showMessageBox(L"Error", L"An unknown error occurred at " + sPathToFunc + L".", true);
    }
}

void SAudioEngine::showError(const std::wstring& sPathToFunc, const std::wstring& sErrorText)
{
    pMainWindow->showMessageBox(L"Error", L"An error occurred at " + sPathToFunc + L": " + sErrorText, true);
}
