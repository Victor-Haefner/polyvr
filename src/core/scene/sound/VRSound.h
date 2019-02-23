#ifndef VRSOUND_H_INCLUDED
#define VRSOUND_H_INCLUDED

#include <list>
#include <OpenSG/OSGVector.h>

#include "VRSoundFwd.h"
#include "core/utils/VRFunctionFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSound {
    private:
        struct ALData;
        shared_ptr<ALData> al;
        VRUpdateCbPtr callback;

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

        double synth_t0 = 0;

        bool loop = false;
        float pitch = 1;
        float gain = 1;
        Vec3d pos, vel;

        void playBuffer(vector<short>& buffer, int sample_rate);

    public:
        VRSound();
        ~VRSound();
        static VRSoundPtr create();

        void setPath(string path);
        void setLoop(bool loop);
        void setPitch(float pitch);
        void setGain(float gain);
        void setUser(Vec3d p, Vec3d v);
        void setCallback(VRUpdateCbPtr callback);

        bool isRunning();
        int getState();
        string getPath();

        void play();
        void stop();
        void close();
        void reset();
        void updateSource();
        bool initiate();
        void playLocally();
        void playFrame();

        int getQueuedBuffer();
        void recycleBuffer();
        uint getFreeBufferID();

        // carrier amplitude, carrier frequency, carrier phase, modulation amplitude, modulation frequency, modulation phase, packet duration
        void synthesize(float Ac = 32760, float wc = 440, float pc = 0, float Am = 0, float wm = 0, float pm = 0, float T = 1);
        vector<short> synthSpectrum(vector<double> spectrum, uint samples, float duration, float fade_factor, bool returnBuffer = false);
        vector<short> synthBuffer(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float T = 1);
        vector<short> synthBufferForChannel(vector<Vec2d> freqs1, vector<Vec2d> freqs2, int channel, float T = 1);
        void synthBufferOnChannels(vector<vector<Vec2d>> freqs1, vector<vector<Vec2d>> freqs2, float T = 1);

        vector<short> test(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float T = 1);

};

OSG_END_NAMESPACE;

#endif // VRSOUND_H_INCLUDED
