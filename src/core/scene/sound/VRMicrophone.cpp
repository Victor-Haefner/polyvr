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

const float Pi  = 3.141592653589793f;

using namespace OSG;

VRMicrophone::VRMicrophone() { setup(); }

VRMicrophone::~VRMicrophone() {
    alcCaptureCloseDevice(device);
    delete streamMutex;
}

VRMicrophonePtr VRMicrophone::create() { return VRMicrophonePtr( new VRMicrophone() ); }
VRMicrophonePtr VRMicrophone::ptr() { return static_pointer_cast<VRMicrophone>(shared_from_this()); }

void VRMicrophone::setup() {
    streamMutex = new VRMutex();

    alGetError();
    device = alcCaptureOpenDevice(NULL, sample_rate, AL_FORMAT_MONO16, sample_size);
    if (alGetError() != AL_NO_ERROR) {
        cout << "No microphone device found!" << endl;

    } else deviceOk = true;
}

void VRMicrophone::start() {
    if (!deviceOk) return;
    if (started) {
        cout << "VRMicrophone::start, skip, already started" << endl;
        return;
    }
    recordingSound = VRSoundManager::get()->setupSound("");
    alcCaptureStart(device);
    started = true;
}

VRSoundBufferPtr VRMicrophone::fetchDevicePacket() {
    if (doSim) return genPacket();

    ALint Count = 0;
    alGetError();
    alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &Count);

    if (Count > 0) {
        auto frame = VRSoundBuffer::allocate(Count*2, sample_rate, AL_FORMAT_MONO16);
        alGetError();
        alcCaptureSamples(device, frame->data, Count);
        //cout << "fetchDevicePacket " << frame->size << endl;
        return frame;
    }

    return 0;
}

void VRMicrophone::startRecording() {
    if (!deviceOk) return;
    start();
    doRecord = true;

    auto recordCb = [&]() {
        while (doRecord) {
            auto frame = fetchDevicePacket();
            if (frame) recordingSound->addBuffer(frame);
        }
    };

    recordingThread = new thread(recordCb);
}

template <typename T>
using duration = std::chrono::duration<T, std::milli>;

void VRMicrophone::startRecordingThread() {
    auto recordCb = [&]() {
        recording = true;

        while (doStream) {
            auto frame = fetchDevicePacket();

            if (frame && !streamPaused) {
                VRLock lock(*streamMutex);
                if (frameBuffer.size() < maxBufferSize) {
                    frameBuffer.push_back(frame);
                    queuedFrames++;
                    queuedFrames = min(int(frameBuffer.size()), queuedFrames);
                    queuedStream = max(queuedStream-1, 0);
                } else {
                    frameBuffer.pop_front();
                    frameBuffer.push_back(frame);
                }
            }
        }

        recording = false;
    };

    recordingThread = new thread(recordCb);
}

void VRMicrophone::startStreamingThread() {
    auto streamCb = [&]() {
        streaming = true;

        while (doStream) {
            bool enoughInitialQueuedFrames = bool(queuedFrames >= queueSize);
            bool enoughQueuedFramesInStream = bool(queuedStream >= queueSize - streamBuffer);

            if (enoughInitialQueuedFrames || enoughQueuedFramesInStream || needsFlushing ) {
                VRLock lock(*streamMutex);
                while (queuedFrames && frameBuffer.size() > 0) {
                    auto frame = frameBuffer.front();
                    frameBuffer.pop_front();

                    if (frame) {
                        //cout << " stream mike buffer " << frame->size << endl;
                        recordingSound->streamBuffer(frame);
                        queuedFrames = max(queuedFrames-1, 0);
                        if (!needsFlushing) queuedStream++;
                    }
                }
            }

            if (needsFlushing) {
                recordingSound->flushPackets();
                needsFlushing = false;
            }

            duration<double> T(1);
            std::this_thread::sleep_for(T);
        }

        recordingSound->closeStream();
        streaming = false;
    };

    streamingThread = new thread(streamCb);
}

void VRMicrophone::startStreamingOver(VRNetworkClientPtr client) {
    if (!deviceOk) return;
    if (!started) start();
    doStream = true;
    recordingSound->addOutStreamClient(client);

    if (!recording) startRecordingThread();
    if (!streaming) startStreamingThread();
}

void VRMicrophone::startStreaming(string address, int port) {
    if (!deviceOk) return;
    start();
    streamMutex = new VRMutex();
    doStream = recordingSound->setupOutStream(address, port);

    startRecordingThread();
    startStreamingThread();
}

void VRMicrophone::pauseStreaming(bool p) {
    VRLock lock(*streamMutex);
    if (streamPaused == p) return;
    cout << "VRMicrophone::pauseStreaming " << p << endl;
    streamPaused = p;
    queuedStream = 0;
    needsFlushing = true;
}

void VRMicrophone::stop() {
    if (recordingThread) recordingThread->join();
    if (streamingThread) streamingThread->join();
    recordingThread = 0;
    streamingThread = 0;
    if (deviceOk) alcCaptureStop(device);
}

VRSoundPtr VRMicrophone::stopRecording() {
    doRecord = false;
    stop();
    auto r = recordingSound;
    recordingSound = 0;
    return r;
}

void VRMicrophone::stopStreaming() {
    doStream = false;
    stop();
    recording = 0;
}

VRSoundBufferPtr VRMicrophone::genPacket() {
    float Ac = 32760;
    float duration = period1+period2;
    float wc = frequency;

    int sample_rate = 22050;
    size_t buf_size = duration * sample_rate;
    buf_size = 208; // TODO: mike bsize..
    buf_size += buf_size%2;
    auto frame = VRSoundBuffer::allocate(buf_size*sizeof(short), sample_rate, AL_FORMAT_MONO16);

    double t = 0;
    for(uint i=0; i<buf_size; i++) {
        double k = double(i)/(buf_size-1);
        double Ak = 0.0;
        if (k < period1*0.1) Ak = k*10.0/period1;
        else if (k < period1*0.9) Ak = 1.0;
        else if (k < period1) Ak = 1.0-(k-period1*0.9)*10.0;
        else Ak = 0.0;

        Ak = 1.0; // TODO

        t = i*2*Pi/sample_rate + simPhase;
        short v = Ak * Ac * sin( wc*t );
        ((short*)frame->data)[i] = v;
    }
    simPhase = t;
    //cout << frame->size << " > " << ((short*)frame->data)[0] << endl;
    return frame;
}

void VRMicrophone::simSource(bool active, float freq, float tone, float pause) {
    doSim = active;
    frequency = freq;
    period1 = tone;
    period2 = pause;
}



