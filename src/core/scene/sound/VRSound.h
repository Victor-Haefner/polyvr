#ifndef VRSOUND_H_INCLUDED
#define VRSOUND_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <list>
#include <vector>
#include "core/math/OSGMathFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/VRObjectFwd.h"

#include "VRSoundFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/networking/VRNetworkingFwd.h"

struct AVPacket;
struct AVCodecContext;
struct AVFormatContext;
struct OutputStream;
struct InputStream;

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
        VRUpdateCbPtr callback;
        vector<VRNetworkClientPtr> udpClients;
        VRUDPServerPtr udpServer;

        unsigned int frequency = 0;
        double duration = 0;
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
        float lpass = 1;
        float hpass = 1;

        PosePtr lastPose;
        VRTransformPtr headBeacon;
        VRTransformPtr poseBeacon;
        float velocity = 0;
        VRUpdateCbPtr poseUpdateCb;

        AVFormatContext* muxer = 0;
        OutputStream* audio_ost = 0;
        InputStream*  audio_ist = 0;

        void updateSampleAndFormat();
        void update3DSound();
        void write_buffer(AVFormatContext *oc, OutputStream *ost, VRSoundBufferPtr buffer);
        string onStreamData(string s, bool stereo);

    public:
        VRSound();
        ~VRSound();
        static VRSoundPtr create();

        VRSoundInterfacePtr getInterface();

        void setPath(string path);
        void setLoop(bool loop);
        void setPitch(float pitch);
        void setVolume(float gain);
        void setBandpass(float lpass, float hpass);
        void setBeacon(VRTransformPtr t, VRTransformPtr head = 0);
        void setCallback(VRUpdateCbPtr callback);

        bool isRunning();
        int getState();
        string getPath();
        double getDuration();

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

        bool setupOutStream(string url, int port, string method = "mp3");
        bool addOutStreamClient(VRNetworkClientPtr client, string method = "mp3");
        void streamBuffer(VRSoundBufferPtr frame, string method = "mp3");
        void closeStream(bool keepOpen = false);
        void flushPackets();

        bool listenStream(int port, bool stereo);
        bool playPeerStream(VRNetworkClientPtr client, bool stereo);

        void exportToFile(string path);
        void streamTo(string url, int port, bool keepOpen = false);
        void writeStreamData(const string& data);

        // carrier amplitude, carrier frequency, carrier phase, modulation amplitude, modulation frequency, modulation phase, packet duration
        void synthesize(float Ac = 32760, float wc = 440, float pc = 0, float Am = 0, float wm = 0, float pm = 0, float T = 1, int maxQueued = -1);
        vector<short> synthSpectrum(vector<double> spectrum, unsigned int samples, float duration, float fade_factor, bool returnBuffer = false, int maxQueued = -1);
        vector<short> synthBuffer(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float T = 1, int maxQueued = -1);
        vector<short> synthBufferForChannel(vector<Vec2d> freqs1, vector<Vec2d> freqs2, int channel, float T = 1, int maxQueued = -1);
        void synthBufferOnChannels(vector<vector<Vec2d>> freqs1, vector<vector<Vec2d>> freqs2, float T = 1, int maxQueued = -1);

        vector<short> test(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float T = 1);
		void testMP3Write();
		vector<float> getBuffer();
};

OSG_END_NAMESPACE;

#endif // VRSOUND_H_INCLUDED
