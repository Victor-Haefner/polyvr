#include "VRMicrophone.h"
#include "VRSound.h"
#include "VRSoundManager.h"
#include "VRSoundUtils.h"

#include "core/utils/VRMutex.h"
#include "core/utils/system/VRSystem.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include <thread>
#include <chrono>

#include "core/utils/toString.h"

const float Pi  = 3.141592653589793f;

using namespace OSG;

VRMicrophone::VRMicrophone() { setup(); }

VRMicrophone::~VRMicrophone() {
    stop();
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

    static int i=0; i++;
    recordingSound = VRSoundManager::get()->setupSound( "mikeRec"+::toString(i) );
    alcCaptureStart(device);
    started = true;
}

void printFrame(VRSoundBufferPtr f) {
    if (f) cout << " frame: " << f->sample_rate << " " << f->format << ", " << f->size << ", " << (int)f->data[0] << endl;
}

VRSoundBufferPtr VRMicrophone::fetchDevicePacket() {
    if (doSim) {
        double t = getTime()*1e-6;
        double dt = t - lastSimTime;

        if (dt > 0.03) {
            alGetError();
            VRSoundBufferPtr frame;
            if (dt < 0.2) frame = genPacket(dt);
            lastSimTime = t;
            return frame;
        }

        return 0;
    }

    ALint Count = 0;
    alGetError();
    alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &Count);

    if (Count > 0) {
        auto frame = VRSoundBuffer::allocate(Count*2, sample_rate, AL_FORMAT_MONO16);
        alGetError();
        alcCaptureSamples(device, frame->data, Count);
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
            //printFrame(frame);

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
                        if (recordingSound) recordingSound->streamBuffer(frame);
                        queuedFrames = max(queuedFrames-1, 0);
                        if (!needsFlushing) queuedStream++;
                    }
                }
            }

            if (needsFlushing) {
                if (recordingSound) recordingSound->flushPackets();
                needsFlushing = false;
            }

            duration<double> T(1);
            std::this_thread::sleep_for(T);
        }

        if (recordingSound) recordingSound->closeStream();
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
    if (!started) start();
    doStream = true;
    recordingSound->setupOutStream(address, port);

    if (!recording) startRecordingThread();
    if (!streaming) startStreamingThread();
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
    doRecord = false;
    doStream = false;

    if (recordingThread) {
        recordingThread->join();
        delete recordingThread;
    }

    if (streamingThread) {
        streamingThread->join();
        delete streamingThread;
    }

    recordingThread = 0;
    streamingThread = 0;
    if (deviceOk) alcCaptureStop(device);
}

VRSoundPtr VRMicrophone::stopRecording() {
    stop();
    auto r = recordingSound;
    recordingSound = 0;
    return r;
}

void VRMicrophone::stopStreaming() {
    stop();
    recording = 0;
}

VRSoundBufferPtr VRMicrophone::genPacket(double dt) {
    float Ac = 32760;
    float wc = frequency;
    int sample_rate = 22050;

    size_t buf_size = dt * sample_rate;
    buf_size += buf_size%2;
    auto frame = VRSoundBuffer::allocate(buf_size*sizeof(short), sample_rate, AL_FORMAT_MONO16);

    double H = dt/(buf_size-1);

    double st = 0;
    for(uint i=0; i<buf_size; i++) {
        double k = lastSimTime + double(i)*H;
        double Ak = abs(sin(k/period1));

        st = i*2*Pi/sample_rate + simPhase;
        short v = Ak * Ac * sin( wc*st );
        ((short*)frame->data)[i] = v;
    }
    simPhase = st;
    return frame;
}

void VRMicrophone::simSource(bool active, float freq, float tone, float pause) {
    doSim = active;
    frequency = freq;
    period1 = tone;
    period2 = pause;
}



