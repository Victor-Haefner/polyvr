#ifndef VRSOUND_H_INCLUDED
#define VRSOUND_H_INCLUDED

#include <list>
#include <OpenSG/OSGVector.h>

#include "VRSoundFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSound {
    private:
        struct ALData;
        shared_ptr<ALData> al;

        int queuedBuffers = 0;
        uint source = 0;
        uint* buffers = 0;
        list<uint> free_buffers;
        uint frequency = 0;
        uint Nbuffers = 50;
        int stream_id = 0;
        int init = 0;
        string path;
        bool interrupt = false;
        bool initiated = false;
        bool doUpdate = false;

        bool loop = false;
        float pitch = 1;
        float gain = 1;
        Vec3f pos, vel;

        void playBuffer(short* buffer, size_t N, int sample_rate);

    public:
        VRSound();
        ~VRSound();
        static VRSoundPtr create();

        void setPath(string path);
        void stop();
        void setLoop(bool loop);
        void setPitch(float pitch);
        void setGain(float gain);
        void setUser(Vec3f p, Vec3f v);

        bool isRunning();
        int getState();
        string getPath();

        void close();

        void reset();

        void updateSource();

        bool initiate();

        void playFrame();
        void play();

        // carrier amplitude, carrier frequency, carrier phase, modulation amplitude, modulation frequency, modulation phase, packet duration
        void synthesize(float Ac = 32760, float wc = 440, float pc = 0, float Am = 0, float wm = 0, float pm = 0, float T = 1);
        void synthesizeSpectrum(double* spectrum, uint samples, float duration, float fade_factor);
        void synthBuffer(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float T = 1);

        int getQueuedBuffer();
        void recycleBuffer();
};

OSG_END_NAMESPACE;

#endif // VRSOUND_H_INCLUDED
