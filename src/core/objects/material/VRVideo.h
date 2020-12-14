#ifndef VRVIDEO_H_INCLUDED
#define VRVIDEO_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRStorage.h"
#include <OpenSG/OSGConfig.h>

using namespace std;

class AVFormatContext;
class AVCodecContext;
class AVFrame;

OSG_BEGIN_NAMESPACE;

class VRVideo : public VRStorage {
    private:
        map<int, map<int, VRTexturePtr> > frames; // frames[stream, frame]
        int width;
        int height;
        int NStreams;

        VRMaterialWeakPtr material;
        VRAnimationPtr anim;
        VRAnimCbPtr animCb;

        AVFormatContext* vFile;
        AVCodecContext* vCodec;
        AVFrame* vFrame;

        int getNStreams();
        int getStream(int j);
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
