#ifndef VRRECORDER_H_INCLUDED
#define VRRECORDER_H_INCLUDED

#include "core/math/OSGMathFwd.h"
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

        int bitrate; //multiplier
        string codecName;
        static map<string, int> codecs;

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

        void setBitrate(int br);
        int getBitrate();
        void setCodec(string c);
        string getCodec();

        void setViewResolution(string res);
        void enableVSync(bool b);

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
        Vec3d getFrom(int f);
        Vec3d getDir(int f);
        Vec3d getAt(int f);
        Vec3d getUp(int f);
        VRTexturePtr get(int f);

        void setRecording(bool b);
        string getPath();

        weak_ptr<VRFunction<bool> > getToggleCallback();

        static vector<string> getCodecList();
        static vector<string> getResList();
};

OSG_END_NAMESPACE;

#endif // VRRECORDER_H_INCLUDED
