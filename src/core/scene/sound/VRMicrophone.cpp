#include "VRMicrophone.h"
#include "VRSound.h"
#include "VRSoundManager.h"
#include "VRSoundUtils.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include <thread>

using namespace OSG;

VRMicrophone::VRMicrophone() { setup(); }
VRMicrophone::~VRMicrophone() { alcCaptureCloseDevice(device); }

VRMicrophonePtr VRMicrophone::create() { return VRMicrophonePtr( new VRMicrophone() ); }
VRMicrophonePtr VRMicrophone::ptr() { return static_pointer_cast<VRMicrophone>(shared_from_this()); }

void VRMicrophone::setup() {
    alGetError();
    device = alcCaptureOpenDevice(NULL, sample_rate, AL_FORMAT_MONO16, sample_size);
    if (alGetError() != AL_NO_ERROR) cout << "No microphone device found!" << endl;
}

void VRMicrophone::startRecording() {
    //recording = VRSound::create();
    //recording->initiate();
    recording = VRSoundManager::get()->setupSound("");

    doRecord = true;
    alcCaptureStart(device);

    auto recordCb = [&]() {
        while (doRecord) {
            ALint Count = 0;
            alGetError();
            alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &Count);

            if (Count > 0) {
                //frame = VRSoundBuffer::allocate(sample_rate*2*Count, sample_rate, AL_FORMAT_MONO16);
                frame = VRSoundBuffer::allocate(2*Count, sample_rate, AL_FORMAT_MONO16);
                alGetError();
                alcCaptureSamples(device, frame->data, Count);
                recording->addBuffer(frame);
                //recording->playBuffer(frame);
            }
        }
    };

    recordingThread = new thread(recordCb);

    //recordCb();
}

VRSoundPtr VRMicrophone::stopRecording() {
    doRecord = false;
    recordingThread->join();
    recordingThread = 0;
    alcCaptureStop(device);
    auto r = recording;
    recording = 0;
    return r;
}
