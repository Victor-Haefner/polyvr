#include "VRSoundUtils.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"
#include <OpenSG/OSGVector.h>

#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/efx.h>

#ifdef _WIN32
#include <minmax.h>
#endif

using namespace OSG;

string alToString(ALenum a) {
    switch (a) {
        case AL_NO_ERROR: return "ok";
        case AL_INVALID_NAME: return "invalid name";
        case AL_INVALID_ENUM: return "invalid enum";
        case AL_INVALID_VALUE: return "invalid value";
        case AL_INVALID_OPERATION: return "invalid operation";
        case AL_OUT_OF_MEMORY: return "out of memory";
        default: return "unknown";
    }
    return "unknown";
}

VRSoundBuffer::VRSoundBuffer() {}

VRSoundBuffer::~VRSoundBuffer() {
    if (data && owned) delete[] data;
}

VRSoundBufferPtr VRSoundBuffer::wrap(ALbyte* d, int s, int r, ALenum f) {
    auto b = VRSoundBufferPtr( new VRSoundBuffer() );
    b->data = d;
    b->size = s;
    b->sample_rate = r;
    b->format = f;
    return b;
}

VRSoundBufferPtr VRSoundBuffer::allocate(int s, int r, ALenum f) {
    auto b = VRSoundBufferPtr( new VRSoundBuffer() );
    b->size = s;
    b->sample_rate = r;
    b->format = f;
    b->data = new ALbyte[s];
    b->owned = true;
    return b;
}

VRSoundInterface::VRSoundInterface() {
    buffers = new uint[Nbuffers];

    ALCHECK( alGenBuffers(Nbuffers, buffers) );
    for (uint i=0; i<Nbuffers; i++) free_buffers.push_back(buffers[i]);

    ALCHECK( alGenSources(1u, &source) );
    updateSource(1,1);
}

VRSoundInterface::~VRSoundInterface() {
    if (source) ALCHECK( alDeleteSources(1u, &source));
    if (buffers && Nbuffers) ALCHECK( alDeleteBuffers(Nbuffers, buffers));
    delete[] buffers;
}

VRSoundInterfacePtr VRSoundInterface::create() { return VRSoundInterfacePtr( new VRSoundInterface() ); }

void VRSoundInterface::pause() {
    ALint val = -1;
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    if (val == AL_PLAYING) ALCHECK( alSourcePause(source));
}

void VRSoundInterface::play() {
    ALint val = -1;
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
}

void VRSoundInterface::updatePose(PosePtr pose, float velocity) {
    if (pose) {
        Vec3f pos = Vec3f(pose->pos());
        Vec3f vel = Vec3f(pose->dir())*velocity;
        ALCHECK( alSource3f(source, AL_POSITION, pos[0], pos[1], pos[2]));
        ALCHECK( alSource3f(source, AL_VELOCITY, vel[0], vel[1], vel[2]));
        //cout << "VRSoundInterface::updateSource " << pos << ", " << vel << endl;
    }
}

void VRSoundInterface::updateSource(float pitch, float gain, float lowpass, float highpass) {
    cout << "update source, pitch: " << pitch << " gain: " << gain << endl;
    ALCHECK( alSourcef(source, AL_PITCH, pitch));
    ALCHECK( alSourcef(source, AL_MAX_GAIN, gain));
    ALCHECK( alSourcef(source, AL_GAIN, gain));

    if ((lowpass < 1.0-1e-3 || highpass < 1.0-1e-3) && filter == 0) {
        ALCHECK( alGenFilters(1u, &filter) );
    }

    if (filter > 0) {
        ALCHECK( alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_BANDPASS) );
        //ALCHECK( alFilterf(filter, AL_BANDPASS_GAIN, 0.25f) );
        ALCHECK( alFilterf(filter, AL_BANDPASS_GAIN, (lowpass+highpass)*0.5f) );
        ALCHECK( alFilterf(filter, AL_BANDPASS_GAINLF, lowpass) );
        ALCHECK( alFilterf(filter, AL_BANDPASS_GAINHF, highpass) );
        ALCHECK( alSourcei(source, AL_DIRECT_FILTER, filter) );
    }
}

void VRSoundInterface::queueFrame(VRSoundBufferPtr frame) {
    ALint val = -1;
    ALuint bufid = getFreeBufferID();
    //printBuffer(frame, "queueFrame");
    ALCHECK( alBufferData(bufid, frame->format, frame->data, frame->size, frame->sample_rate) );
    ALCHECK( alSourceQueueBuffers(source, 1, &bufid) );
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val) );
    if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
}

uint VRSoundInterface::getFreeBufferID() {
    recycleBuffer();

    if (free_buffers.size()) {
        queuedBuffers += 1;
        auto bufid = free_buffers.front();
        free_buffers.pop_front();
        return bufid;
    }

    ALuint bufid;
    alGenBuffers(1, &bufid);
    queuedBuffers += 1;
    return bufid;
}

int VRSoundInterface::getQueuedBuffer() { return queuedBuffers; }

void VRSoundInterface::recycleBuffer() {
    ALint Nprocessed = -1;
    ALuint bufid = 0;

     // TODO: not working properly!!
    /*do { ALCHECK_BREAK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &Nprocessed) ); // recycle buffers
        cout << " recycleBuffer Nprocessed " << Nprocessed << endl;
        for(; Nprocessed > 0; --Nprocessed) {
            ALCHECK( alSourceUnqueueBuffers(source, 1, &bufid));
            free_buffers.push_back(bufid);
            if ( queuedBuffers > 0 ) queuedBuffers -= 1;
        }
    } while (Nprocessed > 0);*/

    //do { ALCHECK_BREAK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &Nprocessed) ); } // recycle buffers
    //while (Nprocessed <= 0 && free_buffers.size() == 0); // wait for a buffer to be processed if no free_buffers and none processed
    // TODO: THIS IS BAD!! -> avoid all while loops!!

    ALCHECK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &Nprocessed) );

    //if (Nprocessed <= 0 && free_buffers.size() == 0) { al->state = AL_STOPPED; return; } // no available buffer, stop!
    for(; Nprocessed > 0; --Nprocessed) {
        ALCHECK( alSourceUnqueueBuffers(source, 1, &bufid));
        free_buffers.push_back(bufid);
        queuedBuffers = max(0,queuedBuffers-1);
    }
}


void VRSoundInterface::printBuffer(VRSoundBufferPtr frame, string tag) {
    cout << " -- print buffer info " << tag << endl;
    cout << "  data: " << (void*)frame->data << endl;
    cout << "  size: " << frame->size << endl;
    cout << "  sample_rate: " << frame->sample_rate << endl;
    cout << "  format: " << frame->format << endl;
    cout << "  owned: " << frame->owned << endl;
    cout << " -- done -- " << endl;
}



