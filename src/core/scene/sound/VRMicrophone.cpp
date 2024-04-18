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

const double Pi  = 3.141592653589793;

using namespace OSG;

VRMicrophone::VRMicrophone() {
    streamMutex = new VRMutex();
    paramsMutex = new VRMutex();

    setup();
}

VRMicrophone::~VRMicrophone() {
    stop();
    alcCaptureCloseDevice(device);
    if (streamMutex) delete streamMutex;
    if (paramsMutex) delete paramsMutex;
}

VRMicrophonePtr VRMicrophone::create() { return VRMicrophonePtr( new VRMicrophone() ); }
VRMicrophonePtr VRMicrophone::ptr() { return static_pointer_cast<VRMicrophone>(shared_from_this()); }

void VRMicrophone::setSampleRate(int rate) { sample_rate = rate; setup(); }

void VRMicrophone::setup() {
    alGetError();
    if (device) alcCaptureCloseDevice(device);
    device = alcCaptureOpenDevice(NULL, sample_rate, AL_FORMAT_MONO16, sample_rate);
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
	recordingSound->setPath("");
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
        VRSoundBufferPtr frame = VRSoundBuffer::allocate(Count*2, sample_rate, AL_FORMAT_MONO16);
        alGetError();
        alcCaptureSamples(device, frame->data, Count);

		// test if its in stereo, make mono
        /*VRSoundBufferPtr nframe = VRSoundBuffer::allocate(Count*2, sample_rate, AL_FORMAT_MONO16);
		int16_t* src = (int16_t*)frame->data;
		int16_t* dst = (int16_t*)nframe->data;

		for (int i=0; i<Count; i++) {
			dst[i] = src[2*i];
		}
		frame = nframe;*/

        return frame; // frame
    }

    return 0;
}

double VRMicrophone::getAmplitude() {
    VRLock lock(*paramsMutex);
    return currentAmp;
}

void VRMicrophone::startRecording() {
    if (!deviceOk) return;
    start();
    doRecord = true;

    auto recordCb = [&]() {
        while (doRecord) {
            auto frame = fetchDevicePacket();
            if (frame) {
                recordingSound->addBuffer(frame);
                double A = 0;
                int16_t* src = (int16_t*)frame->data;
                size_t Count = frame->size / 2;
                if (Count > 0) {
                    for (int i=0; i<Count; i += 5) A += abs( src[i] );
                    A *= 5.0/Count;
                }
                VRLock lock(*paramsMutex);
                currentAmp = A;
            }
        }
    };

    recordingThread = new thread(recordCb);
}

template <typename T>
using duration = std::chrono::duration<T, std::milli>;

/*void SaveSound(const std::string& Filename, const std::vector<ALshort>& Samples) {
    // On renseigne les paramètres du fichier à créer
    SF_INFO FileInfos;
    FileInfos.channels   = 1;
    FileInfos.samplerate = 44100;
    FileInfos.format     = SF_FORMAT_PCM_16 | SF_FORMAT_WAV;

    // On ouvre le fichier en écriture
    SNDFILE* File = sf_open(Filename.c_str(), SFM_WRITE, &FileInfos);
    if (!File)
        return;

    // Écriture des échantillons audio
    sf_write_short(File, &Samples[0], Samples.size());

    // Fermeture du fichier
    sf_close(File);
}*/

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

void VRMicrophone::startStreamingThread(string method) {
    auto streamCb = [&](string method) {
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
                        if (recordingSound) recordingSound->streamBuffer(frame, method);
                        queuedFrames = max(queuedFrames-1, 0);
                        if (!needsFlushing) queuedStream++;
                    }
                }
            }

            if (needsFlushing) {
                //if (recordingSound) recordingSound->flushPackets(); // this closes the stream!
                needsFlushing = false;
            }

            duration<double> T(1);
            std::this_thread::sleep_for(T);
        }

        if (recordingSound) recordingSound->closeStream();
        streaming = false;
    };

    streamingThread = new thread(streamCb, method);
}

void VRMicrophone::startStreamingOver(VRNetworkClientPtr client, string method) {
    if (!deviceOk) return;
    if (!started) start();
    doStream = true;
    recordingSound->addOutStreamClient(client, method);

    if (!recording) startRecordingThread();
    if (!streaming) startStreamingThread(method);
}

void VRMicrophone::startStreaming(string address, int port, string method) {
    if (!deviceOk) return;
    if (!started) start();
    doStream = true;
    recordingSound->setupOutStream(address, port, method);

    if (!recording) startRecordingThread();
    if (!streaming) startStreamingThread(method);
}

bool VRMicrophone::isStreaming() { return doStream; }

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
    if (deviceOk) {
		alcCaptureStop(device);
		flushDevice();
	}
}

void VRMicrophone::flushDevice() { // doesnt seem to work yet..
	ALCint SamplesAvailable;
	alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, 1, &SamplesAvailable);
	if (SamplesAvailable > 0) {
		vector<ALshort> data;
		data.resize(SamplesAvailable);
		alcCaptureSamples(device, &data[0], SamplesAvailable);
	}
}

VRSoundPtr VRMicrophone::stopRecording() {
    stop();
    auto r = recordingSound;
    recordingSound = 0;
    started = false;
    return r;
}

void VRMicrophone::stopStreaming() {
    stop();
    recording = 0;
}

VRSoundBufferPtr VRMicrophone::genPacket(double T) {
    // tone parameters
    float Ac = 32760;
    float wc = frequency;

    // allocate frame
    size_t buf_size = size_t(T * sample_rate);
    buf_size += buf_size%2;
    auto frame = VRSoundBuffer::allocate(buf_size*sizeof(short), sample_rate, AL_FORMAT_MONO16);

    double dt = T/(buf_size-1);
    double F = simPhase;

    for(uint i=0; i<buf_size; i++) {
        double a = double(i)*dt;
        double k = lastSimTime + a;
        double Ak = abs(sin(k/period1));

        F += wc*2.0*Pi/sample_rate;
        while (F > 2*Pi) F -= 2*Pi;

        short v = short(Ak * Ac * sin( F ));
        ((short*)frame->data)[i] = v;
    }

    simPhase = F;
    return frame;
}

void VRMicrophone::simSource(bool active, float freq, float tone, float pause) {
    doSim = active;
    frequency = freq;
    period1 = tone;
    period2 = pause;
}



