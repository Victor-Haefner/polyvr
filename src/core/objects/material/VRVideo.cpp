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

#if LIBAVFORMAT_VERSION_MAJOR >= 58
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

using namespace OSG;

VRVideoFrame::VRVideoFrame() {}
VRVideoFrame::~VRVideoFrame() {}

VRTexturePtr VRVideoFrame::getTexture() { return tex; }
bool VRVideoFrame::isQueuedForRemoval() { return removalQueued; }
void VRVideoFrame::queueRemoval() { removalQueued = true; }

void VRVideoFrame::applyToMaterial(VRMaterialPtr material) {
    if (!tex || !material) return;
    material->setTexture(tex);
    material->setMagMinFilter(GL_LINEAR, GL_LINEAR);
}

void VRVideoFrame::setupTexture(int width, int height, int Ncols, vector<uint8_t>& data) {
    tex = VRTexture::create();
    if (Ncols == 1) tex->getImage()->set(Image::OSG_L_PF, width, height, 1, 1, 1, 0.0, &data[0], Image::OSG_UINT8_IMAGEDATA, true, 1);
    if (Ncols == 3) tex->getImage()->set(Image::OSG_RGB_PF, width, height, 1, 1, 1, 0.0, &data[0], Image::OSG_UINT8_IMAGEDATA, true, 1);
}


VRVideoStream::VRVideoStream() {}

VRVideoStream::VRVideoStream(AVStream* avStream, AVCodecContext* avContext) {
    vFrame = av_frame_alloc();
    nFrame = av_frame_alloc();
    vCodec = avContext;
    fps = av_q2d(avStream->avg_frame_rate);
}

VRVideoStream::~VRVideoStream() {
    cout << " VRVideoStream::~VRVideoStream " << endl;
    if (vCodec) avcodec_close(vCodec); // Close the codec
    if (vFrame) av_frame_free(&vFrame);
    if (nFrame) av_frame_free(&nFrame);
    vCodec = 0;
    vFrame = 0;
    nFrame = 0;
}

int VRVideoStream::getCurrentFrame() {
    VRLock lock(osgMutex);
    return currentFrame;
}

void VRVideoStream::setCurrentFrame(int f) {
    VRLock lock(osgMutex);
    currentFrame = f;
}

void VRVideoStream::queueFrameUpdate(int frame) {
    VRLock lock(osgMutex);
    currentFrame = frame;
    needsFrameUpdate = true;
}

void VRVideoStream::updateFrame(VRMaterialPtr material) {
    if (!needsFrameUpdate) return;
    needsFrameUpdate = false;
    int frame = getCurrentFrame();
    if (!frames.count(frame)) return;
    frames[frame].applyToMaterial(material);
}

void VRVideoStream::doCleanup() {
    if (!needsCleanup) return;
    VRLock lock(osgMutex);
    for (auto r : toRemove) frames.erase(r);
    toRemove.clear();
    needsCleanup = false;
}

void VRVideoStream::processFrames() {
    if (!texDataQueued) return;
    VRLock lock(osgMutex);
    for (auto& tdi : texDataPool) {
        auto& td = tdi.second;
        frames[td.frameI].setupTexture(td.width, td.height, td.Ncols, td.data);
    }
    texDataPool.clear();
    texDataQueued = false;
}

VRTexturePtr VRVideoStream::getTexture(int i) {
    if (frames.count(i) == 0) return 0;
    return frames[i].getTexture();
}

bool VRVideoStream::needsData() { return bool(cachedFrameMax-currentFrame < cacheSize); }
int VRVideoStream::getFPS() { return fps; }

void VRVideoStream::reset() {
    texDataPool.clear();
    frames.clear();
    cachedFrameMax = 0;
}

void VRVideoStream::checkOldFrames() {
    VRLock lock(osgMutex);
    for (auto& f : frames) { // read stream
        if (f.second.isQueuedForRemoval()) continue;
        if (f.first < currentFrame) {
            //cout << " queue removal " << f.first << ", " << currentF << ", " << &f << endl;
            toRemove.push_back( f.first );
            needsCleanup = true;
            f.second.queueRemoval();
        }
    }
    cachedFrameMin = currentFrame;
}

bool VRVideoStream::decode(AVPacket* packet) {
    int valid = 0;
    int r = avcodec_decode_video2(vCodec, vFrame, &valid, packet); // Decode video frame

    if (valid == 0 || r < 0) {
        cout << " avcodec_decode_video2 failed with " << r << endl;
        // TODO: print packet data
        return false;
    }

    auto flipFrame = [](AVFrame* pFrame) {
        for (int i = 0; i < 4; i++) {
            if (i) pFrame->data[i] += pFrame->linesize[i] * ((pFrame->height >> 1)-1);
            else   pFrame->data[i] += pFrame->linesize[i] *  (pFrame->height-1);
            pFrame->linesize[i] = -pFrame->linesize[i];
        }
    };

    flipFrame(vFrame);
    int width = vFrame->width;
    int height = vFrame->height;
    AVPixelFormat pf = AVPixelFormat(vFrame->format);

    int Ncols = getNColors(pf);
    if (Ncols == 0) { cout << "ERROR: stream has no colors!" << endl; return false; }

    if (swsContext == 0) {
        if (Ncols == 1) nFrame->format = AV_PIX_FMT_GRAY8;
        if (Ncols == 3) nFrame->format = AV_PIX_FMT_RGB24;

        swsContext = sws_getContext(width, height, pf, width, height, AVPixelFormat(nFrame->format), SWS_BILINEAR, NULL, NULL, NULL);
        nFrame->width = width;
        nFrame->height = height;
        if (av_frame_get_buffer(nFrame, 0) < 0) { cout << "  Error in VRVideo, av_frame_get_buffer failed!" << endl; return false; }
    }

    int rgbH = sws_scale(swsContext, vFrame->data, vFrame->linesize, 0, height, nFrame->data, nFrame->linesize);
    if (rgbH < 0) { cout << "  Error in VRVideo, sws_scale failed!" << endl; return false; }
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
    int frameI = cachedFrameMax;
    texDataPool[frameI] = { frameI, width, height, Ncols, osgFrame };
    texDataQueued = true;
    cachedFrameMax++;

    return true;
}


VRVideo::VRVideo(VRMaterialPtr mat) {
    //avMutex = new boost::mutex();
    material = mat;
#if LIBAVFORMAT_VERSION_MAJOR < 58
    av_register_all();
#endif

    mainLoopCb = VRUpdateCb::create("Video main update", bind(&VRVideo::mainThreadUpdate, this));
    VRScene::getCurrent()->addUpdateFkt(mainLoopCb);
}

VRVideo::~VRVideo() {
    cout << "VRVideo::~VRVideo " << endl;
    if (anim) anim->stop();
    if (wThreadID >= 0) VRScene::getCurrent()->stopThread(wThreadID, 1000);
    if (vFile) avformat_close_input(&vFile); // Close the video file
    vFile = 0;
    cout << " VRVideo::~VRVideo done" << endl;
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

void VRVideo::mainThreadUpdate() {
    if (!vStreams.count(currentStream)) return;

    auto& stream = vStreams[ currentStream ];
    stream.updateFrame( material.lock() );
    stream.processFrames();
    stream.doCleanup();
}

void VRVideo::showFrame(int stream, int frame) {
    currentStream = stream; // thread safety?
    auto& vStream = vStreams[ stream ];
    vStream.queueFrameUpdate(frame);

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

void VRVideo::open(string f) {
    // open file
    if (avformat_open_input(&vFile, f.c_str(), NULL, NULL)!=0) return; // Couldn't open file
    if (avformat_find_stream_info(vFile, NULL)<0) return; // Couldn't find stream information
    av_dump_format(vFile, 0, f.c_str(), 0); // Dump information about file onto standard error

    duration = vFile->duration     * 1e-6;
    start_time = vFile->start_time * 1e-6;

    cout << " VRVideo::open " << f << endl;

    vStreams.clear();
    aStreams.clear();

    for (int i=0; i<(int)vFile->nb_streams; i++) {
        AVStream* avStream = vFile->streams[i];
        AVCodecParameters* avCodec = avStream->codecpar;
        const AVCodec* codec = avcodec_find_decoder(avCodec->codec_id);
        if (codec == 0) { fprintf(stderr, "Unsupported codec!\n"); continue; } // Codec not found
        AVCodecContext* avContext = avcodec_alloc_context3(codec);
        if (avcodec_parameters_to_context(avContext, avCodec) < 0) continue;
        if (avCodec == 0) continue;
        AVDictionary* optionsDict = 0;
        if (avcodec_open2(avContext, codec, &optionsDict)<0) continue; // Could not open codec

        bool isVideo = (avCodec->codec_type == AVMEDIA_TYPE_VIDEO);
        bool isAudio = (avCodec->codec_type == AVMEDIA_TYPE_AUDIO);

        if (isVideo) vStreams.emplace(piecewise_construct, forward_as_tuple(i), forward_as_tuple(avStream, avContext));

        if (isAudio) {
            aStreams[i] = AStream();
            aStreams[i].audio = VRSound::create();
            aStreams[i].audio->setVolume(volume);
            aStreams[i].audio->initWithCodec(avContext);
        }
    }

    worker = VRThreadCb::create( "video cache", bind(&VRVideo::cacheFrames, this, placeholders::_1) );
    wThreadID = VRScene::getCurrent()->initThread(worker, "video cache", true, 0);
}

void VRVideo::cacheFrames(VRThreadWeakPtr t) { loadSomeFrames(); }

void VRVideo::loadSomeFrames() {
    VRLock lock(avMutex);

    bool doReturn = true;
    for (auto& s : vStreams) if ( s.second.needsData() ) doReturn = false;
    if (doReturn) return;

    for (AVPacket packet; av_read_frame(vFile, &packet)>=0; av_packet_unref(&packet)) { // read packets
        if (interruptCaching) break;

        int stream = packet.stream_index;

        if (aStreams.count(stream)) {
            auto a = aStreams[stream].audio;
            auto data = a->extractPacket(&packet);
            aStreams[stream].frames[aStreams[stream].cachedFrameMax] = data;
            aStreams[stream].cachedFrameMax++;
        }

        if (vStreams.count(stream))
            if (!vStreams[stream].decode(&packet)) return;

        // break if all streams are sufficiently cached
        bool doBreak = true;
        for (auto& s : vStreams) if ( s.second.needsData() ) doBreak = false;
        //for (auto& s : aStreams) if (s.second.cachedFrameMax-currentF < cacheSize) doBreak = false;
        if (doBreak) { av_packet_unref(&packet); break; }
    }

    for (auto& s : vStreams) { // cleanup cache
        if (interruptCaching) break;
        s.second.checkOldFrames();
    }

    interruptCaching = false;
}

void VRVideo::setVolume(float v) {
    volume = v;
    for (auto& a : aStreams) a.second.audio->setVolume(v);
}

size_t VRVideo::getNFrames(int stream) {
    auto& s = vStreams[stream];
    return s.getFPS() * duration;
}

float VRVideo::getDuration() { return duration; }

void VRVideo::prepareJump() {
    ;
}

void VRVideo::goTo(float t) { // TODO, handle t and audio
    VRLock lock(avMutex);

    for (auto& s : vStreams) s.second.reset();
    /*for (auto& s : vStreams) {
        int frame = s.second.fps * duration * t;
    }*/

    //if (anim) anim->goTo(t);
    if (anim) anim->start();

    int64_t timestamp = t; // TODO
    //interruptCaching = true; // TODO

    cout << " goTo " << t << endl;
    for (int i=0; i<(int)vFile->nb_streams; i++) {
        AVStream* avStream = vFile->streams[i];
        AVCodecParameters* avCodec = avStream->codecpar;
        const AVCodec* c = avcodec_find_decoder(avCodec->codec_id);
        AVCodecContext* avContext = avcodec_alloc_context3(c);
        if (avcodec_parameters_to_context(avContext, avCodec) < 0) continue;
        if (avCodec == 0) continue;

        bool isVideo = (avCodec->codec_type == AVMEDIA_TYPE_VIDEO);
        bool isAudio = (avCodec->codec_type == AVMEDIA_TYPE_AUDIO);

        int r = av_seek_frame(vFile, i, timestamp, AVSEEK_FLAG_BACKWARD);
        if (r < 0) cout << "AAAAAAAA, av_seek_frame failed!!" << endl;

        if (isAudio) {
            /*aStreams[i].frames.clear();
            aStreams[i].cachedFrameMax = 0;
            aStreams[i].lastFrameQueued = 0;
            aStreams[i].audio->stop();
            aStreams[i].audio->play();*/

            aStreams[i].audio->stop();

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
    auto& vStream = vStreams[stream];
    int i = vStream.getFPS() * duration * t;
    if (vStream.currentFrame == i) return;
    //cout << "frameUpdate " << t << ", " << i << ", " << vStream.frames.size() << endl;
    showFrame(stream, i);
}

void VRVideo::play(int stream, float t0, float t1, float v) {
    if (anim) {
        goTo(t0);
        return;
    }

    animCb = VRAnimCb::create("videoCB", bind(&VRVideo::frameUpdate, this, placeholders::_1, stream));
    anim = VRAnimation::create();
    anim->addCallback(animCb);
    anim->setDuration(duration);
    anim->start(start_time);
    cout << "video, play stream" << stream << ", start offset: " << start_time << ", duration: " << duration << endl;
}

VRTexturePtr VRVideo::getFrame(int stream, int i) {
    if (vStreams.count(stream) == 0) return 0;
    return vStreams[stream].getTexture(i);
}

VRTexturePtr VRVideo::getFrame(int stream, float t) {
    if (vStreams.count(stream) == 0) return 0;
    int i = vStreams[stream].getFPS() * duration * t;
    return getFrame(stream, i);
}

