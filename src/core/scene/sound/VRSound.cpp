#include "VRSound.h"
#include "VRSoundUtils.h"
#include "core/scene/VRScene.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/math/path.h"
#include "core/math/fft.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/networking/udp/VRUDPClient.h"
#include "core/networking/udp/VRUDPServer.h"
#include "core/networking/rest/VRRestClient.h"
#include "VRSoundManager.h"

#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#endif

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavutil/mathematics.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
    #include <libavcodec/avcodec.h>
}

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <fstream>
#include <map>
#include <climits>
#include <complex>

#ifndef WITHOUT_GTK
#define WARN(x) \
VRConsoleWidget::get( "Errors" )->write( x+"\n" );
#else
#define WARN(x) {}
#endif

using namespace OSG;

string avErrToStr(const int& e) {
    char buf[100];
    int N = 100;
    av_strerror(e, buf, N);
    return string(buf);
}

void printFrame(AVFrame& frame, string tag) {
    ;
}

void printPacket(AVPacket& pkt, string tag) {
    cout << " -- print packet info " << tag << endl;
    cout << "  size: " << pkt.size << endl;
    cout << "  duration: " << pkt.duration << endl;
    cout << "  flags: " << pkt.flags << endl;
    cout << "  stream_index: " << pkt.stream_index << endl;
    //cout << "  convergence_duration: " << pkt.convergence_duration << endl;
    cout << "  dts: " << pkt.dts << endl;
    cout << "  pos: " << pkt.pos << endl;
    cout << "  pts: " << pkt.pts << endl;
    cout << "  side_data_elems: " << pkt.side_data_elems << endl;
    cout << "  buf: " << pkt.buf << endl;
    cout << "  data: " << (void*)pkt.data << endl;
    cout << "  side_data: " << pkt.side_data << endl;
    cout << " -- done -- " << endl;
}

struct VRSound::ALData {
    ALenum sample = 0;
    ALenum format = 0;
    ALenum layout = 0;
    ALenum state = AL_INITIAL;
    AVFormatContext* context = 0;
    SwrContext* resampler = 0;
    AVCodecContext* codec = NULL;
    AVPacket packet;
    AVFrame* frame;
};

VRSound::VRSound() {
    VRSoundManager::get(); // this may init channel
    al = shared_ptr<ALData>( new ALData() );
    reset();
    poseUpdateCb = VRUpdateCb::create( "poseUpdateCb", bind(&VRSound::update3DSound, this) );
}

VRSound::~VRSound() {
    close();
}

VRSoundPtr VRSound::create() { return VRSoundPtr( new VRSound() ); }

VRSoundInterfacePtr VRSound::getInterface() { return interface; }

int VRSound::getState() { return al->state; }
string VRSound::getPath() { return path; }
void VRSound::setPath( string p ) { path = p; }

void VRSound::setLoop(bool loop) { this->loop = loop; doUpdate = true; }
void VRSound::setPitch(float pitch) { this->pitch = pitch; doUpdate = true; }
void VRSound::setVolume(float gain) { this->gain = gain; doUpdate = true; }
void VRSound::setCallback(VRUpdateCbPtr cb) { callback = cb; }

void VRSound::setBeacon(VRTransformPtr t, VRTransformPtr head) {
    poseBeacon = t;
    headBeacon = head;
    if (t) VRScene::getCurrent()->addUpdateFkt(poseUpdateCb);
    else   VRScene::getCurrent()->dropUpdateFkt(poseUpdateCb);
}

void VRSound::setBandpass(float lpass, float hpass) {
    this->lpass = lpass;
    this->hpass = hpass;
    doUpdate = true;
}

bool VRSound::isRunning() {
    if (interface) interface->recycleBuffer();
    //cout << "isRunning " << bool(al->state == AL_PLAYING) << " " << bool(al->state == AL_INITIAL) << " " << getQueuedBuffer()<< endl;
    return (al->state == AL_PLAYING) || (al->state == AL_INITIAL) || (interface && interface->getQueuedBuffer() != 0);
}
void VRSound::stop() { interrupt = true; loop = false; }

void VRSound::pause() {
    if (interface) interface->pause();
}

void VRSound::resume() {
    if (interface) interface->play();
}

void VRSound::close() {
    cout << " !!! VRSound::close !!!" << endl;
    stop();
    interface = 0;
    if (al->context) avformat_close_input(&al->context);
    if (al->resampler) swr_free(&al->resampler);
    al->context = 0;
    al->resampler = 0;
    init = 0;
    cout << "  VRSound::close done" << endl;
}

void VRSound::reset() { al->state = AL_STOPPED; }
void VRSound::play() { al->state = AL_INITIAL; }


void VRSound::updateSampleAndFormat() {
#ifdef __APPLE__
    if (al->codec->ch_layout.u.mask == 0) {
        if (al->codec->ch_layout.nb_channels == 1) al->codec->ch_layout.u.mask = AV_CH_LAYOUT_MONO;
        if (al->codec->ch_layout.nb_channels == 2) al->codec->ch_layout.u.mask = AV_CH_LAYOUT_STEREO;
        if (al->codec->ch_layout.u.mask == 0) cout << "WARNING! channel_layout is 0.\n";
    }
#else
    if (al->codec->channel_layout == 0) {
        if (al->codec->channels == 1) al->codec->channel_layout = AV_CH_LAYOUT_MONO;
        if (al->codec->channels == 2) al->codec->channel_layout = AV_CH_LAYOUT_STEREO;
        if (al->codec->channel_layout == 0) cout << "WARNING! channel_layout is 0.\n";
    }
#endif

    frequency = al->codec->sample_rate;
    al->format = AL_FORMAT_MONO16;
    AVSampleFormat sfmt = al->codec->sample_fmt;

    if (sfmt == AV_SAMPLE_FMT_NONE) cout << "ERROR: unsupported format: none\n";

    if (sfmt == AV_SAMPLE_FMT_U8) al->sample = AL_UNSIGNED_BYTE_SOFT;
    if (sfmt == AV_SAMPLE_FMT_S16) al->sample = AL_SHORT_SOFT;
    if (sfmt == AV_SAMPLE_FMT_S32) al->sample = AL_INT_SOFT;
    if (sfmt == AV_SAMPLE_FMT_FLT) al->sample = AL_FLOAT_SOFT;
    if (sfmt == AV_SAMPLE_FMT_DBL) al->sample = AL_DOUBLE_SOFT;

    if (sfmt == AV_SAMPLE_FMT_U8P) al->sample = AL_UNSIGNED_BYTE_SOFT;
    if (sfmt == AV_SAMPLE_FMT_S16P) al->sample = AL_SHORT_SOFT;
    if (sfmt == AV_SAMPLE_FMT_S32P) al->sample = AL_INT_SOFT;
    if (sfmt == AV_SAMPLE_FMT_FLTP) al->sample = AL_FLOAT_SOFT;
    if (sfmt == AV_SAMPLE_FMT_DBLP) al->sample = AL_DOUBLE_SOFT;

#ifdef __APPLE__
    auto layout = al->codec->ch_layout.u.mask;
#else
    auto layout = al->codec->channel_layout;
#endif

    if (layout == AV_CH_LAYOUT_MONO   ) al->layout = AL_MONO_SOFT;
    if (layout == AV_CH_LAYOUT_STEREO ) al->layout = AL_STEREO_SOFT;
    if (layout == AV_CH_LAYOUT_QUAD   ) al->layout = AL_QUAD_SOFT;
    if (layout == AV_CH_LAYOUT_5POINT1) al->layout = AL_5POINT1_SOFT;
    if (layout == AV_CH_LAYOUT_7POINT1) al->layout = AL_7POINT1_SOFT;

    switch(al->sample) {
        case AL_UNSIGNED_BYTE_SOFT:
            switch(al->layout) {
                case AL_MONO_SOFT:    al->format = AL_FORMAT_MONO8; break;
                case AL_STEREO_SOFT:  al->format = AL_FORMAT_STEREO8; break;
                case AL_QUAD_SOFT:    al->format = alGetEnumValue("AL_FORMAT_QUAD8"); break;
                case AL_5POINT1_SOFT: al->format = alGetEnumValue("AL_FORMAT_51CHN8"); break;
                case AL_7POINT1_SOFT: al->format = alGetEnumValue("AL_FORMAT_71CHN8"); break;
                default: cout << "OpenAL unsupported format 8\n"; break;
            } break;
        case AL_SHORT_SOFT:
            switch(al->layout) {
                case AL_MONO_SOFT:    al->format = AL_FORMAT_MONO16; break;
                case AL_STEREO_SOFT:  al->format = AL_FORMAT_STEREO16; break;
                case AL_QUAD_SOFT:    al->format = alGetEnumValue("AL_FORMAT_QUAD16"); break;
                case AL_5POINT1_SOFT: al->format = alGetEnumValue("AL_FORMAT_51CHN16"); break;
                case AL_7POINT1_SOFT: al->format = alGetEnumValue("AL_FORMAT_71CHN16"); break;
                default: cout << "OpenAL unsupported format 16\n"; break;
            } break;
        case AL_FLOAT_SOFT:
            switch(al->layout) {
                case AL_MONO_SOFT:    al->format = alGetEnumValue("AL_FORMAT_MONO_FLOAT32"); break;
                case AL_STEREO_SOFT:  al->format = alGetEnumValue("AL_FORMAT_STEREO_FLOAT32"); break;
                case AL_QUAD_SOFT:    al->format = alGetEnumValue("AL_FORMAT_QUAD32"); break;
                case AL_5POINT1_SOFT: al->format = alGetEnumValue("AL_FORMAT_51CHN32"); break;
                case AL_7POINT1_SOFT: al->format = alGetEnumValue("AL_FORMAT_71CHN32"); break;
                default: cout << "OpenAL unsupported format 32\n"; break;
            } break;
        case AL_DOUBLE_SOFT:
            switch(al->layout) {
                case AL_MONO_SOFT:    al->format = alGetEnumValue("AL_FORMAT_MONO_DOUBLE"); break;
                case AL_STEREO_SOFT:  al->format = alGetEnumValue("AL_FORMAT_STEREO_DOUBLE"); break;
                default: cout << "OpenAL unsupported format 64\n"; break;
            } break;
        default: cout << "OpenAL unsupported format";
    }

    if (av_sample_fmt_is_planar(al->codec->sample_fmt)) {
        AVSampleFormat out_sample_fmt;
        switch(al->codec->sample_fmt) {
            case AV_SAMPLE_FMT_U8P:  out_sample_fmt = AV_SAMPLE_FMT_U8; break;
            case AV_SAMPLE_FMT_S16P: out_sample_fmt = AV_SAMPLE_FMT_S16; break;
            case AV_SAMPLE_FMT_S32P: out_sample_fmt = AV_SAMPLE_FMT_S32; break;
            case AV_SAMPLE_FMT_DBLP: out_sample_fmt = AV_SAMPLE_FMT_DBL; break;
            case AV_SAMPLE_FMT_FLTP:
            default: out_sample_fmt = AV_SAMPLE_FMT_FLT;
        }

        al->resampler = swr_alloc();
#ifdef __APPLE__
        av_opt_set_chlayout(al->resampler, "in_channel_layout",  &al->codec->ch_layout, 0);
        av_opt_set_sample_fmt    (al->resampler, "in_sample_fmt",      al->codec->sample_fmt,     0);
        av_opt_set_int           (al->resampler, "in_sample_rate",     al->codec->sample_rate,    0);
        av_opt_set_chlayout(al->resampler, "out_channel_layout", &al->codec->ch_layout, 0);
        av_opt_set_sample_fmt    (al->resampler, "out_sample_fmt",     out_sample_fmt,            0);
        av_opt_set_int           (al->resampler, "out_sample_rate",    al->codec->sample_rate,    0);
#else
        av_opt_set_channel_layout(al->resampler, "in_channel_layout",  al->codec->channel_layout, 0);
        av_opt_set_sample_fmt    (al->resampler, "in_sample_fmt",      al->codec->sample_fmt,     0);
        av_opt_set_int           (al->resampler, "in_sample_rate",     al->codec->sample_rate,    0);
        av_opt_set_channel_layout(al->resampler, "out_channel_layout", al->codec->channel_layout, 0);
        av_opt_set_sample_fmt    (al->resampler, "out_sample_fmt",     out_sample_fmt,            0);
        av_opt_set_int           (al->resampler, "out_sample_rate",    al->codec->sample_rate,    0);
#endif
        swr_init(al->resampler);
    }
}

bool VRSound::initiate() {
    cout << "init sound " << path << endl;
    interface = VRSoundInterface::create();
    initiated = true;

    if (path == "") return 1;

    auto e = avformat_open_input(&al->context, path.c_str(), NULL, NULL);
    if (e < 0) {
        string warning = "ERROR! avformat_open_input of path '"+path+"' failed: " + avErrToStr(e);
        cout << warning << endl;
        WARN(warning);
        return 0;
    }

    if (auto e = avformat_find_stream_info(al->context, NULL)) if (e < 0) { cout << "ERROR! avformat_find_stream_info failed: " << avErrToStr(e) << endl; return 0; }
    av_dump_format(al->context, 0, path.c_str(), 0);

    stream_id = av_find_best_stream(al->context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (stream_id == -1) return 0;

    AVCodecParameters* avCodec = al->context->streams[stream_id]->codecpar;
    const AVCodec* avcodec = avcodec_find_decoder(avCodec->codec_id);
    if (avcodec == 0) return 0;
    AVCodecContext* avContext = avcodec_alloc_context3(avcodec);
    avcodec_parameters_to_context(avContext, avCodec);

    al->codec = avContext;
    if (avcodec_open2(al->codec, avcodec, NULL) < 0) return 0;

    nextBuffer = 0;

    updateSampleAndFormat();
    return true;
}

void VRSound::initWithCodec(AVCodecContext* codec) {
    al->state = AL_STOPPED;
    al->resampler = 0;
    al->codec = codec;

    updateSampleAndFormat();

#ifdef OLD_LIBAV
        al->frame = avcodec_alloc_frame(); // Allocate frame
#else
        al->frame = av_frame_alloc(); // Allocate frame
#endif

    interface = VRSoundInterface::create();
    initiated = true;
}

void VRSound::playBuffer(VRSoundBufferPtr frame) { if (interface) interface->queueFrame(frame); }
void VRSound::addBuffer(VRSoundBufferPtr frame) { ownedBuffer.push_back(frame); }

void VRSound::queuePacket(AVPacket* packet) {
    if (doUpdate) {
        interface->updateSource(pitch, gain, hpass, lpass);
        doUpdate = false;
    }

    for (auto frame : extractPacket(packet)) {
        if (interrupt) { cout << "interrupt sound\n"; break; }
        interface->queueFrame(frame);
    }
}

int avcodec_decode_audio4(AVCodecContext* avctx, AVFrame* frame, int* got_frame, AVPacket* avpkt) {
    int ret = avcodec_receive_frame(avctx, frame);
    if (ret == 0) *got_frame = true;
    if (ret == AVERROR(EAGAIN)) ret = 0;
    if (ret == 0) ret = avcodec_send_packet(avctx, avpkt);
    if (ret == AVERROR(EAGAIN)) ret = 0;
    else if (ret < 0) return ret;
    else ret = avpkt->size;
    return ret;
}

vector<VRSoundBufferPtr> VRSound::extractPacket(AVPacket* packet) {
    vector<VRSoundBufferPtr> res;
    //cout << "VRSound::queuePacket, alIsSource1: " << bool(alIsSource(source) == AL_TRUE) << endl;
    while (packet->size > 0) { // Decodes audio data from `packet` into the frame
        if (interrupt) { cout << "interrupt sound\n"; break; }

        int finishedFrame = 0;
        int len = avcodec_decode_audio4(al->codec, al->frame, &finishedFrame, packet);
        if (len < 0) { cout << "decoding error\n"; break; }

        if (finishedFrame) {
            if (interrupt) { cout << "interrupt sound\n"; break; }

            // Decoded data is now available in frame->data[0]
            int linesize;

#ifdef __APPLE__
            int data_size = av_samples_get_buffer_size(&linesize, al->codec->ch_layout.nb_channels, al->frame->nb_samples, al->codec->sample_fmt, 0);
#else
            int data_size = av_samples_get_buffer_size(&linesize, al->codec->channels, al->frame->nb_samples, al->codec->sample_fmt, 0);
#endif

            ALbyte* frameData;
            if (al->resampler != 0) {
                frameData = (ALbyte *)av_malloc(data_size*sizeof(uint8_t));
                swr_convert( al->resampler,
                            (uint8_t **)&frameData,
                            al->frame->nb_samples,
                            (const uint8_t **)al->frame->data,
                            al->frame->nb_samples);
            } else frameData = (ALbyte*)al->frame->data[0];

            auto frame = VRSoundBuffer::wrap(frameData, data_size, frequency, al->format);
            res.push_back(frame);
        }

        //There may be more than one frame of audio data inside the packet.
        packet->size -= len;
        packet->data += len;
    } // while packet.size > 0
    return res;
}

void VRSound::playFrame() {
    //cout << "VRSound::playFrame " << interrupt << " " << this << " playing: " << (al->state == AL_PLAYING) << " N buffer: " << getQueuedBuffer() << endl;

    bool internal = (ownedBuffer.size() > 0);

    if (al->state == AL_INITIAL) {
        cout << "playFrame AL_INITIAL initiated: " << initiated << " " << internal << endl;
        if (!initiated) initiate();
        if (internal) { al->state = AL_PLAYING; interrupt = false; return; }
        if (!al->context) { /*cout << "VRSound::playFrame Warning: no context" << endl;*/ return; }
#ifdef OLD_LIBAV
        al->frame = avcodec_alloc_frame(); // Allocate frame
#else
        al->frame = av_frame_alloc(); // Allocate frame
#endif
        av_seek_frame(al->context, stream_id, 0,  AVSEEK_FLAG_FRAME);
        al->state = AL_PLAYING;
        interrupt = false;
    }

    if (al->state == AL_PLAYING) {
        if (interface->getQueuedBuffer() > 5) {
            interface->recycleBuffer();
            return;
        }

        bool endReached = false;

        if (!internal) {
            auto avrf = av_read_frame(al->context, &al->packet);
            if (interrupt || avrf < 0) { // End of stream, done decoding
                if (al->packet.data) av_packet_unref(&al->packet);
                av_free(al->frame);
                endReached = true;
            } else {
                if (al->packet.stream_index != stream_id) { cout << "skip non audio\n"; return; } // Skip non audio packets
                queuePacket(&al->packet);
            }
        } else {
            if (nextBuffer < ownedBuffer.size()) {
                interface->queueFrame(ownedBuffer[nextBuffer]);
                nextBuffer++;
            } else endReached = true;
        }

        if (endReached) {
            al->state = loop ? AL_INITIAL : AL_STOPPED;
            cout << "endReached, stop? " << (al->state == AL_STOPPED) << endl;
            if (al->state == AL_STOPPED && callback) {
                auto scene = VRScene::getCurrent();
                if (scene) scene->queueJob(callback);
            }
            return;
        }
    } // while more packets exist inside container.
}

void VRSound::update3DSound() {
    if (!poseBeacon) return;
    VRTransformPtr head = headBeacon;
    if (!head) head = VRScene::getCurrent()->getActiveCamera();
    auto pose = head->getPoseTo(poseBeacon);

    if (!lastPose) lastPose = Pose::create(*pose);
    velocity = float(pose->pos().dist(lastPose->pos()));
    lastPose->setPos(pose->pos());
    interface->updatePose(pose, velocity);
}


struct OutputStream {
    int64_t next_pts = 0;
    AVStream *st = 0;
    AVCodecContext *enc = 0;
    AVFrame *frame = 0;
    AVFrame *tmp_frame = 0;
    SwrContext *avr = 0;
};

void add_audio_stream(OutputStream *ost, AVFormatContext *oc, enum AVCodecID codec_id) {
    const AVCodec* codec = avcodec_find_encoder(codec_id);
	cout << " --- add_audio_stream, found codec: " << codec->name << endl;
    if (!codec) { fprintf(stderr, "codec not found\n"); return; }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) { fprintf(stderr, "Could not alloc stream\n"); return; }

    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) { fprintf(stderr, "Could not alloc an encoding context\n"); return; }
    ost->enc = c;

    /* put sample parameters */
    c->sample_rate    = codec->supported_samplerates ? codec->supported_samplerates[0] : 44100;
#ifdef __APPLE__
    c->ch_layout      = codec->ch_layouts            ? codec->ch_layouts[0]            : AVChannelLayout(AV_CHANNEL_LAYOUT_STEREO);
#else
    c->channel_layout = codec->channel_layouts       ? codec->channel_layouts[0]       : AV_CH_LAYOUT_STEREO;
#endif
    c->sample_fmt     = codec->sample_fmts           ? codec->sample_fmts[0]           : AV_SAMPLE_FMT_S16;
#ifndef __APPLE__
    c->channels       = av_get_channel_layout_nb_channels(c->channel_layout);
#endif
    c->bit_rate       = 64000;

	ost->st->time_base.num = 1;
	ost->st->time_base.den = c->sample_rate;

    // some formats want stream headers to be separate
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* initialize sample format conversion;
     * to simplify the code, we always pass the data through lavr, even
     * if the encoder supports the generated format directly -- the price is
     * some extra data copying;
     */
    ost->avr = swr_alloc();
    if (!ost->avr) { fprintf(stderr, "Error allocating the resampling context\n"); return; }

#ifdef __APPLE__
    auto mono = AVChannelLayout(AV_CHANNEL_LAYOUT_MONO);
    av_opt_set_chlayout      (ost->avr, "in_channel_layout",  &mono,             0);
    av_opt_set_sample_fmt    (ost->avr, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int           (ost->avr, "in_sample_rate",     22050,             0);
    av_opt_set_chlayout      (ost->avr, "out_channel_layout", &c->ch_layout,      0);
    av_opt_set_sample_fmt    (ost->avr, "out_sample_fmt",     c->sample_fmt,     0);
    av_opt_set_int           (ost->avr, "out_sample_rate",    c->sample_rate,    0);
#else
    av_opt_set_channel_layout(ost->avr, "in_channel_layout",  AV_CH_LAYOUT_MONO, 0);
    av_opt_set_sample_fmt    (ost->avr, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int           (ost->avr, "in_sample_rate",     22050,             0);
    av_opt_set_channel_layout(ost->avr, "out_channel_layout", c->channel_layout, 0);
    av_opt_set_sample_fmt    (ost->avr, "out_sample_fmt",     c->sample_fmt,     0);
    av_opt_set_int           (ost->avr, "out_sample_rate",    c->sample_rate,    0);
#endif



    int ret = swr_init(ost->avr);
    if (ret < 0) { fprintf(stderr, "Error opening the resampling context\n"); return; }
}

#ifdef __APPLE__
AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt, AVChannelLayout ch_layout, int sample_rate, int nb_samples) {
#else
AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples) {
#endif

    AVFrame* frame = av_frame_alloc();
    if (!frame) { fprintf(stderr, "Error allocating an audio frame\n"); return 0; }

#ifdef __APPLE__
    frame->ch_layout = ch_layout;
#else
    frame->channel_layout = channel_layout;
#endif
    frame->format = sample_fmt;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        int ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) { fprintf(stderr, "Error allocating an audio buffer\n"); return 0; }
    }

    return frame;
}

bool open_audio(AVFormatContext *oc, OutputStream *ost) {
    int nb_samples, ret;

    AVCodecContext* c = ost->enc;

	const AVCodec* codec = avcodec_find_encoder(oc->oformat->audio_codec);
	cout << " --- open_audio, found codec: " << codec->name << endl;

	//AVCodec* codec = avcodec_find_encoder(c->codec_id);
    if (codec == NULL) { fprintf(stderr, "could not find codec\n"); return false; }

    cout << "Open audio codec: " << c << " " << codec << endl;
	int r = -1;
	for (int i=0; i<10; i++) {
		r = avcodec_open2(c, codec, NULL);
		if (r >= 0) {
			cout << "Opened codec on " << i+1 << "-th try" << endl;
			break;
		}
	}

	if (r < 0) {
		fprintf(stderr, "Could not open codec!\n");
		return false;
	}

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) nb_samples = 10000;
    else nb_samples = c->frame_size;

	if (nb_samples == 0) {
		cout << "Warning, no samples set, set to 10000" << endl;
		nb_samples = 10000;
	}

    cout << "Allocate audio frames: " << nb_samples << endl;

#ifdef __APPLE__
    ost->frame     = alloc_audio_frame(c->sample_fmt, c->ch_layout, c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, AVChannelLayout(AV_CHANNEL_LAYOUT_MONO), 22050, nb_samples);
#else
    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_MONO, 22050, nb_samples);
#endif

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) { fprintf(stderr, "Could not copy the stream parameters\n"); return false; }

	return true;
}

AVFrame* get_audio_frame(OutputStream *ost, VRSoundBufferPtr buffer) {
    AVFrame* frame = ost->tmp_frame;
    if (!frame || !buffer) return 0;

    int16_t* src = (int16_t*)buffer->data;
    int16_t* dst = (int16_t*)frame->data[0];

    frame->nb_samples = buffer->size/2;
    for (int j = 0; j < frame->nb_samples; j++) {
        int v = src[j]; // audio data
        *dst++ = v;
        //for (int i = 0; i < ost->enc->channels; i++) *dst++ = v; // this was wrong, exploded on windows
    }

    return frame;
}

void setupMP3Decoder(AVCodecContext** ctx, AVFormatContext** fmtctx) {
    const AVInputFormat* fmt = av_find_input_format("mp3");
    auto codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
    auto context = avcodec_alloc_context3(codec);
    *ctx = context;

#ifdef __APPLE__
    context->ch_layout = AVChannelLayout(AV_CHANNEL_LAYOUT_MONO);
#else
    context->channel_layout = AV_CH_LAYOUT_MONO;
    context->channels       = 1;
#endif
    context->sample_fmt     = AV_SAMPLE_FMT_S32P;
    context->sample_rate    = 44100;
    context->bit_rate       = 64000;
    if (fmt->flags & AVFMT_GLOBALHEADER) context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    int ret = avcodec_open2(context, NULL, NULL);

    auto fmtcontext = avformat_alloc_context();
    *fmtctx = fmtcontext;
    fmtcontext->iformat = (AVInputFormat*)fmt;


    AVStream* stream = avformat_new_stream(fmtcontext, NULL);
    //stream->codec = context;
    stream->time_base.num = 1;
    stream->time_base.den = context->sample_rate;
    ret = avcodec_parameters_from_context(stream->codecpar, context);
    stream->codecpar->format = AV_SAMPLE_FMT_S32P;
    if (ret < 0) { fprintf(stderr, "Could not copy the stream parameters\n"); return; }

    av_dump_format(fmtcontext, 0, NULL, 0);
}

void testDecodePacket(AVPacket& pkt) {
    static AVCodecContext* context = 0;
    static AVFormatContext* fmtcontext = 0;
    if (!context) setupMP3Decoder(&context, &fmtcontext);
    cout << "   test decode packet.. " << pkt.size << " " << context << endl;
    int got_frame = 0;
    int finishedFrame = 0;
    AVFrame* frame = av_frame_alloc();

    //context->codec->decode(ost->enc, frame, &got_frame, &pkt);
    avcodec_decode_audio4(context, frame, &finishedFrame, &pkt);
    cout << "   ..test decode packet done! " << finishedFrame << " " << frame->pkt_size << endl;
    av_frame_free(&frame);
}

void encode_audio_frame(AVFormatContext *oc, OutputStream *ost, AVFrame *frame) {
    AVPacket pkt = { 0 }; // data and size must be 0;
    av_init_packet(&pkt);

    auto writePacket = [&]() {
        pkt.stream_index = ost->st->index;
        //cout << "write packet" << endl;

        //cout << "   rescale packet" << endl;
        av_packet_rescale_ts(&pkt, ost->enc->time_base, ost->st->time_base);

        //cout << "    write frame, pkt: " << pkt.size << endl;
        if (av_interleaved_write_frame(oc, &pkt) != 0) { fprintf(stderr, "Error while writing audio frame\n"); return; }
        //cout << "     write frame done" << endl;
    };

    int ret = avcodec_send_frame(ost->enc, frame);
    /*cout << " send packet " << frame << ", ret: " << ret;
	if (frame) cout << ", " << frame->nb_samples;
	cout << endl;*/
    if ( ret != AVERROR_EOF && ret != AVERROR(EAGAIN) && ret != 0) { fprintf(stderr, "Could not send frame\n"); return; }

    while (ret >= 0) {
        ret = avcodec_receive_packet(ost->enc, &pkt);
        //cout << "  received packet, ret: " << ret << ", pkt: " << pkt.size << endl;
        if (ret == AVERROR(EAGAIN)) continue;
        if (ret == AVERROR_EOF) return;
        if (ret < 0) { fprintf(stderr, "!!! Error during encoding\n"); return; }
        writePacket();
    }
}

void VRSound::write_buffer(AVFormatContext *oc, OutputStream *ost, VRSoundBufferPtr buffer) {
    //cout << "  get audio frame" << endl;
    AVFrame* frame = get_audio_frame(ost, buffer);
    if (!frame) return;

    //cout << "  resample convert " << frame->linesize[0] << " " << frame->nb_samples << endl;
    int ret = swr_convert(ost->avr, NULL, 0, (const uint8_t **)frame->extended_data, frame->nb_samples);
    if (ret < 0) { fprintf(stderr, "Error feeding audio data to the resampler\n"); return; }

    //cout << "  write buffer" << endl;
    while ((frame) || (!frame && swr_get_out_samples(ost->avr, 0))) {
        // when we pass a frame to the encoder, it may keep a reference to it internally; make sure we do not overwrite it here
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0) return;

        /* the difference between the two avresample calls here is that the
         * first one just reads the already converted data that is buffered in
         * the lavr output buffer, while the second one also flushes the
         * resampler */
        ret = swr_convert(ost->avr, ost->frame->extended_data, ost->frame->nb_samples, NULL, 0);
        if (ret < 0) { fprintf(stderr, "Error while resampling\n"); return; }

        if (frame && ret != ost->frame->nb_samples) {
            //fprintf(stderr, "Too few samples returned from lavr\n");
            if (ret == 0) {
                //cout << " VRSound::write_buffer: Too few samples returned from lavr! expected: " << ost->frame->nb_samples << ", got: " << ret << endl;
                return;
            }
        }

        //cout << "  encode frame" << endl;
        ost->frame->nb_samples = ret;
        ost->frame->pts        = ost->next_pts;
        ost->next_pts         += ost->frame->nb_samples;
        encode_audio_frame(oc, ost, ret ? ost->frame : NULL);
        //cout << "   encode frame done" << endl;
    }
}

void close_stream(AVFormatContext *oc, OutputStream *ost) {
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    swr_free(&ost->avr);
}

void VRSound::exportToFile(string path) {
    OutputStream audio_st;
    const char *filename = path.c_str();
#ifndef _WIN32
#ifndef __APPLE__
    av_register_all();
#endif
#endif

	cout << "sound exportToFile: " << filename << endl;

    auto fmt = av_guess_format(NULL, filename, NULL);
    if (!fmt) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        fmt = av_guess_format("mpeg", NULL, NULL);
    }
    if (!fmt) { fprintf(stderr, "Could not find suitable output format\n"); return; }

    AVFormatContext* oc = avformat_alloc_context();
    if (!oc) { fprintf(stderr, "Memory error\n"); return; }

    oc->oformat = fmt;
	//cout << "pass filename to av context: " << sizeof(oc->filename) << endl;
    //snprintf(oc->filename, sizeof(oc->filename), "%s", filename);

    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		add_audio_stream(&audio_st, oc, fmt->audio_codec);
		cout << "added audio stream" << endl;
	}

    if (!open_audio(oc, &audio_st)) return;
    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            return;
        }
    }

    /* Write the stream header, if any. */
    avformat_write_header(oc, NULL);
    //int i=0;
    for (auto buffer : ownedBuffer) {
        write_buffer(oc, &audio_st, buffer);
        /*i++;
        if (i == 4) { ownedBuffer.clear(); break; }*/
    }

    encode_audio_frame(oc, &audio_st, NULL); // flush
    av_write_trailer(oc);

    // cleanup
    close_stream(oc, &audio_st);
    if (!(fmt->flags & AVFMT_NOFILE)) avio_close(oc->pb);
    avformat_free_context(oc);
}

void VRSound::writeStreamData(const string& data) {
    //cout << " custom_io_write " << data.size() << endl;
    //if (data.size() < 200) cout << endl << data << endl;
    for (auto& cli : udpClients) cli->send(data, "", false);
    //doFrameSleep(0, 60);
}

#ifdef __APPLE__
int custom_io_write(void* opaque, const uint8_t* buffer, int32_t N) {
#else
int custom_io_write(void* opaque, uint8_t* buffer, int32_t N) {
#endif
    VRSound* sound = (VRSound*)opaque;
    string data((char*)buffer, N);
    if (sound && N > 0) sound->writeStreamData(data);
    return N;
}

void VRSound::flushPackets() { // deprecated?
    if (!muxer || !audio_ost) return;
    encode_audio_frame(muxer, audio_ost, NULL); // flush last frames
}

bool VRSound::addOutStreamClient(VRNetworkClientPtr client, string method) {
    udpClients.push_back(client);
    if (method == "raw") return true;

    audio_ost = new OutputStream();
#ifndef _WIN32
#ifndef __APPLE__
    av_register_all();
#endif
#endif

    //AVOutputFormat* fmt = av_guess_format("opus", NULL, NULL);
    auto fmt = av_guess_format("mp3", NULL, NULL);
    //AVOutputFormat* fmt = av_guess_format("matroska", "test.mkv", NULL);
    if (!fmt) { fprintf(stderr, "Could not find suitable output format\n"); return false; }

    muxer = avformat_alloc_context();
    if (!muxer) { fprintf(stderr, "Memory error\n"); return false; }

    muxer->oformat = fmt;
    //muxer->oformat->audio_codec = AV_CODEC_ID_OPUS;
    //snprintf(muxer->filename, sizeof(muxer->filename), "%s", filename);

    int avio_buffer_size = 1024;
    unsigned char* avio_buffer = (unsigned char*)av_malloc(avio_buffer_size);
    AVIOContext* custom_io = avio_alloc_context ( avio_buffer, avio_buffer_size, 1, (void*)this, NULL, &custom_io_write, NULL);
    muxer->pb = custom_io;

    cout << " add audio stream" << endl;
    add_audio_stream(audio_ost, muxer, fmt->audio_codec);
    if (!open_audio(muxer, audio_ost)) return false;
    av_dump_format(muxer, 0, NULL, 1);

    // header
    cout << " write header" << endl;
    AVDictionary* options = NULL;
    av_dict_set(&options, "live", "1", 0);
    avformat_write_header(muxer, &options);
    return true;
}

bool VRSound::setupOutStream(string url, int port, string method) { // TODO: make a udpClients map instead of vector
    auto cli = VRUDPClient::create("sound-out");
    cli->connect(url, port);
    return addOutStreamClient(cli, method);
}

void VRSound::streamBuffer(VRSoundBufferPtr frame, string method) {
    if (!frame || frame->size == 0) return;
    if (method == "mp3") write_buffer(muxer, audio_ost, frame);
    if (method == "raw") writeStreamData( string((char*)frame->data, frame->size) );
}

void VRSound::closeStream(bool keepOpen) {
    cout << "VRSound::closeStream " << keepOpen << endl;
    flushPackets();
    av_write_trailer(muxer);

    // cleanup
    close_stream(muxer, audio_ost);
    delete audio_ost;
    audio_ost = 0;
    avformat_free_context(muxer);
    muxer = 0;
    if (!keepOpen) udpClients.clear();
}

void VRSound::streamTo(string url, int port, bool keepOpen) {
    setupOutStream(url, port);
    for (auto buffer : ownedBuffer) streamBuffer(buffer);
    closeStream(keepOpen);

    // test with
    //   ffplay -f mp3 -vn -listen 1 -i http://localhost:1234
    //   ffplay -f mp3 -vn -listen 1 -i tcp://127.0.0.1:1234

    //  ffplay starts playback if the stream is not too short, or the tcp connection is closed
}


struct InputStream {
    string header;
    string tsse;

    InputStream(string d) : header(d) {
        cout << "init InputStream: " << d << endl;
        if (startsWith(d, "ID3")) { // mp3
            tsse = splitString(d, "TSSE")[1]; // TODO: find out how to use the parameters!
            cout << " detected mp3 stream, tsse = " << tsse << endl;
        }
    }
};

string VRSound::onStreamData(string data, bool stereo) {
	if (!audio_ist) {
        if (!initiated) initiate();
        audio_ist = new InputStream(data);

        auto fmt = av_find_input_format("mp3");
        auto codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
        al->codec = avcodec_alloc_context3(codec);

		if (stereo) { // windows stream
			al->codec->sample_fmt     = AV_SAMPLE_FMT_FLTP;
			al->codec->sample_rate    = 44100;
#ifdef __APPLE__
			al->codec->ch_layout      = AVChannelLayout(AV_CHANNEL_LAYOUT_STEREO);
#else
			al->codec->channel_layout = AV_CH_LAYOUT_STEREO;
			al->codec->channels       = 2;
#endif
			al->codec->bit_rate       = 96000;
		} else { // linux stream
			al->codec->sample_fmt     = AV_SAMPLE_FMT_S32P;
			al->codec->sample_rate    = 44100;
#ifdef __APPLE__
			al->codec->ch_layout      = AVChannelLayout(AV_CHANNEL_LAYOUT_MONO);
#else
			al->codec->channel_layout = AV_CH_LAYOUT_MONO;
			al->codec->channels       = 1;
#endif
			al->codec->bit_rate       = 64000;
		}

        if (fmt->flags & AVFMT_GLOBALHEADER) al->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        int ret = avcodec_open2(al->codec, NULL, NULL);

        al->context = avformat_alloc_context();
        al->context->iformat = fmt;

        AVStream* stream = avformat_new_stream(al->context, NULL);
        //stream->codec = al->codec;
        stream->time_base.num = 1;
        stream->time_base.den = al->codec->sample_rate;
        ret = avcodec_parameters_from_context(stream->codecpar, al->codec);
        if (stereo) stream->codecpar->format = AV_SAMPLE_FMT_FLTP;
        else        stream->codecpar->format = AV_SAMPLE_FMT_S32P;
        if (ret < 0) { fprintf(stderr, "Could not copy the stream parameters\n"); return ""; }

        av_dump_format(al->context, 0, NULL, 0);

        al->frame = av_frame_alloc(); // Allocate frame
        al->packet = { 0 };
        av_init_packet(&al->packet);

        updateSampleAndFormat();
    } else {
        int r = av_packet_from_data(&al->packet, (uint8_t*)&data[0], int(data.size()));
        if (r < 0 ) cout << "  av_packet_from_data failed with " << r << endl;
        queuePacket(&al->packet);
    }

    return "";
}

bool VRSound::listenStream(int port, bool stereo) {
    auto streamCb = bind(&VRSound::onStreamData, this, placeholders::_1, stereo);

    if (!udpServer) {
        udpServer = VRUDPServer::create("sound-in");
        udpServer->onMessage(streamCb);
        udpServer->listen(port);
    }

#ifndef _WIN32
#ifndef __APPLE__
    av_register_all();
#endif
#endif
    return true;
}

bool VRSound::playPeerStream(VRNetworkClientPtr client, bool stereo) {
    auto streamCb = bind(&VRSound::onStreamData, this, placeholders::_1, stereo);
    client->onMessage(streamCb);
#ifndef _WIN32
#ifndef __APPLE__
    av_register_all();
#endif
#endif
    return true;
}


// carrier amplitude, carrier frequency, carrier phase, modulation amplitude, modulation frequency, modulation phase, packet duration
void VRSound::synthesize(float Ac, float wc, float pc, float Am, float wm, float pm, float duration, int maxQueued) {
    if (!initiated) initiate();

    if (maxQueued >= 0) {
        interface->recycleBuffer();
        if ( interface->getQueuedBuffer() >= maxQueued ) return;
    }

    int sample_rate = 22050;
    size_t buf_size = size_t(duration * sample_rate);
    buf_size += buf_size%2;
    vector<short> samples(buf_size);

    double tmp = 0;
    for(uint i=0; i<buf_size; i++) {
        double t = i*2*Pi/sample_rate + synth_t0;
        samples[i] = short( Ac * sin( wc*t + pc + Am*sin(wm*t + pm) ) );
        tmp = t;
    }
    synth_t0 = tmp;

    auto frame = VRSoundBuffer::wrap((ALbyte*)&samples[0], int(samples.size()*sizeof(short)), sample_rate, AL_FORMAT_MONO16);
    playBuffer(frame);
}

vector<short> VRSound::synthSpectrum(vector<double> spectrum, uint sample_rate, float duration, float fade_factor, bool returnBuffer, int maxQueued) {
    if (!initiated) initiate();

    if (maxQueued >= 0) {
        interface->recycleBuffer();
        if ( interface->getQueuedBuffer() >= maxQueued ) return vector<short>();
    }

    /* --- fade in/out curve ---
    ::path c;
    c.addPoint(Vec3d(0,0,0), Vec3d(1,0,0));
    c.addPoint(Vec3d(1,1,0), Vec3d(1,0,0));
    c.compute(sample_rate);
    */

    size_t buf_size = size_t(duration * sample_rate);
    size_t fade = size_t( min(fade_factor * sample_rate, duration * sample_rate) ); // number of samples to fade at beginning and end

    // transform spectrum back to time domain using fftw3
    FFT fft;
    vector<double> out = fft.transform(spectrum, sample_rate);

    vector<short> samples(buf_size);
    for(uint i=0; i<buf_size; ++i) {
        samples[i] = short(0.5 * SHRT_MAX * out[i]); // for fftw normalization
    }

    auto calcFade = [](double& t) {
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        // P3*t³ + P2*3*t²*(1-t) + P1*3*t*(1-t)² + P0*(1-t)³
        // P0(0,0) P1(0.5,0) P2(0.5,1) P3(1,1)
        // P0(0,0) P1(0.5,0.1) P2(0.5,1) P3(1,1)
        double s = 1-t;
        return t*t*(3*s + t);
    };

    for (uint i=0; i < fade; ++i) {
        double t = double(i)/(fade-1);
        double y = calcFade(t);
        samples[i] = short(samples[i]*y);
        samples[buf_size-i-1] *= short(samples[buf_size-i-1]*y);
    }

    auto frame = VRSoundBuffer::wrap((ALbyte*)&samples[0], int(samples.size()*sizeof(short)), sample_rate, AL_FORMAT_MONO16);
    playBuffer(frame);
    return returnBuffer ? samples : vector<short>();
}

vector<short> VRSound::synthBuffer(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float duration, int maxQueued) {
    if (!initiated) initiate();

    if (maxQueued >= 0) {
        interface->recycleBuffer();
        if ( interface->getQueuedBuffer() >= maxQueued ) return vector<short>();
    }

    // play sound
    int sample_rate = 22050;
    size_t buf_size = size_t(duration * sample_rate);
    vector<short> samples(buf_size);
    double Ni = 1.0/freqs1.size();
    double T = 2*Pi/sample_rate;
    static map<int,complex<double>> phasors;
    for (uint i=0; i<buf_size; i++) {
        double k = double(i)/(buf_size-1);
        samples[i] = 0;
        for (uint j=0; j<freqs1.size(); j++) {
            double A = freqs1[j][1]*(1.0-k) + freqs2[j][1]*k;
            double f = freqs1[j][0]*(1.0-k) + freqs2[j][0]*k;

            if (!phasors.count(j)) phasors[j] = complex<double>(1,0);
            phasors[j] *= exp( complex<double>(0,T*f) );
            samples[i] += short(A*Ni*phasors[j].imag());
        }
    }
    auto frame = VRSoundBuffer::wrap((ALbyte*)&samples[0], int(samples.size()*sizeof(short)), sample_rate, AL_FORMAT_MONO16);
    playBuffer(frame);
    if (true) return samples;
    return vector<short>();
}

vector<short> VRSound::synthBufferForChannel(vector<Vec2d> freqs1, vector<Vec2d> freqs2, int channel, float duration, int maxQueued) {
    /*
        this creates synthetic sound samples based on two vectors of frequencies
        the channel is required for separation of the different static phasors
        the duration determines how long these samples will be

        this is based on the work done in synthBuffer
    */
    if (!initiated) initiate();

    if (maxQueued >= 0) {
        interface->recycleBuffer();
        if ( interface->getQueuedBuffer() >= maxQueued ) return vector<short>();
    }

    // play sound
    int sample_rate = 22050;
    size_t buf_size = size_t(duration * sample_rate);
    vector<short> samples(buf_size);
    double Ni = 1.0/freqs1.size();
    double T = 2*Pi/sample_rate;
    static map<int,map<int,complex<double>>> phasors;

    if (!phasors.count(channel)) {
        phasors[channel] = map<int,complex<double>>();
    }

    for (uint i=0; i<buf_size; i++) {
        double k = double(i)/(buf_size-1);
        samples[i] = 0;
        for (uint j=0; j<freqs1.size(); j++) {
            double A = freqs1[j][1]*(1.0-k) + freqs2[j][1]*k;
            double f = freqs1[j][0]*(1.0-k) + freqs2[j][0]*k;

            if (!phasors[channel].count(j)) {
                phasors[channel][j] = complex<double>(1,0);
            }
            phasors[channel][j] *= exp( complex<double>(0,T*f) );
            samples[i] += short(A*Ni*phasors[channel][j].imag());
        }
    }
    return samples;
}

void VRSound::synthBufferOnChannels(vector<vector<Vec2d>> freqs1, vector<vector<Vec2d>> freqs2, float duration, int maxQueued) {
    /*
        this expects two vectors of input vectors consisting of <frequency, amplitude> tuples
        the vectors should contain a vector with input data for every channel
        the duration determines how long the generated sound samples will be played on the audio buffer
    */
    if (!initiated) initiate();

    if (maxQueued >= 0) {
        interface->recycleBuffer();
        if ( interface->getQueuedBuffer() >= maxQueued ) return;
    }

    auto num_channels = freqs1.size();
    if (num_channels != freqs2.size()) {
        cout << "synthBufferOnChannels - sizes don't match - freqs1 = " << num_channels << " freqs2 = " << freqs2.size() << endl;
        return;
    }

    // vector to generate all synth buffers into
    vector<vector<short>> synth_buffer(num_channels);

    // generate a new buffer with size for all buffers * amount of channels
    int sample_rate = 22050;
    int buffer_size = int(ceil(sample_rate * duration));
    vector<short> buffer(buffer_size * num_channels);

    // generate synth buffers for every channel and store them for later use
    for (uint channel = 0; channel < num_channels; channel++) {
        synth_buffer[channel] = synthBufferForChannel(freqs1[channel], freqs2[channel], channel, duration);
    }

    /*
      create one big buffer composed of every synth buffer
      elements have to be in the order of the real audio channels
      element 0 is the first frame on the first channel
      element 1 the first frame on the second channel
      element 8 is the second frame on the first channel
    */
    for (int i = 0; i < buffer_size; i++) {
        for (uint channel = 0; channel < num_channels; channel++) {
            buffer[num_channels * i + channel] = synth_buffer[channel][i];
        }
    }

    // try to determine the amount of channels and their respective mapping inside OpenAL
    // play mono as default because that should output sound on every channel
    ALenum format;
    switch (num_channels) {
        case 1:
            format = AL_FORMAT_MONO16;
            break;
        case 2:
            format = AL_FORMAT_STEREO16;
            break;
        case 6:
            format = AL_FORMAT_51CHN16;
            break;
        case 8:
            format = AL_FORMAT_71CHN16;
            break;
        default:
            format = AL_FORMAT_MONO16;
    }

    auto frame = VRSoundBuffer::wrap((ALbyte*)&buffer[0], int(buffer.size()*sizeof(short)), sample_rate, format);
    playBuffer(frame);
}



double simTime = 0;
double simPhase = 0;

VRSoundBufferPtr test_genPacket(double T) {
    // tone parameters
    float Ac = 32760;
    float wc = 440;
    int sample_rate = 22050;
    float period1 = 0.2;

    // allocate frame
    size_t buf_size = size_t(T * sample_rate);
    buf_size += buf_size%2;
    auto frame = VRSoundBuffer::allocate(buf_size*sizeof(short), sample_rate, AL_FORMAT_MONO16);

    double dt = T/(buf_size-1);
    double F = simPhase;

    for(uint i=0; i<buf_size; i++) {
        double a = double(i)*dt;
        double k = simTime + a;
        //double Ak = abs(sin(k/period1));
        double Ak = 1.0-a/T;

        F += wc*2.0*Pi/sample_rate;
        while (F > 2*Pi) F -= 2*Pi;

        short v = short(Ak * Ac * sin( F ));
        ((short*)frame->data)[i] = v;
    }

    simPhase = F;
	simTime += T;
    return frame;
}

void VRSound::testMP3Write() {
	cout << "start av mp3 write test" << endl;
	cout << " AV versions: " << endl;
	cout << " codec:  " << AV_VERSION_MAJOR(LIBAVCODEC_VERSION_INT) << "." << AV_VERSION_MINOR(LIBAVCODEC_VERSION_INT) << endl;
	cout << " format: " << AV_VERSION_MAJOR(LIBAVFORMAT_VERSION_INT) << "." << AV_VERSION_MINOR(LIBAVFORMAT_VERSION_INT) << endl;

	// create data
	simTime = 0;
	simPhase = 0;
	for (int i=0; i<10; i++) {
		ownedBuffer.push_back( test_genPacket(0.3) );
	}

	string path = "test.mp3";

    OutputStream audio_st;
    const char *filename = path.c_str();
#ifndef _WIN32
#ifndef __APPLE__
    av_register_all();
#endif
#endif

	cout << "sound exportToFile: " << filename << endl;

    auto fmt = av_guess_format(NULL, filename, NULL);
    if (!fmt) { fprintf(stderr, "Could not find suitable output format\n"); return; }

    AVFormatContext* oc = avformat_alloc_context();
    if (!oc) { fprintf(stderr, "Memory error\n"); return; }

    oc->oformat = fmt;
	//cout << "pass filename to av context: " << sizeof(oc->filename) << endl;
    //snprintf(oc->filename, sizeof(oc->filename), "%s", filename);

    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		add_audio_stream(&audio_st, oc, fmt->audio_codec);
		cout << "added audio stream" << endl;
	}

    if (!open_audio(oc, &audio_st)) return;
    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
		cout << "open output file" << endl;
        if (avio_open(&oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            return;
        }
    }

    /* Write the stream header, if any. */
	cout << " start writing buffers" << endl;
    avformat_write_header(oc, NULL);
    for (auto buffer : ownedBuffer) write_buffer(oc, &audio_st, buffer);
	cout << " done writing buffers" << endl;

    encode_audio_frame(oc, &audio_st, NULL); // flush last frames
    av_write_trailer(oc);

    // cleanup
	cout << " cleanup" << endl;
	cout << "  close stream" << endl;
    close_stream(oc, &audio_st);
	cout << "  close file" << endl;
    if (!(fmt->flags & AVFMT_NOFILE)) avio_close(oc->pb);
	cout << "  free context" << endl;
    avformat_free_context(oc);

	cout << "av mp3 write test success!" << endl;
}
