#include "VRRecorder.h"
#include "core/setup/windows/VRView.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
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
        ImageRecPtr capture = 0;
        int timestamp = 0;

        Vec3f f,a,u; // from at up
};

VRRecorder::VRRecorder() {
    av_register_all();
    avcodec_register_all();

    toggleCallback = new VRFunction<bool>("recorder toggle", boost::bind(&VRRecorder::on_record_toggle, this, _1));
    updateCallback = new VRFunction<int>("recorder update", boost::bind(&VRRecorder::capture, this));
}

void VRRecorder::setView(int i) {
    viewID = i;
    if (VRSetupManager::getCurrent() == 0) return;
    view = VRSetupManager::getCurrent()->getView(i);
}

void VRRecorder::setMaxFrames(int maxf) { maxFrames = maxf; }
bool VRRecorder::frameLimitReached() { return ((int)captures.size() == maxFrames); }

void VRRecorder::setTransform(VRTransform* t, int f) {
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
    if (view == 0) view = VRSetupManager::getCurrent()->getView(viewID);
    if (view == 0) return;
    if (frameLimitReached()) return;

    //int ts = VRGlobals::get()->CURRENT_FRAME;
    VRFrame* f = new VRFrame();
    captures.push_back(f);
    f->capture = view->grab();
    f->timestamp = glutGet(GLUT_ELAPSED_TIME);

    VRTransform* t = view->getCamera();
    if (t == 0) return;
    f->f = t->getFrom();
    f->a = t->getAt();
    f->u = t->getUp();
}

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

void VRRecorder::compile(string path) {
    if (captures.size() == 0) return;
    ImageRecPtr img0 = captures[0]->capture;

    /*for (int i=0; i<1; i++) { // test export the first N images
        string pimg = path+"."+toString(i)+".png";
        captures[i]->capture->write(pimg.c_str());
    }*/

    AVCodec* codec;
    AVCodecContext* c= NULL;
    int i, ret, x, y, got_output;
    FILE *f;
    AVFrame *frame;
    AVPacket pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    AVCodecID codec_id = AV_CODEC_ID_MPEG1VIDEO;
    //AVCodecID codec_id = AV_CODEC_ID_H264; // only works with m player??
    codec = avcodec_find_encoder(codec_id);
    if (!codec) { fprintf(stderr, "Codec not found\n"); return; }

    c = avcodec_alloc_context3(codec);
    if (!c) { fprintf(stderr, "Could not allocate video codec context\n"); return; }

    c->width = img0->getWidth();
    c->height = img0->getHeight();
    c->bit_rate = c->width*c->height*5; /* put sample parameters */
	c->time_base.num = 1;
	c->time_base.den = 25;/* frames per second */
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec_id == AV_CODEC_ID_H264) av_opt_set(c->priv_data, "preset", "slow", 0);
    if (avcodec_open2(c, codec, NULL) < 0) { fprintf(stderr, "Could not open codec\n"); return; } /* open codec */

    f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "Could not open %s\n", path.c_str()); return; }

    frame = avcodec_alloc_frame();
    if (!frame) { fprintf(stderr, "Could not allocate video frame\n"); return; }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    /* the image can be allocated by any means && av_image_alloc() is
    * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
    if (ret < 0) { fprintf(stderr, "Could not allocate raw picture buffer\n"); return; }

    for (i=0; i<(int)captures.size(); i++) {
        ImageRecPtr img = captures[i]->capture;
        if (img == 0) continue;
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        const unsigned char* data = img->getData();
        for (y=0; y<c->height; y++) { // Y
         for (x=0; x<c->width; x++) {
            int k = y*c->width + x;
            int r = data[k*3+0];
            int g = data[k*3+1];
            int b = data[k*3+2];
            int Y = 16 + 0.256789063*r + 0.504128906*g + 0.09790625*b;
            frame->data[0][y * frame->linesize[0] + x] = Y;

            if (y%2 == 0 && y%2 == 0) {
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
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) { fprintf(stderr, "Error encoding frame\n"); return; }

        if (got_output) {
         fwrite(pkt.data, 1, pkt.size, f);
         av_free_packet(&pkt);
        }
    }

    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        fflush(stdout);

        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) { fprintf(stderr, "Error encoding frame\n"); return; }

        if (got_output) {
         fwrite(pkt.data, 1, pkt.size, f);
         av_free_packet(&pkt);
        }
    }

    /* add sequence end code to have a real mpeg file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    avcodec_free_frame(&frame);
}

Image* VRRecorder::get(int f) {
    VRFrame* fr = captures[f];
    return fr->capture;
}

void VRRecorder::on_record_toggle(bool b) {
    if (b) VRSceneManager::get()->addUpdateFkt(updateCallback);
    else {
        VRSceneManager::get()->dropUpdateFkt(updateCallback);
        compile("recording_"+toString(VRGlobals::get()->CURRENT_FRAME)+".avi");
        clear();
    }
}

VRFunction<bool>* VRRecorder::getToggleCallback() { return toggleCallback; }

OSG_END_NAMESPACE;
