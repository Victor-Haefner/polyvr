#include "VRMicrophone.h"
#include "VRSound.h"
#include "VRSoundUtils.h"

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

    doRecord = true;
    ALint count;
    alcCaptureStart(device);

    auto recordCb = [&]() {
        while (doRecord) {
            auto frame = VRSoundBuffer::allocate(sample_rate*2, sample_rate, AL_FORMAT_STEREO16);
            alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &count);
            alcCaptureSamples(device, frame->data, count);
            recording->addBuffer(frame);
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
