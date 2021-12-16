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
    alcCaptureStart(device);
}

void VRMicrophone::startRecording() {
    start();
    doRecord = true;

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
    doStream = true;
    streamMutex = new VRMutex();

    auto recordCb = [&]() {
        while (doStream) {
            ALint Count = 0;
            alGetError();
            alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &Count);

            if (Count > 0) {
                frame = VRSoundBuffer::allocate(2*Count, sample_rate, AL_FORMAT_MONO16);
                alGetError();
                alcCaptureSamples(device, frame->data, Count);

                if (!streamPaused) {
                    VRLock(*streamMutex);
                    frameBuffer.push_back(frame);
                    queuedFrames++;
                    queuedStream = max(queuedStream-1, 0);
                }
            }
        }
    };

    auto streamCb = [&](string address, int port) {
        doStream = recording->setupStream(address, port);

        while (doStream) {
            bool enoughInitialQueuedFrames = bool(queuedFrames >= queueSize);
            bool enoughQueuedFramesInStream = bool(queuedStream >= queueSize - streamBuffer);

            if (enoughInitialQueuedFrames || enoughQueuedFramesInStream || needsFlushing ) {
                VRLock(*streamMutex);
                while (queuedFrames) {
                    auto frame = frameBuffer.front();
                    frameBuffer.pop_front();

                    if (frame) {
                        recording->streamBuffer(frame);
                        queuedFrames = max(queuedFrames-1, 0);
                        if (!needsFlushing) queuedStream++;
                    }
                }
            }

            if (needsFlushing) {
                recording->flushPackets();
                needsFlushing = false;
            }

            duration<double> T(1);
            std::this_thread::sleep_for(T);
        }

        recording->closeStream();
    };

    recordingThread = new thread(recordCb);
    streamingThread = new thread(streamCb, address, port);
}

void VRMicrophone::pauseStreaming(bool p) {
    streamPaused = p;
    queuedStream = 0;
    needsFlushing = true;
}

void VRMicrophone::stop() {
    if (recordingThread) recordingThread->join();
    if (streamingThread) streamingThread->join();
    recordingThread = 0;
    streamingThread = 0;
    alcCaptureStop(device);
    delete streamMutex;
}

VRSoundPtr VRMicrophone::stopRecording() {
    doRecord = false;
    stop();
    auto r = recording;
    recording = 0;
    return r;
}

void VRMicrophone::stopStreaming() {
    doStream = false;
    stop();
    recording = 0;
}


