#ifndef VRVIDEO_H_INCLUDED
#define VRVIDEO_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRStorage.h"
#include "core/scene/sound/VRSoundFwd.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGBaseTypes.h>
#include <boost/thread/mutex.hpp>

using namespace std;

class AVFormatContext;
class AVCodecContext;
class AVPacket;
class AVFrame;
class SwsContext;

OSG_BEGIN_NAMESPACE;

class VRVideo : public VRStorage {
    private:
        struct VStream {
            AVCodecContext* vCodec = 0;
            map<int, VRTexturePtr> frames;
            double fps = 0;
            ~VStream();
        };

        struct AStream {
            VRSoundPtr audio;
        };

        map<int, VStream> vStreams;
        map<int, AStream> aStreams;
        int width = 0;
        int height = 0;
        float volume = 1.0;

        double start_time = 0;
        double duration = 0;

        int cacheSize = 100;
        int currentFrame = 0;
        int cachedFrameMax = 0;

        VRMaterialWeakPtr material;
        VRAnimationPtr anim;
        VRAnimCbPtr animCb;

        AVFormatContext* vFile = 0;
        AVFrame* vFrame = 0;
        AVFrame* nFrame = 0;
        vector<UInt8> osgFrame;
        SwsContext* swsContext = 0;
        AVPacket* packet = 0;

        boost::mutex mutex;
        VRThreadCbPtr worker;
        int wThreadID = 0;

        int getNStreams();
        int getStream(int j);
        VRTexturePtr convertFrame(int stream, AVPacket* packet);
        void frameUpdate(float t, int stream);
        void loadSomeFrames(int stream);
        void cacheFrames(VRThreadWeakPtr t);

    public:
        VRVideo(VRMaterialPtr mat);
        virtual ~VRVideo();

        static VRVideoPtr create(VRMaterialPtr mat);

        void open(string f);
        void showFrame(int stream, int frame);
        void play(int stream, float t0, float t1, float v);

        void setVolume(float v);

        size_t getNFrames(int stream);
        VRTexturePtr getFrame(int stream, int i);
        VRTexturePtr getFrame(int stream, float t);
};

OSG_END_NAMESPACE;

#endif // VRVIDEO_H_INCLUDED
