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
#include "core/scene/sound/VRSoundUtils.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#include <string>
#include "core/utils/Thread.h"
#include "core/utils/VRMutex.h"



using namespace OSG;

VRVideo::VRVideo(VRMaterialPtr mat) {
    //avMutex = new boost::mutex();
    material = mat;
#ifndef _WIN32
#ifndef __APPLE__
    av_register_all(); // Register all formats && codecs
#endif
#endif

    mainLoopCb = VRUpdateCb::create("Video main update", bind(&VRVideo::mainThreadUpdate, this));
    VRScene::getCurrent()->addUpdateFkt(mainLoopCb);
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
    for(int i=0; i<(int)vFile->nb_streams; i++) if(vFile->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
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

#if defined(_WIN32) || defined(__APPLE__)
int avcodec_decode_video2(AVCodecContext* video_ctx, AVFrame* frame, int* got_frame, AVPacket* pkt) {
    int used = 0;
    if (video_ctx->codec_type == AVMEDIA_TYPE_VIDEO || video_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        used = avcodec_send_packet(video_ctx, pkt);
        if (used < 0 && used != AVERROR(EAGAIN) && used != AVERROR_EOF) {

        } else {
            if (used >= 0) pkt->size = 0;
            used = avcodec_receive_frame(video_ctx, frame);
            if (used >= 0) *got_frame = 1;
        }
    }
    return used;
}
#endif

void VRVideo::mainThreadUpdate() {
    if (needsMainUpdate) {
        if (auto m = material.lock()) {
            VRLock lock(osgMutex);

            auto tex = getFrame(currentStream, currentFrame);
            if (tex) {
                //cout << " set frame " << currentFrame << endl;
                m->setTexture(tex);
                m->setMagMinFilter(GL_LINEAR, GL_LINEAR);
            } else {
                //cout << " no frame " << currentFrame << endl;
            }

            needsMainUpdate = false;
        }
    }

    if (texDataQueued) {
        VRLock lock(osgMutex);
        auto& td = texDataPool[texPoolPointer];
        //cout << " setup frame " << td.frameI << endl;
        setupTexture(td.stream, td.frameI, td.width, td.height, td.Ncols, td.data);
        texPoolPointer++;
        if (texPoolPointer == texDataPool.size()) {
            texPoolPointer = 0;
            texDataPool.clear();
            texDataQueued = false;
        }
    }

    if (needsCleanup) {
        VRLock lock(osgMutex);
        for (auto r : toRemove) {
            //cout << " erase frame " << r.second << endl;
            vStreams[r.first].frames.erase(r.second);
        }
        toRemove.clear();
    }
}

void VRVideo::setupTexture(int stream, int frameI, int width, int height, int Ncols, uint8_t* data) {
    VRTexturePtr img = VRTexture::create();
    if (Ncols == 1) img->getImage()->set(Image::OSG_L_PF, width, height, 1, 1, 1, 0.0, data, Image::OSG_UINT8_IMAGEDATA, true, 1);
    if (Ncols == 3) img->getImage()->set(Image::OSG_RGB_PF, width, height, 1, 1, 1, 0.0, data, Image::OSG_UINT8_IMAGEDATA, true, 1);

    if (!img) return;
    VRLock lock(osgMutex);
    vStreams[stream].frames[frameI].tex = img;
}

void VRVideo::showFrame(int stream, int frame) {
    {
        VRLock lock(osgMutex);
        //cout << " set current frame to " << frame << endl;
        currentFrame = frame;
        currentStream = stream;
        needsMainUpdate = true;
    }

    // audio, queue until current frame
    for (auto& s : aStreams) { // just pick first audio stream if any..
        AStream& aStream = s.second;
        int I0 = aStream.lastFrameQueued;
        int I1 = aStream.cachedFrameMax; //min(frame+audioQueue, aStream.cachedFrameMax);
        //cout << ", queue audio: " << I0 << " -> " << I1 << ", queued buffers: " << aStream.audio->getInterface()->getQueuedBuffer() << endl;
        for (int i=I0; i<I1; i++) {
            for (auto aframe : aStream.frames[i]) {
                aStream.audio->playBuffer(aframe);
            }
        }
        aStream.lastFrameQueued = I1;
    }
}

void VRVideo::convertFrame(int stream, AVPacket* packet) {
    if (!vStreams.count(stream)) { cout << " unknown stream " << stream << endl; return; }
    int valid = 0;
    auto vCodec = vStreams[stream].vCodec;
    int r = avcodec_decode_video2(vCodec, vFrame, &valid, packet); // Decode video frame

    if (valid == 0 || r < 0) {
        cout << " avcodec_decode_video2 failed with " << r << endl;
        // TODO: print packet data
        return;
    }

    FlipFrame(vFrame);
    int width = vFrame->width;
    int height = vFrame->height;
    AVPixelFormat pf = AVPixelFormat(vFrame->format);

    int Ncols = getNColors(pf);
    if (Ncols == 0) { cout << "ERROR: stream has no colors!" << endl; return; }

    if (swsContext == 0) {
        if (Ncols == 1) nFrame->format = AV_PIX_FMT_GRAY8;
        if (Ncols == 3) nFrame->format = AV_PIX_FMT_RGB24;

        swsContext = sws_getContext(width, height, pf, width, height, AVPixelFormat(nFrame->format), SWS_BILINEAR, NULL, NULL, NULL);
        nFrame->width = width;
        nFrame->height = height;
        if (av_frame_get_buffer(nFrame, 0) < 0) { cout << "  Error in VRVideo, av_frame_get_buffer failed!" << endl; return; }
    }

    int rgbH = sws_scale(swsContext, vFrame->data, vFrame->linesize, 0, height, nFrame->data, nFrame->linesize);
    if (rgbH < 0) { cout << "  Error in VRVideo, sws_scale failed!" << endl; return; }
    int rgbW = nFrame->linesize[0]/Ncols;

    osgFrame.resize(width*height*3, 0);

    auto data1 = (uint8_t*)nFrame->data[0];
    auto data2 = (uint8_t*)&osgFrame[0];
    for (int y = 0; y<height; y++) {
        int k1 = y*width*Ncols;
        int k2 = y*rgbW*Ncols;
        memcpy(&data2[k1], &data1[k2], width*Ncols);
    }

    VRLock lock(osgMutex);
    texDataPool.push_back( {stream, vStreams[stream].cachedFrameMax, width, height, Ncols, data2} );
    texDataQueued = true;
    //setupTexture(stream, vStreams[stream].cachedFrameMax, width, height, Ncols, data2);
    vStreams[stream].cachedFrameMax++;
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
        AVCodecParameters* avCodec = avStream->codecpar;
        const AVCodec* c = avcodec_find_decoder(avCodec->codec_id);
        AVCodecContext* avContext = avcodec_alloc_context3(c);
        if (avcodec_parameters_to_context(avContext, avCodec) < 0) continue;
        if (avCodec == 0) continue;

        bool isVideo = (avCodec->codec_type == AVMEDIA_TYPE_VIDEO);
        bool isAudio = (avCodec->codec_type == AVMEDIA_TYPE_AUDIO);

        if (isVideo) {
            vStreams[i] = VStream();
            vStreams[i].vCodec = avContext;
            vStreams[i].fps = av_q2d(avStream->avg_frame_rate);

            // Find the decoder for the video stream
            AVDictionary* optionsDict = 0;

            if (c == 0) { fprintf(stderr, "Unsupported codec!\n"); return; } // Codec not found
            if (avcodec_open2(avContext, c, &optionsDict)<0) return; // Could not open codec
        }

        if (isAudio) {
            aStreams[i] = AStream();
            aStreams[i].audio = VRSound::create();
            aStreams[i].audio->setVolume(volume);

            // Find the decoder for the audio stream
            AVDictionary* optionsDict = 0;
            if (c == 0) { fprintf(stderr, "Unsupported codec!\n"); continue; } // Codec not found
            if (avcodec_open2(avContext, c, &optionsDict)<0) continue; // Could not open codec
            aStreams[i].audio->initWithCodec(avContext);
        }
    }

    worker = VRThreadCb::create( "video cache", bind(&VRVideo::cacheFrames, this, placeholders::_1) );
    wThreadID = VRScene::getCurrent()->initThread(worker, "video cache", true, 0);
}

void VRVideo::cacheFrames(VRThreadWeakPtr t) { loadSomeFrames(); }

void VRVideo::loadSomeFrames() {
    VRLock lock(avMutex);

    int currentF = currentFrame;

    bool doReturn = true;
    for (auto& s : vStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doReturn = false;
    //for (auto& s : aStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doReturn = false;
    //for (auto& s : vStreams) cout << " cachedFrameMax " << s.second.cachedFrameMax << " cacheSize " << cacheSize << " currentF " << currentF << ", return? " << doReturn << endl;
    if (doReturn) return;

    for (AVPacket packet; av_read_frame(vFile, &packet)>=0; av_packet_unref(&packet)) { // read packets
        if (interruptCaching) break;

        int stream = packet.stream_index;

        if (aStreams.count(stream)) {
            auto a = aStreams[stream].audio;
            auto data = a->extractPacket(&packet);
            VRLock lock(osgMutex);
            aStreams[stream].frames[aStreams[stream].cachedFrameMax] = data;
            aStreams[stream].cachedFrameMax++;
        }

        if (vStreams.count(stream)) convertFrame(stream, &packet);

        // break if all streams are sufficiently cached
        bool doBreak = true;
        for (auto& s : vStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doBreak = false;
        //for (auto& s : aStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doBreak = false;
        if (doBreak) { av_packet_unref(&packet); break; }
    }

    for (auto& s : vStreams) { // cleanup cache
        if (interruptCaching) break;
        VRLock lock(osgMutex);
        for (auto& f : s.second.frames) { // read stream
            if (f.second.removalQueued) continue;
            if (f.first < currentF) {
                //cout << " queue removal " << f.first << ", " << currentF << ", " << &f << endl;
                toRemove.push_back( make_pair(s.first,f.first) );
                needsCleanup = true;
                f.second.removalQueued = true;
            }
        }
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
    VRLock lock(avMutex);

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

bool VRVideo::isRunning() {
    if (!anim) return false;
    if (anim->isActive()) return true;
    return false;
}

void VRVideo::frameUpdate(float t, int stream) {
    VRLock lock(osgMutex);
    int i = vStreams[stream].fps * duration * t;
    //cout << "frameUpdate " << t << ", " << i << ", " << vStreams[stream].frames.size() << endl;
    showFrame(stream, i);
}

void VRVideo::play(int stream, float t0, float t1, float v) {
    if (!anim) anim = VRAnimation::create();

    animCb = VRAnimCb::create("videoCB", bind(&VRVideo::frameUpdate, this, placeholders::_1, stream));
    anim->addCallback(animCb);
    anim->setDuration(duration);
    anim->start(start_time);
    cout << "video, play stream" << stream << ", start offset: " << start_time << ", duration: " << duration << endl;
}

VRTexturePtr VRVideo::getFrame(int stream, int i) {
    if (vStreams.count(stream) == 0) return 0;
    if (vStreams[stream].frames.count(i) == 0) return 0;
    return vStreams[stream].frames[i].tex;
}

VRTexturePtr VRVideo::getFrame(int stream, float t) {
    if (vStreams.count(stream) == 0) return 0;
    int i = vStreams[stream].fps * duration * t;
    return getFrame(stream, i);
}


