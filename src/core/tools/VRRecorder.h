#ifndef VRRECORDER_H_INCLUDED
#define VRRECORDER_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <string>
#include <vector>

#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/setup/VRSetupFwd.h"

class AVCodec;
class AVCodecContext;
class AVFrame;
class SwsContext;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFrame;

class VRRecorder {
    private:
        int viewID = 0;
        VRViewWeakPtr view;
        vector<VRFrame*> captures;
        int maxFrames = -1;
        bool running = 0;

        AVCodec* codec = 0;
        AVCodecContext* codec_context = 0;
        AVFrame* frame = 0;
        SwsContext* sws_context = NULL;

        VRToggleCbPtr toggleCallback;
        VRUpdateCbPtr updateCallback;

        void initCodec();
        void closeCodec();
        void initFrame();
        void closeFrame();

    public:
        VRRecorder();
        ~VRRecorder();
        static shared_ptr<VRRecorder> create();

        void setView(int i);
        void capture();
        void compile(string path);
        void clear();
        bool isRunning();
        int getRecordingSize();
        float getRecordingLength();
        void setMaxFrames(int maxf);
        bool frameLimitReached();
        void setTransform(VRTransformPtr t, int f);
        Vec3f getFrom(int f);
        Vec3f getDir(int f);
        Vec3f getAt(int f);
        Vec3f getUp(int f);
        VRTexturePtr get(int f);

        void setRecording(bool b);
        string getPath();

        weak_ptr<VRFunction<bool> > getToggleCallback();
};

OSG_END_NAMESPACE;

#endif // VRRECORDER_H_INCLUDED
