﻿// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "ssoundmix.h"

// Custom
#include "AudioEngine/SAudioEngine/saudioengine.h"

SSoundMix::SSoundMix(SAudioEngine* pAudioEngine)
{
    this->pAudioEngine = pAudioEngine;

    pSubmixVoice = nullptr;

    bEffectsSet = false;
}

bool SSoundMix::init(bool bMonoOutput)
{
    this->bMonoOutput = bMonoOutput;

    HRESULT hr = pAudioEngine->pXAudio2Engine->CreateSubmixVoice(&pSubmixVoice, bMonoOutput ? 1 : 2, 44100, 0, 0, 0, 0);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"SSoundMix::init::CreateSubmixVoice()");
        return true;
    }

    return false;
}

bool SSoundMix::setVolume(float fVolume)
{
    HRESULT hr = pSubmixVoice->SetVolume(fVolume);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"SSoundMix::setVolume::SetVolume()");
        return true;
    }


    return false;
}

bool SSoundMix::setAudioEffects(std::vector<SAudioEffect> *pvEffects)
{
    if (bEffectsSet)
    {
        // remove prev chain
        HRESULT hr = pSubmixVoice->SetEffectChain(NULL);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"SSoundMix::setAudioEffects::SetEffectChain() [reset]");
            return true;
        }

        if (pvEffects->size() == 0)
        {
            bEffectsSet = false;
            return false;
        }
    }


    std::vector<IUnknown*> vXAPO(pvEffects->size());
    HRESULT hr = S_OK;

    for (size_t i = 0; i < pvEffects->size(); i++)
    {
        hr = S_OK;

        switch(pvEffects->operator[](i).effectType)
        {
        case(ET_REVERB):
        {
            hr = CreateFX(__uuidof(FXReverb), &vXAPO[i]);
            break;
        }
        case(ET_EQ):
        {
            hr = CreateFX(__uuidof(FXEQ), &vXAPO[i]);
            break;
        }
        case(ET_ECHO):
        {
            hr = CreateFX(__uuidof(FXEcho), &vXAPO[i]);
            break;
        }
        }

        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"SSoundMix::setAudioEffects::CreateFX()");
            return true;
        }
    }


    std::vector<XAUDIO2_EFFECT_DESCRIPTOR> vDescriptors(vXAPO.size());

    for (size_t i = 0; i < vXAPO.size(); i++)
    {
        vDescriptors[i].InitialState = pvEffects->operator[](i).isEnabled();
        vDescriptors[i].OutputChannels = bMonoOutput ? 1 : 2;
        vDescriptors[i].pEffect = vXAPO[i];
    }

    XAUDIO2_EFFECT_CHAIN chain;
    chain.EffectCount = vXAPO.size();
    chain.pEffectDescriptors = &vDescriptors[0];

    hr = pSubmixVoice->SetEffectChain(&chain);
    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"SSoundMix::setAudioEffects::SetEffectChain()");
        return true;
    }


    for (size_t i = 0; i < vXAPO.size(); i++)
    {
        // Releasing the client's reference to the XAPO allows XAudio2 to take ownership of the XAPO.
        vXAPO[i]->Release();
    }


    for (size_t i = 0; i < pvEffects->size(); i++)
    {
        switch(pvEffects->operator[](i).effectType)
        {
        case(ET_REVERB):
        {
            hr = pSubmixVoice->SetEffectParameters(i, &pvEffects->operator[](i).reverbParams, sizeof( FXREVERB_PARAMETERS ), XAUDIO2_COMMIT_NOW);
            break;
        }
        case(ET_EQ):
        {
            hr = pSubmixVoice->SetEffectParameters(i, &pvEffects->operator[](i).eqParams, sizeof( FXEQ_PARAMETERS ), XAUDIO2_COMMIT_NOW);
            break;
        }
        case(ET_ECHO):
        {
            hr = pSubmixVoice->SetEffectParameters(i, &pvEffects->operator[](i).echoParams, sizeof( FXECHO_PARAMETERS ), XAUDIO2_COMMIT_NOW);
            break;
        }
        }

        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"SSoundMix::setAudioEffects::SetEffectParameters()");
            return true;
        }
    }


    bEffectsSet = true;

    return false;
}

bool SSoundMix::setEnableAudioEffect(unsigned int iEffectIndex, bool bEnable)
{
    if (bEffectsSet == false)
    {
        pAudioEngine->showError(L"Sound::setEnableAudioEffect()", L"no effects added, use setAudioEffects() first.");
        return true;
    }

    if (bEnable)
    {
        HRESULT hr = pSubmixVoice->EnableEffect(iEffectIndex);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"SSoundMix::setEnableAudioEffect::EnableEffect()");
            return true;
        }
    }
    else
    {
        HRESULT hr = pSubmixVoice->DisableEffect(iEffectIndex);
        if (FAILED(hr))
        {
            pAudioEngine->showError(hr, L"SSoundMix::setEnableAudioEffect::DisableEffect()");
            return true;
        }
    }

    return false;
}

bool SSoundMix::setAudioEffectParameters(unsigned int iEffectIndex, SAudioEffect *params)
{
    if (bEffectsSet == false)
    {
        pAudioEngine->showError(L"SSoundMix::setAudioEffectParameters()", L"no effects added, use setAudioEffects() first.");
        return true;
    }

    HRESULT hr = S_OK;

    switch(params->effectType)
    {
    case(ET_REVERB):
    {
        hr = pSubmixVoice->SetEffectParameters(iEffectIndex, &params->reverbParams, sizeof( FXREVERB_PARAMETERS ), XAUDIO2_COMMIT_NOW);
        break;
    }
    case(ET_EQ):
    {
        hr = pSubmixVoice->SetEffectParameters(iEffectIndex, &params->eqParams, sizeof( FXEQ_PARAMETERS ), XAUDIO2_COMMIT_NOW);
        break;
    }
    case(ET_ECHO):
    {
        hr = pSubmixVoice->SetEffectParameters(iEffectIndex, &params->echoParams, sizeof( FXECHO_PARAMETERS ), XAUDIO2_COMMIT_NOW);
        break;
    }
    }

    if (FAILED(hr))
    {
        pAudioEngine->showError(hr, L"SSoundMix::setAudioEffectParameters::SetEffectParameters()");
        return true;
    }

    return false;
}

void SSoundMix::getVolume(float &fVolume)
{
    pSubmixVoice->GetVolume(&fVolume);
}

SSoundMix::~SSoundMix()
{
    if (pSubmixVoice)
    {
        pSubmixVoice->DestroyVoice();
    }
}
