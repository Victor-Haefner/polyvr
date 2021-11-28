#ifndef VRSOUND_H_INCLUDED
#define VRSOUND_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <list>
#include <vector>
#include "core/math/OSGMathFwd.h"

#include "VRSoundFwd.h"
#include "core/utils/VRFunctionFwd.h"

class AVPacket;
class AVCodecContext;
class AVFormatContext;

typedef signed char ALbyte;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSound {
    private:
        VRSoundInterfacePtr interface;

        struct ALData;
        shared_ptr<ALData> al;
        vector<VRSoundBufferPtr> ownedBuffer;
        int nextBuffer = 0;
        VRUpdateCbWeakPtr callback;

        unsigned int frequency = 0;
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
        Vec3d* pos;
        Vec3d* vel;

        void updateSampleAndFormat();

    public:
        VRSound();
        ~VRSound();
        static VRSoundPtr create();

        void setPath(string path);
        void setLoop(bool loop);
        void setPitch(float pitch);
        void setVolume(float gain);
        void setUser(Vec3d p, Vec3d v);
        void setCallback(VRUpdateCbPtr callback);

        bool isRunning();
        int getState();
        string getPath();

        void play();
        void stop();
        void pause();
        void resume();
        void close();
        void reset();
        bool initiate();
        void playFrame();

        void initWithCodec(AVCodecContext* codec);
        vector<VRSoundBufferPtr> extractPacket(AVPacket* packet);
        void queuePacket(AVPacket* packet);

        void playBuffer(VRSoundBufferPtr frame);
        void addBuffer(VRSoundBufferPtr frame);

        // carrier amplitude, carrier frequency, carrier phase, modulation amplitude, modulation frequency, modulation phase, packet duration
        void synthesize(float Ac = 32760, float wc = 440, float pc = 0, float Am = 0, float wm = 0, float pm = 0, float T = 1);
        vector<short> synthSpectrum(vector<double> spectrum, unsigned int samples, float duration, float fade_factor, bool returnBuffer = false, int maxQueued = -1);
        vector<short> synthBuffer(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float T = 1);
        vector<short> synthBufferForChannel(vector<Vec2d> freqs1, vector<Vec2d> freqs2, int channel, float T = 1);
        void synthBufferOnChannels(vector<vector<Vec2d>> freqs1, vector<vector<Vec2d>> freqs2, float T = 1);

        vector<short> test(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float T = 1);

};

OSG_END_NAMESPACE;

#endif // VRSOUND_H_INCLUDED
