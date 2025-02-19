#ifndef VRVIDEO_H_INCLUDED
#define VRVIDEO_H_INCLUDED

#include "VRMaterialFwd.h"
#include "core/utils/VRStorage.h"
#include "core/scene/sound/VRSoundFwd.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGBaseTypes.h>
#include "core/utils/VRMutex.h"

using namespace std;

class AVFormatContext;
class AVCodecContext;
class AVPacket;
class AVFrame;
class SwsContext;

typedef signed char ALbyte;

OSG_BEGIN_NAMESPACE;

class VRVideo : public VRStorage {
    private:
        struct VStream {
            AVCodecContext* vCodec = 0;
            map< int, VRTexturePtr > frames;
            double fps = 0;
            int cachedFrameMax = 0;
            ~VStream();
        };

        struct AStream {
            VRSoundPtr audio;
            map< int, vector<VRSoundBufferPtr> > frames;
            int lastFrameQueued = 0;
            int cachedFrameMax = 0;
            ~AStream();
        };

        struct texData {
            int stream;
            int frameI;
            int width;
            int height;
            int Ncols;
            uint8_t* data = 0;
        };

        map<int, VStream> vStreams;
        map<int, AStream> aStreams;
        int width = 0;
        int height = 0;
        float volume = 1.0;

        double start_time = 0;
        double duration = 0;

        int cacheSize = 100;
        int audioQueue = 40;
        int currentFrame = 0;
        bool interruptCaching = false;

        bool texDataQueued = false;
        vector<texData> texDataPool;
        int texPoolPointer = 0;

        VRMaterialWeakPtr material;
        VRTexturePtr currentTexture;
        VRAnimationPtr anim;
        VRAnimCbPtr animCb;

        bool needsMainUpdate = false;
        VRUpdateCbPtr mainLoopCb;

        AVFormatContext* vFile = 0;
        AVFrame* vFrame = 0;
        AVFrame* nFrame = 0;
        vector<UInt8> osgFrame;
        SwsContext* swsContext = 0;
        AVPacket* packet = 0;

        VRMutex avMutex;
        VRMutex osgMutex;
        VRThreadCbPtr worker;
        int wThreadID = -1;

        int getNStreams();
        int getStream(int j);
        void setupTexture(int stream, int frameI, int width, int height, int Ncols, uint8_t* data);
        void convertFrame(int stream, AVPacket* packet);
        void frameUpdate(float t, int stream);
        void loadSomeFrames();
        void cacheFrames(VRThreadWeakPtr t);

        void mainThreadUpdate();

    public:
        VRVideo(VRMaterialPtr mat);
        virtual ~VRVideo();

        static VRVideoPtr create(VRMaterialPtr mat);

        void open(string f);
        void showFrame(int stream, int frame);
        void play(int stream, float t0, float t1, float v);

        void pause();
        void resume();
        bool isPaused();
        void goTo(float t);
        void setVolume(float v);

        float getDuration();
        size_t getNFrames(int stream);
        VRTexturePtr getFrame(int stream, int i);
        VRTexturePtr getFrame(int stream, float t);
};

OSG_END_NAMESPACE;

#endif // VRVIDEO_H_INCLUDED
