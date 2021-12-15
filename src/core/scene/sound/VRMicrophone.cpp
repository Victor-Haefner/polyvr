#include "VRMicrophone.h"
#include "VRSound.h"
#include "VRSoundManager.h"
#include "VRSoundUtils.h"
#include "core/utils/VRMutex.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include <thread>
#include <chrono>

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

void VRMicrophone::start() {
    recording = VRSoundManager::get()->setupSound("");
    doRecord = true;
    alcCaptureStart(device);
}

void VRMicrophone::startRecording() {
    start();

    auto recordCb = [&]() {
        while (doRecord) {
            ALint Count = 0;
            alGetError();
            alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &Count);

            if (Count > 0) {
                frame = VRSoundBuffer::allocate(2*Count, sample_rate, AL_FORMAT_MONO16);
                alGetError();
                alcCaptureSamples(device, frame->data, Count);
                recording->addBuffer(frame);
            }
        }
    };

    recordingThread = new thread(recordCb);
}

template <typename T>
using duration = std::chrono::duration<T, std::milli>;

void VRMicrophone::startStreaming(string address, int port) {
    start();
    streamMutex = new VRMutex();

    auto recordCb = [&]() {
        streamBuffer.resize(4, 0);

        while (doRecord) {
            ALint Count = 0;
            alGetError();
            alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &Count);

            if (Count > 0) {
                frame = VRSoundBuffer::allocate(2*Count, sample_rate, AL_FORMAT_MONO16);
                alGetError();
                alcCaptureSamples(device, frame->data, Count);

                if (!streamPaused) {
                    VRLock(*streamMutex);
                    streamBuffer[nextPointer] = frame;
                    lastPointer = nextPointer;
                    nextPointer = (nextPointer+1)%streamBuffer.size();
                    cout << "rec frame at " << lastPointer << endl;
                }
            }
        }
    };

    auto streamCb = [&](string address, int port) {
        cout << " --- streamCb A1" << endl;
        recording->setupStream(address, port);
        cout << " --- streamCb A2" << endl;

        while (doRecord) {
            if (lastPointer != streamedPointer) {
                VRLock(*streamMutex);
                streamedPointer = lastPointer;
                auto frame = streamBuffer[lastPointer];
                if (frame) {
                    cout << "stream frame at " << lastPointer << endl;
                    recording->streamBuffer(frame);
                }
            }

            duration<double> T(1);
            std::this_thread::sleep_for(T);
        }

        recording->closeStream();
    };

    recordingThread = new thread(recordCb);
    streamingThread = new thread(streamCb, address, port);
}

void VRMicrophone::pauseStreaming(bool p) { streamPaused = p; }

void VRMicrophone::stop() {
    doRecord = false;
    if (recordingThread) recordingThread->join();
    if (streamingThread) streamingThread->join();
    recordingThread = 0;
    streamingThread = 0;
    alcCaptureStop(device);
    delete streamMutex;
}

VRSoundPtr VRMicrophone::stopRecording() {
    stop();
    auto r = recording;
    recording = 0;
    return r;
}

void VRMicrophone::stopStreaming() {
    stop();
    recording = 0;
}


