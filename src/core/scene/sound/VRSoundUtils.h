#ifndef VRSOUNDUTILS_H_INCLUDED
#define VRSOUNDUTILS_H_INCLUDED

#include "VRSoundFwd.h"
#include <string>
#include <list>
#include <vector>

typedef int ALenum;
typedef signed char ALbyte;
std::string toString(ALenum a);

#define ALCHECK(x) { \
ALenum error = alGetError(); \
if (error != AL_NO_ERROR) { \
        fprintf(stderr, " Prior runtime error: before executing %s got %s at %s:%d\n", #x, toString(error).c_str(), __FILE__, __LINE__); \
} \
x; \
error = alGetError(); \
if (error != AL_NO_ERROR) { \
        fprintf(stderr, "Runtime error: %s got %s at %s:%d\n", #x, toString(error).c_str(), __FILE__, __LINE__); \
} }

#define ALCHECK_BREAK(x) { \
x; \
ALenum error = alGetError(); \
if (error != AL_NO_ERROR) { \
        fprintf(stderr, "Runtime error: %s got %s at %s:%d\n", #x, toString(error).c_str(), __FILE__, __LINE__); \
        break; \
} }

using namespace std;

class VRSoundBuffer {
    public:
        ALbyte* data = 0; // pointer to buffer
        int size = 0; // size of buffer
        int sample_rate = 0;
        ALenum format = 0x1101; // AL_FORMAT_MONO16; // number of channels
        bool owned = false;

        VRSoundBuffer();
        ~VRSoundBuffer();

        static VRSoundBufferPtr wrap(ALbyte* data, int size, int rate, ALenum format);
        static VRSoundBufferPtr allocate(int size, int rate, ALenum format);
};

class VRSoundInterface {
    private:
        bool initiated = false;
        unsigned int Nbuffers = 50;
        unsigned int source = 0;
        int queuedBuffers = 0;
        unsigned int* buffers = 0;
        list<unsigned int> free_buffers;

    public:
        VRSoundInterface();
        ~VRSoundInterface();

        static VRSoundInterfacePtr create();

        void play();
        void pause();

        void updateSource(float pitch, float gain);
        void checkSource();

        int getQueuedBuffer();
        void recycleBuffer();
        unsigned int getFreeBufferID();
        void queueFrame(VRSoundBufferPtr frame);
};

#endif // VRSOUNDUTILS_H_INCLUDED
