#include "VRVideo.h"
#include "VRMaterial.h"

#include <OpenSG/OSGImage.h>
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/VRAnimation.h"
#include "core/scene/VRScene.h"
#include "core/scene/sound/VRSound.h"
#include "core/scene/sound/VRSoundManager.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#include <string>
#include <thread>
#include <boost/thread/recursive_mutex.hpp>

typedef boost::recursive_mutex::scoped_lock PLock;

using namespace OSG;

template<> string typeName(const VRVideo& o) { return "Video"; }

VRVideo::VRVideo(VRMaterialPtr mat) {
    //avMutex = new boost::mutex();
    material = mat;
    av_register_all(); // Register all formats && codecs
}

VRVideo::~VRVideo() {
    cout << "VRVideo::~VRVideo " << endl;
    if (anim) anim->stop();
    if (wThreadID >= 0) VRScene::getCurrent()->stopThread(wThreadID, 1000);
    if (vFrame) av_frame_free(&vFrame);
    if (nFrame) av_frame_free(&nFrame);
    if (vFile) avformat_close_input(&vFile); // Close the video file
    //if (avMutex) delete avMutex;
    cout << " VRVideo::~VRVideo done" << endl;

    vFrame = 0;
    vFile = 0;
}

VRVideo::VStream::~VStream() {
    cout << " VRVideo::VStream::~VStream " << endl;
    if (vCodec) avcodec_close(vCodec); // Close the codec
    vCodec = 0;
}

VRVideo::AStream::~AStream() {
    cout << " VRVideo::AStream::~AStream " << endl;
    if (audio) audio->close(); // Close the codec
    audio = 0;
}

VRVideoPtr VRVideo::create(VRMaterialPtr mat) { return VRVideoPtr( new VRVideo(mat) ); }

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

int getNColors(AVPixelFormat pfmt) {
    if (pfmt == AV_PIX_FMT_NONE) return 0;
    if (pfmt == AV_PIX_FMT_YUV420P) return 3;
    if (pfmt == AV_PIX_FMT_YUYV422) return 3;
    if (pfmt == AV_PIX_FMT_RGB24) return 3;
    if (pfmt == AV_PIX_FMT_BGR24) return 3;
    if (pfmt == AV_PIX_FMT_YUV422P) return 3;
    if (pfmt == AV_PIX_FMT_YUV444P) return 3;
    if (pfmt == AV_PIX_FMT_YUV410P) return 1;
    if (pfmt == AV_PIX_FMT_YUV411P) return 1;
    if (pfmt == AV_PIX_FMT_GRAY8) return 1;
    if (pfmt == AV_PIX_FMT_MONOWHITE) return 1;
    if (pfmt == AV_PIX_FMT_MONOBLACK) return 1;
    if (pfmt == AV_PIX_FMT_PAL8) return 3;
    if (pfmt == AV_PIX_FMT_YUVJ420P) return 3;
    if (pfmt == AV_PIX_FMT_YUVJ422P) return 3;
    if (pfmt == AV_PIX_FMT_YUVJ444P) return 3;
    // ...

    return 3;
}

VRTexturePtr VRVideo::convertFrame(int stream, AVPacket* packet) {
    if (!vStreams.count(stream)) { cout << " unknown stream " << stream << endl; return 0; }
    int valid = 0;
    auto vCodec = vStreams[stream].vCodec;
    int r = avcodec_decode_video2(vCodec, vFrame, &valid, packet); // Decode video frame

    if (valid == 0 || r < 0) {
        cout << " avcodec_decode_video2 failed with " << r << endl;
        // TODO: print packet data
        return 0;
    }

    FlipFrame(vFrame);
    int width = vFrame->width;
    int height = vFrame->height;
    AVPixelFormat pf = AVPixelFormat(vFrame->format);

    int Ncols = getNColors(pf);
    if (Ncols == 0) { cout << "ERROR: stream has no colors!" << endl; return 0; }

    if (swsContext == 0) {
        if (Ncols == 1) nFrame->format = AV_PIX_FMT_GRAY8;
        if (Ncols == 3) nFrame->format = AV_PIX_FMT_RGB24;

        swsContext = sws_getContext(width, height, pf, width, height, AVPixelFormat(nFrame->format), SWS_BILINEAR, NULL, NULL, NULL);
        nFrame->width = width;
        nFrame->height = height;
        if (av_frame_get_buffer(nFrame, 0) < 0) { cout << "  Error in VRVideo, av_frame_get_buffer failed!" << endl; return 0; }
    }

    int rgbH = sws_scale(swsContext, vFrame->data, vFrame->linesize, 0, height, nFrame->data, nFrame->linesize);
    if (rgbH < 0) { cout << "  Error in VRVideo, sws_scale failed!" << endl; return 0; }
    int rgbW = nFrame->linesize[0]/Ncols;

    osgFrame.resize(width*height*3, 0);

    auto data1 = (uint8_t*)nFrame->data[0];
    auto data2 = (uint8_t*)&osgFrame[0];
    for (int y = 0; y<height; y++) {
        int k1 = y*width*Ncols;
        int k2 = y*rgbW*Ncols;
        memcpy(&data2[k1], &data1[k2], width*Ncols);
    }

    VRTexturePtr img = VRTexture::create();
    if (Ncols == 1) img->getImage()->set(Image::OSG_L_PF, width, height, 1, 1, 1, 0.0, data2, Image::OSG_UINT8_IMAGEDATA, true, 1);
    if (Ncols == 3) img->getImage()->set(Image::OSG_RGB_PF, width, height, 1, 1, 1, 0.0, data2, Image::OSG_UINT8_IMAGEDATA, true, 1);
    return img;
}

void VRVideo::open(string f) {
    // open file
    if (avformat_open_input(&vFile, f.c_str(), NULL, NULL)!=0) return; // Couldn't open file
    if (avformat_find_stream_info(vFile, NULL)<0) return; // Couldn't find stream information
    av_dump_format(vFile, 0, f.c_str(), 0); // Dump information about file onto standard error

    duration = vFile->duration     * 1e-6;
    start_time = vFile->start_time * 1e-6;

    cout << " VRVideo::open " << f << endl;

    if (!vFrame) vFrame = av_frame_alloc(); // Allocate video frame
    if (!nFrame) nFrame = av_frame_alloc(); // Allocate video frame

    vStreams.clear();
    aStreams.clear();
    for (int i=0; i<(int)vFile->nb_streams; i++) {
        AVStream* avStream = vFile->streams[i];
        AVCodecContext* avCodec = avStream->codec;
        if (avCodec == 0) continue;

        bool isVideo = (avCodec->codec_type == AVMEDIA_TYPE_VIDEO);
        bool isAudio = (avCodec->codec_type == AVMEDIA_TYPE_AUDIO);

        if (isVideo) {
            vStreams[i] = VStream();
            vStreams[i].vCodec = avCodec;
            vStreams[i].fps = av_q2d(avStream->avg_frame_rate);

            // Find the decoder for the video stream
            AVDictionary* optionsDict = 0;
            AVCodec* c = avcodec_find_decoder(avCodec->codec_id);
            if (c == 0) { fprintf(stderr, "Unsupported codec!\n"); return; } // Codec not found
            if (avcodec_open2(avCodec, c, &optionsDict)<0) return; // Could not open codec
        }

        if (isAudio) {
            aStreams[i] = AStream();
            aStreams[i].audio = VRSound::create();
            aStreams[i].audio->setVolume(volume);

            // Find the decoder for the audio stream
            AVDictionary* optionsDict = 0;
            AVCodec* c = avcodec_find_decoder(avCodec->codec_id);
            if (c == 0) { fprintf(stderr, "Unsupported codec!\n"); continue; } // Codec not found
            if (avcodec_open2(avCodec, c, &optionsDict)<0) continue; // Could not open codec
            aStreams[i].audio->initWithCodec(avCodec);
        }
    }

    worker = VRThreadCb::create( "video cache", bind(&VRVideo::cacheFrames, this, placeholders::_1) );
    wThreadID = VRScene::getCurrent()->initThread(worker, "video cache", true, 0);
}

void VRVideo::cacheFrames(VRThreadWeakPtr t) { loadSomeFrames(); }

void VRVideo::loadSomeFrames() {
    PLock lock(avMutex);

    int currentF = currentFrame;

    bool doReturn = true;
    for (auto& s : vStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doReturn = false;
    //for (auto& s : aStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doReturn = false;
    //cout << "LF " << currentF << ", return? " << doReturn << endl;
    if (doReturn) return;

    for (AVPacket packet; av_read_frame(vFile, &packet)>=0; av_packet_unref(&packet)) { // read packets
        if (interruptCaching) break;

        int stream = packet.stream_index;

        if (aStreams.count(stream)) {
            auto a = aStreams[stream].audio;
            auto data = a->extractPacket(&packet);
            PLock lock(osgMutex);
            aStreams[stream].frames[aStreams[stream].cachedFrameMax] = data;
            aStreams[stream].cachedFrameMax++;
        }

        if (vStreams.count(stream)) {
            //cout << " v frame0: " << currentF << " N: " << vStreams[stream].cachedFrameMax << endl;
            auto img = convertFrame(stream, &packet);
            if (!img) continue;
            //cout << "  converted the frame!" << endl;
            PLock lock(osgMutex);
            vStreams[stream].frames[vStreams[stream].cachedFrameMax] = img;
            vStreams[stream].cachedFrameMax++;
        }

        // break if all streams are sufficiently cached
        bool doBreak = true;
        for (auto& s : vStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doBreak = false;
        //for (auto& s : aStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doBreak = false;
        if (doBreak) { av_packet_unref(&packet); break; }
    }

    for (auto& s : vStreams) { // cleanup cache
        if (interruptCaching) break;
        vector<int> toRemove;
        for (auto f : s.second.frames) { // read stream
            if (f.first < currentF) toRemove.push_back(f.first);
        }

        for (auto r : toRemove) s.second.frames.erase(r);
    }

    interruptCaching = false;
}

void VRVideo::setVolume(float v) {
    volume = v;
    for (auto& a : aStreams) a.second.audio->setVolume(v);
}

size_t VRVideo::getNFrames(int stream) {
    auto& s = vStreams[stream];
    return s.fps * duration;
}

float VRVideo::getDuration() { return duration; }

void VRVideo::goTo(float t) { // TODO
    PLock lock(avMutex);

    t = 0;

    if (anim) anim->goTo(t);

    int64_t timestamp = t; // TODO

    cout << " goTo " << t << endl;
    for (int i=0; i<(int)vFile->nb_streams; i++) {
        int r = av_seek_frame(vFile, i, timestamp, AVSEEK_FLAG_BACKWARD);
        if (r < 0) cout << "AAAAAAAA, av_seek_frame failed!!" << endl;
        vStreams[i].frames.clear();
        vStreams[i].cachedFrameMax = 0; // TODO

        interruptCaching = true; // TODO
        currentFrame = 0; // TODO
    }
    cout << "  goTo done" << endl;
}

void VRVideo::pause() {
    if (anim) anim->pause();
    for (auto& a : aStreams)
        if (a.second.audio) a.second.audio->pause();
}

void VRVideo::resume() {
    if (anim) anim->resume();
    for (auto& a : aStreams)
        if (a.second.audio) a.second.audio->resume();
}

bool VRVideo::isPaused() {
    if (anim) return anim->isPaused();
    return false;
}

void VRVideo::showFrame(int stream, int frame) {
    PLock lock(osgMutex);
    currentFrame = frame;

    // video, just jump to frame
    auto f = getFrame(stream, frame);
    if (f) {
        //cout << " showFrame " << frame << " " << f->getSize() << " threadID: " << this_thread::get_id() << endl;
        if (auto m = material.lock()) {
            m->setTexture(f);
            m->setMagMinFilter(GL_LINEAR, GL_LINEAR);
        }
    } //else cout << " showFrame, none found " << frame << endl;

    // audio, queue until current frame
    for (auto& s : aStreams) { // just pick first audio stream if any..
        int I0 = s.second.lastFrameQueued;
        int I1 = s.second.cachedFrameMax; //min(frame+audioQueue, s.second.cachedFrameMax);
        //cout << ", queue audio: " << I0 << " -> " << I1 << ", queued buffers: " << s.second.audio->getQueuedBuffer() << endl;
        for (int i=I0; i<I1; i++) {
            for (auto& d : s.second.frames[i]) {
                s.second.audio->queueFrameData(d.first, d.second);
            }
        }
        s.second.lastFrameQueued = I1;
    }
}

void VRVideo::frameUpdate(float t, int stream) {
    PLock lock(osgMutex);
    int i = vStreams[stream].fps * duration * t;
    showFrame(stream, i);
}

void VRVideo::play(int stream, float t0, float t1, float v) {
    if (!anim) anim = VRAnimation::create();

    animCb = VRAnimCb::create("videoCB", bind(&VRVideo::frameUpdate, this, placeholders::_1, stream));
    anim->setCallback(animCb);
    anim->setDuration(duration);
    anim->start(start_time);
    cout << "video, play stream" << stream << ", start offset: " << start_time << ", duration: " << duration << endl;
}

VRTexturePtr VRVideo::getFrame(int stream, int i) {
    if (vStreams.count(stream) == 0) return 0;
    if (vStreams[stream].frames.count(i) == 0) return 0;
    return vStreams[stream].frames[i];
}

VRTexturePtr VRVideo::getFrame(int stream, float t) {
    int i = vStreams[stream].fps * duration * t;
    return vStreams[stream].frames[i];
}




