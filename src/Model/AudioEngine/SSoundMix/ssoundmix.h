// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>

class SAudioEngine;
class SAudioEffect;

class SSoundMix
{

public:


    bool setVolume(float fVolume);


    // Effects
    // will reset all previous effects, pass empty vector to clear effects
    bool setAudioEffects           (std::vector<SAudioEffect>* pvEffects);
    bool setEnableAudioEffect      (unsigned int iEffectIndex, bool bEnable);
    bool setAudioEffectParameters  (unsigned int iEffectIndex, SAudioEffect* params);


    void getVolume(float& fVolume);



    ~SSoundMix();

private:

    SSoundMix(SAudioEngine* pAudioEngine);

    bool init(bool bMonoOutput = false);

    friend class SAudioEngine;
    friend class SSound;


    class IXAudio2SubmixVoice* pSubmixVoice;

    SAudioEngine* pAudioEngine;


    bool bMonoOutput;
    bool bEffectsSet;
};
