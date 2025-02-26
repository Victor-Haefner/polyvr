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
class AVCodec;
class AVStream;
class SwsContext;

typedef signed char ALbyte;

OSG_BEGIN_NAMESPACE;

class VRVideoFrame {
    private:
        VRTexturePtr tex;
        bool removalQueued = false;

    public:
        VRVideoFrame();
        ~VRVideoFrame();

        VRTexturePtr getTexture();
        void setTexture(VRTexturePtr t);

        bool isQueuedForRemoval();
        void queueRemoval();
};

class VRVideoStream {
    public:
        struct texData {
            int frameI;
            int width;
            int height;
            int Ncols;
            vector<uint8_t> data;
        };

    public:
        AVCodecContext* vCodec = 0;
        double fps = 0;

        AVFrame* vFrame = 0;
        SwsContext* swsContext = 0;
        //AVPacket* packet = 0;
        AVFrame* nFrame = 0;
        vector<UInt8> osgFrame;

        int cacheSize = 100;
        map< int, VRVideoFrame > frames;
        int currentFrame = -1;
        int cachedFrameMin = 0;
        int cachedFrameMax = 0;
        VRMutex osgMutex;

        bool needsFrameUpdate = false;
        bool texDataQueued = false;
        bool needsCleanup = false;
        map<int, texData> texDataPool;
        vector<int> toRemove;

        void setupTexture(int frameI, int width, int height, int Ncols, vector<uint8_t>& data);

    public:
        VRVideoStream();
        VRVideoStream(AVStream* avStream, AVCodecContext* avContext);
        ~VRVideoStream();

        int getFPS();
        void reset();

        /** -= call from video thread =- **/

        void queueFrameUpdate(int frame);
        bool decode(AVPacket* packet);
        bool needsData(int currentF);
        void checkOldFrames(int currentF);

        /** -= call from main thread =- **/

        void updateFrame(VRMaterialPtr material);
        void processFrames();
        void doCleanup();

        VRTexturePtr getTexture(int i);
};

class VRVideo : public VRStorage {
    private:
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
            vector<uint8_t> data;
        };

        map<int, VRVideoStream> vStreams;
        map<int, AStream> aStreams;
        int width = 0;
        int height = 0;
        float volume = 1.0;

        double start_time = 0;
        double duration = 0;

        int cacheSize = 100;
        int audioQueue = 40;
        int currentFrame = -1;
        int currentStream = 0;
        bool interruptCaching = false;

        bool needsCleanup = false;
        vector<pair<int,int>> toRemove;

        bool texDataQueued = false;
        map<int, texData> texDataPool;

        VRMaterialWeakPtr material;
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
        void setupTexture(int stream, int frameI, int width, int height, int Ncols, vector<uint8_t>& data);
        void convertFrame(int stream, AVPacket* packet);
        void frameUpdate(float t, int stream);
        void loadSomeFrames();
        void cacheFrames(VRThreadWeakPtr t);
        void prepareJump();
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
        bool isRunning();
        void goTo(float t);
        void setVolume(float v);

        float getDuration();
        size_t getNFrames(int stream);
        VRTexturePtr getFrame(int stream, int i);
        VRTexturePtr getFrame(int stream, float t);
};

OSG_END_NAMESPACE;

#endif // VRVIDEO_H_INCLUDED
