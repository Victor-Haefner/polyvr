#ifndef VRVIDEO_H_INCLUDED
#define VRVIDEO_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRStorage.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGBaseTypes.h>

using namespace std;

class AVFormatContext;
class AVCodecContext;
class AVPacket;
class AVFrame;
class SwsContext;

OSG_BEGIN_NAMESPACE;

class VRVideo : public VRStorage {
    private:
        map<int, map<int, VRTexturePtr> > frames; // frames[stream, frame]
        int width;
        int height;
        int NStreams;

        int currentFrame = 0;
        int cachedFrameMin = 0;
        int cachedFrameMax = 0;

        VRMaterialWeakPtr material;
        VRAnimationPtr anim;
        VRAnimCbPtr animCb;

        AVFormatContext* vFile = 0;
        AVCodecContext* vCodec = 0;
        AVFrame* vFrame = 0;
        AVFrame* nFrame = 0;
        vector<UInt8> osgFrame;
        SwsContext* swsContext = 0;

        int getNStreams();
        int getStream(int j);
        VRTexturePtr convertFrame(AVPacket* packet);
        void frameUpdate(float t, int stream, int N);

    public:
        VRVideo(VRMaterialPtr mat);
        virtual ~VRVideo();

        static VRVideoPtr create(VRMaterialPtr mat);

        void open(string f);
        void close();
        void showFrame(int stream, int frame);
        void play(int stream, float t0, float t1, float v);

        size_t getNFrames(int stream);
        VRTexturePtr getFrame(int stream, int i);
        VRTexturePtr getFrame(int stream, float t);
};

OSG_END_NAMESPACE;

#endif // VRVIDEO_H_INCLUDED
