#ifndef VRVIDEO_H_INCLUDED
#define VRVIDEO_H_INCLUDED

#include "core/utils/VRStorage.h"
#include <OpenSG/OSGFieldContainerFields.h>

namespace OSG {
    class Image;
    OSG_GEN_CONTAINERPTR(Image);
    class VRMaterial;
}

using namespace std;

class AVFormatContext;
class AVCodecContext;
class AVFrame;

class VRVideo : public OSG::VRStorage {
    private:
        map<int, map<int, OSG::ImageRecPtr> > frames; // frames[stream, frame]
        int width;
        int height;
        int NStreams;

        OSG::VRMaterial* material;

        AVFormatContext* vFile;
        AVCodecContext* vCodec;
        AVFrame* vFrame;

        int getNStreams();
        int getStream(int j);

    public:
        VRVideo(OSG::VRMaterial* mat);
        ~VRVideo();

        void open(string f);
        void close();
        void play(int stream, float t0, float t1, float v);

        OSG::ImageRecPtr getFrame(int stream, int i);
        OSG::ImageRecPtr getFrame(int stream, float t);
        int getNFrames();
};

#endif // VRVIDEO_H_INCLUDED
