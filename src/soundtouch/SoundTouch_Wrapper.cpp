
#include "SoundTouch_Wrapper.h"

#include "SoundTouch.h"

using namespace soundtouch;

void *SoundTouch_init(void) {
    SoundTouch *soundTouch = new SoundTouch();
    soundTouch->setSetting(SETTING_USE_QUICKSEEK, false);
    soundTouch->setSetting(SETTING_USE_AA_FILTER, true);
    soundTouch->setSetting(SETTING_SEQUENCE_MS, 40);
    soundTouch->setSetting(SETTING_SEEKWINDOW_MS, 15);
    soundTouch->setSetting(SETTING_OVERLAP_MS, 8);
    return (void *)soundTouch;
}

void SoundTouch_setSampleRate(void *stouch, unsigned int sampleRate) {
    SoundTouch *soundTouch = (SoundTouch *)stouch;
    soundTouch->setSampleRate(sampleRate);
}

void SoundTouch_setChannels(void *stouch, unsigned int channels) {
    SoundTouch *soundTouch = (SoundTouch *)stouch;
    soundTouch->setChannels(channels);
}

void SoundTouch_setPitchSemiTones(void *stouch, float semiTones) {
    SoundTouch *soundTouch = (SoundTouch *)stouch;
    soundTouch->setPitchSemiTones(semiTones);
}

void SoundTouch_free(void *stouch) {
    SoundTouch *soundTouch = (SoundTouch *)stouch;
    delete soundTouch;
}

void SoundTouch_putSamples(void *stouch, void *samples, unsigned int numSamples) {
    SoundTouch *soundTouch = (SoundTouch *)stouch;
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
    soundTouch->putSamples((SAMPLETYPE *)samples, numSamples);
#else
    soundTouch->putSamples((SAMPLETYPE *)samples, numSamples);
#endif
}

unsigned int SoundTouch_receiveSamples(void *stouch, void *samples, unsigned int maxSamples) {
    SoundTouch *soundTouch = (SoundTouch *)stouch;
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
    return soundTouch->receiveSamples((SAMPLETYPE *)samples, maxSamples);
#else
    return soundTouch->receiveSamples((SAMPLETYPE *)samples, maxSamples);
#endif
}

void SoundTouch_flush(void *stouch) {
    SoundTouch *soundTouch = (SoundTouch *)stouch;
    soundTouch->flush();
}
