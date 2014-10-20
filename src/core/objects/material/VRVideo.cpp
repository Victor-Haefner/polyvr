#include "VRVideo.h"
#include "VRMaterial.h"

#include <OpenSG/OSGImage.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#include <string>

VRVideo::VRVideo(OSG::VRMaterial* mat) {
    material = mat;

    vFile = 0;
    vCodec = 0;
    vFrame = 0;
    NStreams = 0;

    av_register_all(); // Register all formats and codecs

    // test
    const char* file = "~/Videos/drop.avi";
    open(file);
    close();
}

VRVideo::~VRVideo() {;}

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

void VRVideo::open(string f) {
    // open file
    if(avformat_open_input(&vFile, f.c_str(), NULL, NULL)!=0) return; // Couldn't open file
    if(avformat_find_stream_info(vFile, NULL)<0) return; // Couldn't find stream information
    av_dump_format(vFile, 0, f.c_str(), 0); // Dump information about file onto standard error

    NStreams = getNStreams();

    vFrame = avcodec_alloc_frame(); // Allocate video frame

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

        width = vCodec->width;
        height = vCodec->height;

        int valid=0;
        int frame=0;
        for(AVPacket packet; av_read_frame(vFile, &packet)>=0; av_free_packet(&packet) ) { // read stream
            if(packet.stream_index != stream) continue;

            avcodec_decode_video2(vCodec, vFrame, &valid, &packet); // Decode video frame
            if(valid == 0) continue;

            OSG::ImageRecPtr img = OSG::Image::create();
            //img->setData(vFrame->data);

            img->set(OSG::Image::OSG_RGB_PF, width, height, 1, 1, 1, 0.0, (const uint8_t *)vFrame->data, OSG::Image::OSG_UINT8_IMAGEDATA, true, 1); // TODO: try to change true to false

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

void VRVideo::play(int stream, float t0, float t1, float v) {
    cout << "\nPLAY\n";
    material->setTexture(getFrame(stream,0));
}

OSG::ImageRecPtr VRVideo::getFrame(int stream, int i) { if (frames[stream].count(i) == 0) return 0; return frames[stream][i]; }
OSG::ImageRecPtr VRVideo::getFrame(int stream, float t) { return frames[stream][(int)t*getNFrames()]; }
int VRVideo::getNFrames() { return frames.size(); }
