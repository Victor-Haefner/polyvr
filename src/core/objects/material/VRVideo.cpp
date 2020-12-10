#include "VRVideo.h"
#include "VRMaterial.h"

#include <OpenSG/OSGImage.h>
#include "core/utils/toString.h"
#include "core/objects/material/VRTexture.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#include <string>

using namespace OSG;

template<> string typeName(const VRVideo& o) { return "Video"; }

VRVideo::VRVideo(VRMaterialPtr mat) {
    material = mat;

    vFile = 0;
    vCodec = 0;
    vFrame = 0;
    NStreams = 0;

    av_register_all(); // Register all formats && codecs

    // test
    const char* file = "~/Videos/drop.avi";
    open(file);
    close();
}

VRVideo::~VRVideo() {}

VRVideoPtr VRVideo::create(VRMaterialPtr mat) { return VRVideoPtr( new VRVideo(mat) ); }

int VRVideo::getNStreams() {
    if (vFile == 0) return 0;

    int k = 0;
    for(int i=0; i<(int)vFile->nb_streams; i++) if(vFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) k++;
    return k;
}

int VRVideo::getStream(int j) {
    if (vFile == 0) return -1;

    int k = 0;
    for(int i=0; i<(int)vFile->nb_streams; i++) if(vFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (k != j) continue;
        return i;
    }

    return -1;
}

void FlipFrame(AVFrame* pFrame) {
    /*for (int i = 0; i < 4; i++) {
        pFrame->data[i] += pFrame->linesize[i] * (pFrame->height-1);
        pFrame->linesize[i] = -pFrame->linesize[i];
    }*/

    for (int i = 0; i < 4; i++) {
        if (i) {
            pFrame->data[i] += pFrame->linesize[i] * ((pFrame->height >> 1)-1);
        } else {
            pFrame->data[i] += pFrame->linesize[i] * (pFrame->height-1);
        }
        pFrame->linesize[i] = -pFrame->linesize[i];
    }
}

void UnflipFrame(AVFrame* pFrame) {
    for (int i = 0; i < 4; i++) {
        pFrame->linesize[i] = -pFrame->linesize[i];
        pFrame->data[i] -= pFrame->linesize[i] * (pFrame->height-1);
    }
}

void VRVideo::open(string f) {
    // open file
    if(avformat_open_input(&vFile, f.c_str(), NULL, NULL)!=0) return; // Couldn't open file
    if(avformat_find_stream_info(vFile, NULL)<0) return; // Couldn't find stream information
    av_dump_format(vFile, 0, f.c_str(), 0); // Dump information about file onto standard error

    NStreams = getNStreams();

    cout << " VRVideo::open " << f << endl;
    cout << "  VRVideo::open " << NStreams << " streams" << endl;

#ifdef OLD_LIBAV
    vFrame = avcodec_alloc_frame(); // Allocate video frame
#else
    vFrame = av_frame_alloc(); // Allocate video frame
#endif

    frames.clear();
    for (int i=0; i<NStreams; i++) {
        int stream = getStream(i);
        vCodec = vFile->streams[stream]->codec;
        if (vCodec == 0) continue;

        // Find the decoder for the video stream
        AVDictionary* optionsDict = 0;
        AVCodec* c = avcodec_find_decoder(vCodec->codec_id);
        if(c == 0) { fprintf(stderr, "Unsupported codec!\n"); return; } // Codec not found
        if(avcodec_open2(vCodec, c, &optionsDict)<0) return; // Could not open codec

        //SwsContext* swsContext = sws_getContext(vCodec->width, vCodec->height, AV_PIX_FMT_YUV420P, vCodec->width, vCodec->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

        cout << "  VRVideo::open stream " << i << endl;
        int valid=0;
        int frame=0;
        for(AVPacket packet; av_read_frame(vFile, &packet)>=0; av_free_packet(&packet) ) { // read stream
            if(packet.stream_index != stream) continue;

            avcodec_decode_video2(vCodec, vFrame, &valid, &packet); // Decode video frame
            if(valid == 0) continue;

            FlipFrame(vFrame);

            width = vFrame->width;
            height = vFrame->height;

            AVPixelFormat pf = AVPixelFormat(vFrame->format);
            SwsContext* swsContext = sws_getContext(width, height, pf, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

            AVFrame* rgbFrame = av_frame_alloc();
            rgbFrame->format = AV_PIX_FMT_RGB24;
            rgbFrame->width = width;
            rgbFrame->height = height;
            if (av_frame_get_buffer(rgbFrame, 0) < 0) { cout << "  Error in VRVideo, av_frame_get_buffer failed!" << endl; continue; }
            int rgbH = sws_scale(swsContext, vFrame->data, vFrame->linesize, 0, height, rgbFrame->data, rgbFrame->linesize);

            if (rgbH < 0) {
                cout << "  Error in VRVideo, sws_scale failed!" << endl;
                continue;
            }

            auto data = (const uint8_t *)rgbFrame->data[0];

            VRTexturePtr img = VRTexture::create();
            img->getImage()->set(Image::OSG_RGB_PF, rgbFrame->linesize[0]/3, height, 1, 1, 1, 0.0, (const uint8_t *)rgbFrame->data[0], Image::OSG_UINT8_IMAGEDATA, true, 1); // TODO: try to change true to false

            frames[stream][frame] = img;
            frame++;
        }
    }
}

void VRVideo::close() {
    if (vFrame) av_free(vFrame); // Free the YUV frame
    if (vCodec) avcodec_close(vCodec); // Close the codec
    if (vFile) avformat_close_input(&vFile); // Close the video file

    vFrame = 0;
    vCodec = 0;
    vFile = 0;
}

size_t VRVideo::getNFrames(int stream) { return frames[stream].size(); }

void VRVideo::showFrame(int stream, int frame) {
    if (auto m = material.lock()) m->setTexture(getFrame(stream,frame));
}

void VRVideo::play(int stream, float t0, float t1, float v) {
    cout << "\nPLAY\n";
    showFrame(stream, 0);
}

VRTexturePtr VRVideo::getFrame(int stream, int i) { if (frames[stream].count(i) == 0) return 0; return frames[stream][i]; }
VRTexturePtr VRVideo::getFrame(int stream, float t) { return frames[stream][(int)t*getNFrames(stream)]; }
