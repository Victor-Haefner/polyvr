#include "VRRecorder.h"
#include "core/setup/windows/VRView.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGImage.h>
#include <GL/glut.h>

extern "C" {
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFrame {
    public:
        VRTexturePtr capture = 0;
        int timestamp = 0;

        int valid = 0;
        int pktSize = 0;
        char* pktData = 0;

        Vec3f f,a,u; // from at up

        VRFrame() {}
        ~VRFrame() { if (pktData) delete pktData; }

        void transcode(AVFrame *frame, AVCodecContext* codec_context, int i) {
            valid = 0;
            if (capture == 0) return;

            AVPacket pkt;
            av_init_packet(&pkt);
            pkt.data = NULL;    // packet data will be allocated by the encoder
            pkt.size = 0;

            const unsigned char* data = capture->getImage()->getData();
            int x,y,got_output,ret;

            for (y=0; y<codec_context->height; y++) { // Y
             for (x=0; x<codec_context->width; x++) {
                int k = y*codec_context->width + x;
                int r = data[k*3+0];
                int g = data[k*3+1];
                int b = data[k*3+2];
                int Y = 16 + 0.256789063*r + 0.504128906*g + 0.09790625*b;
                frame->data[0][y * frame->linesize[0] + x] = Y;

                if (y%2 == 0) {
                 int u = y/2;
                 int v = x/2;
                 int U = 128 + -0.148222656*r + -0.290992188*g + 0.439214844*b;
                 int V = 128 + 0.439214844*r + -0.367789063*g + -0.071425781*b;
                 frame->data[1][u * frame->linesize[1] + v] = U;
                 frame->data[2][u * frame->linesize[2] + v] = V;
                }
             }
            }

            frame->pts = i;

            /* encode the image */
            ret = avcodec_encode_video2(codec_context, &pkt, frame, &valid);
            if (ret < 0) { fprintf(stderr, "Error encoding frame\n"); return; }

            pktSize = pkt.size;
            pktData = new char[pktSize];
            memcpy(pktData, pkt.data, pktSize);

            av_free_packet(&pkt);
            capture = 0;
        }

        void write(FILE* f) {
            if (valid) fwrite(pktData, 1, pktSize, f);
        }
};

VRRecorder::~VRRecorder() { ; }

VRRecorder::VRRecorder() {
    av_register_all();
    avcodec_register_all();

    toggleCallback = VRFunction<bool>::create("recorder toggle", boost::bind(&VRRecorder::setRecording, this, _1));
    updateCallback = VRFunction<int>::create("recorder update", boost::bind(&VRRecorder::capture, this));

    //initCodec();
}

void VRRecorder::setView(int i) {
    viewID = i;
    if (VRSetupManager::getCurrent() == 0) return;
    view = VRSetupManager::getCurrent()->getView(i);
}

void VRRecorder::setMaxFrames(int maxf) { maxFrames = maxf; }
bool VRRecorder::frameLimitReached() { return ((int)captures.size() == maxFrames); }

void VRRecorder::setTransform(VRTransformPtr t, int f) {
    if (f >= (int)captures.size() || f < 0) return;
    VRFrame* fr = captures[f];
    cout << "setTransform " << t->getName() << " " << fr->f << endl;
    t->setFrom(fr->f);
    t->setAt(fr->a);
    t->setUp(fr->u);
}

Vec3f VRRecorder::getFrom(int f) { VRFrame* fr = captures[f]; return fr->f; }
Vec3f VRRecorder::getDir(int f) { VRFrame* fr = captures[f]; Vec3f d = fr->a - fr->f; d.normalize(); return d; }
Vec3f VRRecorder::getAt(int f) { VRFrame* fr = captures[f]; return fr->a; }
Vec3f VRRecorder::getUp(int f) { VRFrame* fr = captures[f]; return fr->u; }

void VRRecorder::capture() {
    auto v = view.lock();
    if (!v) {
        v = VRSetupManager::getCurrent()->getView(viewID);
        view = v;
    }
    if (!v) return;
    if (frameLimitReached()) return;

    //int ts = VRGlobals::get()->CURRENT_FRAME;
    VRFrame* f = new VRFrame();
    captures.push_back(f);
    f->capture = v->grab();
    f->timestamp = glutGet(GLUT_ELAPSED_TIME);

    VRTransformPtr t = v->getCamera();
    if (t == 0) return;
    f->f = t->getFrom();
    f->a = t->getAt();
    f->u = t->getUp();

    if (!frame) initFrame();

    f->transcode(frame, codec_context, captures.size()-1);
}

bool VRRecorder::isRunning() { return running; }

void VRRecorder::clear() {
    for (auto f : captures) delete f;
    captures.clear();
}

int VRRecorder::getRecordingSize() { return captures.size(); }
float VRRecorder::getRecordingLength() {
    if (captures.size() == 0) return 0;
    int t0 = (*captures.begin())->timestamp;
    int t1 = (*captures.rbegin())->timestamp;
    return (t1-t0)*0.001; //seconds
}

void VRRecorder::initCodec() {
    if (captures.size() == 0) { fprintf(stderr, "No initial capture for CODEC init!\n"); return; }
    VRTexturePtr img0 = captures[0]->capture;
    AVCodecID codec_id = AV_CODEC_ID_MPEG2VIDEO;
    //AVCodecID codec_id = AV_CODEC_ID_H264; // only works with m player??
    codec = avcodec_find_encoder(codec_id);
    if (!codec) { fprintf(stderr, "Codec not found\n"); return; }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) { fprintf(stderr, "Could not allocate video codec context\n"); return; }

    codec_context->width = img0->getImage()->getWidth();
    codec_context->height = img0->getImage()->getHeight();
    codec_context->bit_rate = codec_context->width*codec_context->height*5; /* put sample parameters */
	codec_context->time_base.num = 1;
	codec_context->time_base.den = 25;/* frames per second */
    codec_context->gop_size = 10; /* emit one intra frame every ten frames */
    codec_context->max_b_frames = 1;
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    av_opt_set(codec_context->priv_data, "preset", "ultrafast", 0);

    AVDictionary *param = 0;
    av_dict_set(&param, "crf", "0", 0);
    if (avcodec_open2(codec_context, codec, &param) < 0) { fprintf(stderr, "Could not open codec\n"); return; } /* open codec */
}

void VRRecorder::closeCodec() {
    avcodec_close(codec_context);
    av_free(codec_context);
}

void VRRecorder::initFrame() {
    initCodec();
    frame = avcodec_alloc_frame();
    if (!frame) { fprintf(stderr, "Could not allocate video frame\n"); return; }
    frame->format = codec_context->pix_fmt;
    frame->width  = codec_context->width;
    frame->height = codec_context->height;

    /* the image can be allocated by any means && av_image_alloc() is
    * just the most convenient way if av_malloc() is to be used */
    int ret = av_image_alloc(frame->data, frame->linesize, codec_context->width, codec_context->height, codec_context->pix_fmt, 32);
    if (ret < 0) { fprintf(stderr, "Could not allocate raw picture buffer\n"); return; }
}

void VRRecorder::closeFrame() {
    av_freep(&frame->data[0]);
    avcodec_free_frame(&frame);
    frame = 0;
}

void VRRecorder::compile(string path) {
    if (captures.size() == 0) return;

    int i, ret, got_output;
    FILE* f;

    /*for (int i=0; i<1; i++) { // test export the first N images
        string pimg = path+"."+toString(i)+".png";
        captures[i]->capture->write(pimg.c_str());
    }*/

    f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "Could not open %s\n", path.c_str()); return; }

    for (i=0; i<(int)captures.size(); i++) {
        //captures[i]->transcode(frame, codec_context, i);
        captures[i]->write(f);
    }

    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        fflush(stdout);

        AVPacket pkt;
        ret = avcodec_encode_video2(codec_context, &pkt, NULL, &got_output);
        if (ret < 0) { fprintf(stderr, "Error encoding frame\n"); return; }

        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* add sequence end code to have a real mpeg file */
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    closeCodec();
    closeFrame();
}

VRTexturePtr VRRecorder::get(int f) {
    VRFrame* fr = captures[f];
    return fr->capture;
}

void VRRecorder::setRecording(bool b) {
    if (running == b) return;
    running = b;
    if (b) VRSceneManager::get()->addUpdateFkt(updateCallback);
    else VRSceneManager::get()->dropUpdateFkt(updateCallback);
}

string VRRecorder::getPath() { return "recording_"+toString(VRGlobals::get()->CURRENT_FRAME)+".avi"; }

weak_ptr<VRFunction<bool> > VRRecorder::getToggleCallback() { return toggleCallback; }

OSG_END_NAMESPACE;
