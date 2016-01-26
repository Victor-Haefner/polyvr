#ifndef VRVIDEO_H_INCLUDED
#define VRVIDEO_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRStorage.h"

using namespace std;

class AVFormatContext;
class AVCodecContext;
class AVFrame;

class VRVideo : public OSG::VRStorage {
    private:
        map<int, map<int, OSG::VRTexturePtr> > frames; // frames[stream, frame]
        int width;
        int height;
        int NStreams;

        OSG::VRMaterialPtr material;

        AVFormatContext* vFile;
        AVCodecContext* vCodec;
        AVFrame* vFrame;

        int getNStreams();
        int getStream(int j);

    public:
        VRVideo(OSG::VRMaterialPtr mat);
        ~VRVideo();

        void open(string f);
        void close();
        void play(int stream, float t0, float t1, float v);

        OSG::VRTexturePtr getFrame(int stream, int i);
        OSG::VRTexturePtr getFrame(int stream, float t);
        int getNFrames();
};

#endif // VRVIDEO_H_INCLUDED
