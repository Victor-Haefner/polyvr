#include "VRRecorder.h"
#include "core/setup/windows/VRView.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/objects/object/VRObject.h"

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
};

VRRecorder::VRRecorder() {
    av_register_all();
    avcodec_register_all();
}

void VRRecorder::setView(int i) {
    view = VRSetupManager::getCurrent()->getView(i);
}

void VRRecorder::capture() {
    if (view == 0) return;
    int ts = VRGlobals::get()->CURRENT_FRAME;
    captures[ts] = new VRFrame();
    captures[ts]->capture = view->grab();
    captures[ts]->timestamp = glutGet(GLUT_ELAPSED_TIME);
}

void VRRecorder::clear() {
    for (auto f : captures) delete f.second;
    captures.clear();
}

int VRRecorder::getRecordingSize() { return captures.size(); }
float VRRecorder::getRecordingLength() {
    if (captures.size() == 0) return 0;
    int t0 = captures.begin()->second->timestamp;
    int t1 = captures.rbegin()->second->timestamp;
    return (t1-t0)*0.001; //seconds
}

void VRRecorder::compile(string path) {
    if (captures.size() == 0) return;
    ImageRecPtr img0 = captures.begin()->second->capture;

    AVCodec* codec;
    AVCodecContext* c= NULL;
    int i, ret, x, y, got_output;
    FILE *f;
    AVFrame *frame;
    AVPacket pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    AVCodecID codec_id = AV_CODEC_ID_MPEG1VIDEO;
    //AVCodecID codec_id = AV_CODEC_ID_H264;
    codec = avcodec_find_encoder(codec_id);
    if (!codec) { fprintf(stderr, "Codec not found\n"); return; }

    c = avcodec_alloc_context3(codec);
    if (!c) { fprintf(stderr, "Could not allocate video codec context\n"); return; }

    c->bit_rate = 400000; /* put sample parameters */
    c->width = img0->getWidth() - img0->getWidth()%2; /* resolution must be a multiple of two */
    c->height = img0->getHeight() - img0->getHeight()%2;
    c->time_base= (AVRational){1,60}; /* frames per second */
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames=1;
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

    /* the image can be allocated by any means and av_image_alloc() is
    * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
    if (ret < 0) { fprintf(stderr, "Could not allocate raw picture buffer\n"); return; }

    //for (i=0; i<25; i++) { /* encode 1 second of video */
    i = 0;
    for (auto cap : captures) {
        ImageRecPtr img = cap.second->capture;
        if (img == 0) continue;
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        /*for (y=0; y<c->height; y++) { // dummy image
            for (x=0; x<c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = img->getData()[y * c->width + x +0];
                frame->data[1][y * frame->linesize[1] + x] = img->getData()[y * c->width + x +1];
                frame->data[2][y * frame->linesize[2] + x] = img->getData()[y * c->width + x +2];
            }
        }*/

        fflush(stdout);
        for (y=0; y<c->height; y++) { // dummy image
         for (x=0; x<c->width; x++) {
             frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
         }
        }


        for (y=0; y<c->height/2; y++) { // Cb and Cr
         for (x=0; x<c->width/2; x++) {
             frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
             frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
         }
        }

        frame->pts = i;

        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) { fprintf(stderr, "Error encoding frame\n"); return; }

        if (got_output) {
         printf("Write frame %3d (size=%5d)\n", i, pkt.size);
         fwrite(pkt.data, 1, pkt.size, f);
         av_free_packet(&pkt);
        }

        i++;
    }

    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        fflush(stdout);

        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) { fprintf(stderr, "Error encoding frame\n"); return; }

        if (got_output) {
         printf("Write frame %3d (size=%5d)\n", i, pkt.size);
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
    printf("\n");
}

OSG_END_NAMESPACE;
