#include "VRMicrophone.h"
#include "VRSound.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include <thread>

using namespace OSG;

VRMicrophone::VRMicrophone() { setup(); }
VRMicrophone::~VRMicrophone() {}

VRMicrophonePtr VRMicrophone::create() { return VRMicrophonePtr( new VRMicrophone() ); }
VRMicrophonePtr VRMicrophone::ptr() { return static_pointer_cast<VRMicrophone>(shared_from_this()); }

void VRMicrophone::setup() {
    alGetError();
    device = alcCaptureOpenDevice(NULL, sample_rate, AL_FORMAT_STEREO16, sample_size);
    if (alGetError() != AL_NO_ERROR) cout << "No microphone device found!" << endl;
}

void VRMicrophone::startRecording() {
    recording = VRSound::create();
    recording->initiateEmpty();

    vector<short> samples;
    samples.resize(sample_rate);

    doRecord = true;
    ALint count;
    alcCaptureStart(device);

    auto recordCb = [&]() {
        while (doRecord) {
            alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &count);
            alcCaptureSamples(device, &samples[0], count);
            recording->addBuffer(samples, sample_rate);
        }
    };

    recordingThread = new thread(recordCb);
}

VRSoundPtr VRMicrophone::stopRecording() {
    doRecord = false;
    recordingThread->join();
    recordingThread = 0;
    alcCaptureStop(device);
    alcCaptureCloseDevice(device);
    auto r = recording;
    recording = 0;
    return r;
}
