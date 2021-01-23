#include "VRVideo.h"
#include "VRMaterial.h"

#include <OpenSG/OSGImage.h>
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/VRAnimation.h"

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
    for (int i = 0; i < 4; i++) {
        if (i) {
            pFrame->data[i] += pFrame->linesize[i] * ((pFrame->height >> 1)-1);
        } else {
            pFrame->data[i] += pFrame->linesize[i] * (pFrame->height-1);
        }
        pFrame->linesize[i] = -pFrame->linesize[i];
    }
}

void VRVideo::open(string f) {
    // open file
    if(avformat_open_input(&vFile, f.c_str(), NULL, NULL)!=0) return; // Couldn't open file
    if(avformat_find_stream_info(vFile, NULL)<0) return; // Couldn't find stream information
    av_dump_format(vFile, 0, f.c_str(), 0); // Dump information about file onto standard error

    NStreams = getNStreams();

    cout << " VRVideo::open " << f << ", "<< NStreams << " streams" << endl;

    AVFrame* vFrame = av_frame_alloc(); // Allocate video frame
    AVFrame* rgbFrame = av_frame_alloc();
    vector<UInt8> osgFrame;

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

        SwsContext* swsContext = 0;

        cout << "  VRVideo::open stream " << i << endl;
        int valid=0;
        int frame=0;
        for(AVPacket packet; av_read_frame(vFile, &packet)>=0; av_packet_unref(&packet) ) { // read stream
            if(packet.stream_index != stream) continue;

            avcodec_decode_video2(vCodec, vFrame, &valid, &packet); // Decode video frame
            if(valid == 0) continue;

            FlipFrame(vFrame);
            width = vFrame->width;
            height = vFrame->height;

            if (!swsContext) {
                AVPixelFormat pf = AVPixelFormat(vFrame->format);
                swsContext = sws_getContext(width, height, pf, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

                rgbFrame->format = AV_PIX_FMT_RGB24;
                rgbFrame->width = width;
                rgbFrame->height = height;
                if (av_frame_get_buffer(rgbFrame, 0) < 0) { cout << "  Error in VRVideo, av_frame_get_buffer failed!" << endl; continue; }

                osgFrame.resize(width*height*3, 0);
            }

            int rgbH = sws_scale(swsContext, vFrame->data, vFrame->linesize, 0, height, rgbFrame->data, rgbFrame->linesize);
            if (rgbH < 0) { cout << "  Error in VRVideo, sws_scale failed!" << endl; continue; }
            int rgbW = rgbFrame->linesize[0]/3;

            osgFrame.resize(width*height*3, 0);

            auto data1 = (uint8_t*)rgbFrame->data[0];
            auto data2 = (uint8_t*)&osgFrame[0];
            for (int y = 0; y<height; y++) {
                int k1 = y*width*3;
                int k2 = y*rgbW*3;
                memcpy(&data2[k1], &data1[k2], width*3);
            }

            VRTexturePtr img = VRTexture::create();
            img->getImage()->set(Image::OSG_RGB_PF, width, height, 1, 1, 1, 0.0, data2, Image::OSG_UINT8_IMAGEDATA, true, 1);
            frames[stream][frame] = img;
            frame++;
        }
    }

    av_frame_free(&vFrame);
    av_frame_free(&rgbFrame);
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

void VRVideo::frameUpdate(float t, int stream, int N) {
    showFrame(stream, t*N);
}

void VRVideo::play(int stream, float t0, float t1, float v) {
    if (!anim) anim = VRAnimation::create();
    int N = getNFrames(stream);
    float T = N/24.0;

    animCb = VRAnimCb::create("videoCB", bind(&VRVideo::frameUpdate, this, placeholders::_1, stream, N));
    anim->setCallback(animCb);
    anim->setDuration(T); // 24 fps
    anim->start();
    cout << "video, play stream" << stream << ", duration: " << T << endl;
}

VRTexturePtr VRVideo::getFrame(int stream, int i) { if (frames[stream].count(i) == 0) return 0; return frames[stream][i]; }
VRTexturePtr VRVideo::getFrame(int stream, float t) { return frames[stream][(int)t*getNFrames(stream)]; }
