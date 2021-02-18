#include "VRSound.h"
#include "VRSoundUtils.h"
#include "core/math/path.h"
#include "core/math/fft.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "VRSoundManager.h"

#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#endif

extern "C" {
#include <libavresample/avresample.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <fstream>
#include <map>
#include <climits>

#define WARN(x) \
VRGuiManager::get()->getConsole( "Errors" )->write( x+"\n" );

using namespace OSG;

template<> string typeName(const VRSound& o) { return "Sound"; }

string avErrToStr(const int& e) {
    char buf[100];
    int N = 100;
    av_strerror(e, buf, N);
    return string(buf);
}

struct VRSound::ALData {
    ALenum sample = 0;
    ALenum format = 0;
    ALenum layout = 0;
    ALenum state = AL_INITIAL;
    AVFormatContext* context = 0;
    AVAudioResampleContext* resampler = 0;
    AVCodecContext* codec = NULL;
    AVPacket packet;
    AVFrame* frame;
};

VRSound::VRSound() {
    pos = new Vec3d();
    vel = new Vec3d();

    VRSoundManager::get(); // this may init channel
    buffers = new uint[Nbuffers];
    al = shared_ptr<ALData>( new ALData() );
    reset();
}

VRSound::~VRSound() {
    close();
    delete[] buffers;
    delete pos;
    delete vel;
}

VRSoundPtr VRSound::create() { return VRSoundPtr( new VRSound() ); }

int VRSound::getState() { return al->state; }
string VRSound::getPath() { return path; }
void VRSound::setPath( string p ) { path = p; }

void VRSound::setLoop(bool loop) { this->loop = loop; doUpdate = true; }
void VRSound::setPitch(float pitch) { this->pitch = pitch; doUpdate = true; }
void VRSound::setVolume(float gain) { this->gain = gain; doUpdate = true; }
void VRSound::setUser(Vec3d p, Vec3d v) { *pos = p; *vel = v; doUpdate = true; }
void VRSound::setCallback(VRUpdateCbPtr cb) { callback = cb; }
bool VRSound::isRunning() {
    recycleBuffer();
    //cout << "isRunning " << bool(al->state == AL_PLAYING) << " " << bool(al->state == AL_INITIAL) << " " << getQueuedBuffer()<< endl;
    return al->state == AL_PLAYING || al->state == AL_INITIAL || getQueuedBuffer() != 0;
}
void VRSound::stop() { interrupt = true; loop = false; }

void VRSound::pause() {
    ALint val = -1;
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    if (val == AL_PLAYING) ALCHECK( alSourcePause(source));
}

void VRSound::resume() {
    ALint val = -1;
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
}

void VRSound::close() {
    cout << " !!! VRSound::close !!!" << endl;
    stop();
    if (source) ALCHECK( alDeleteSources(1u, &source));
    if (buffers && Nbuffers) ALCHECK( alDeleteBuffers(Nbuffers, buffers));
    if (al->context) avformat_close_input(&al->context);
    if (al->resampler) avresample_free(&al->resampler);
    al->context = 0;
    al->resampler = 0;
    init = 0;
    cout << "  VRSound::close done" << endl;
}

void VRSound::reset() { al->state = AL_STOPPED; }
void VRSound::play() { al->state = AL_INITIAL; }

void VRSound::updateSource() {
    cout << "update source, pitch: " << pitch << " gain: " << gain << endl;
    ALCHECK( alSourcef(source, AL_PITCH, pitch));
    ALCHECK( alSourcef(source, AL_MAX_GAIN, gain));
    ALCHECK( alSourcef(source, AL_GAIN, gain));
    //ALCHECK( alSource3f(source, AL_POSITION, (*pos)[0], (*pos)[1], (*pos)[2]));
    //ALCHECK( alSource3f(source, AL_VELOCITY, (*vel)[0], (*vel)[1], (*vel)[2]));
    doUpdate = false;
}

void VRSound::updateSampleAndFormat() {
    if (al->codec->channel_layout == 0) {
        if (al->codec->channels == 1) al->codec->channel_layout = AV_CH_LAYOUT_MONO;
        if (al->codec->channels == 2) al->codec->channel_layout = AV_CH_LAYOUT_STEREO;
        if (al->codec->channel_layout == 0) cout << "WARNING! channel_layout is 0.\n";
    }

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

    if (al->codec->channel_layout == AV_CH_LAYOUT_MONO) al->layout = AL_MONO_SOFT;
    if (al->codec->channel_layout == AV_CH_LAYOUT_STEREO) al->layout = AL_STEREO_SOFT;
    if (al->codec->channel_layout == AV_CH_LAYOUT_QUAD) al->layout = AL_QUAD_SOFT;
    if (al->codec->channel_layout == AV_CH_LAYOUT_5POINT1) al->layout = AL_5POINT1_SOFT;
    if (al->codec->channel_layout == AV_CH_LAYOUT_7POINT1) al->layout = AL_7POINT1_SOFT;

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
        int out_sample_fmt;
        switch(al->codec->sample_fmt) {
            case AV_SAMPLE_FMT_U8P:  out_sample_fmt = AV_SAMPLE_FMT_U8; break;
            case AV_SAMPLE_FMT_S16P: out_sample_fmt = AV_SAMPLE_FMT_S16; break;
            case AV_SAMPLE_FMT_S32P: out_sample_fmt = AV_SAMPLE_FMT_S32; break;
            case AV_SAMPLE_FMT_DBLP: out_sample_fmt = AV_SAMPLE_FMT_DBL; break;
            case AV_SAMPLE_FMT_FLTP:
            default: out_sample_fmt = AV_SAMPLE_FMT_FLT;
        }

        al->resampler = avresample_alloc_context();
        av_opt_set_int(al->resampler, "in_channel_layout",  al->codec->channel_layout, 0);
        av_opt_set_int(al->resampler, "in_sample_fmt",      al->codec->sample_fmt,     0);
        av_opt_set_int(al->resampler, "in_sample_rate",     al->codec->sample_rate,    0);
        av_opt_set_int(al->resampler, "out_channel_layout", al->codec->channel_layout, 0);
        av_opt_set_int(al->resampler, "out_sample_fmt",     out_sample_fmt,        0);
        av_opt_set_int(al->resampler, "out_sample_rate",    al->codec->sample_rate,    0);
        avresample_open(al->resampler);
    }
}

bool VRSound::initiate() {
    cout << "init sound " << path << endl;
    initiated = true;

    ALCHECK( alGenBuffers(Nbuffers, buffers) );
    for (uint i=0; i<Nbuffers; i++) free_buffers.push_back(buffers[i]);

    ALCHECK( alGenSources(1u, &source) );
    updateSource();

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

    al->codec = al->context->streams[stream_id]->codec;
    AVCodec* avcodec = avcodec_find_decoder(al->codec->codec_id);
    if (avcodec == 0) return 0;
    if (avcodec_open2(al->codec, avcodec, NULL) < 0) return 0;

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

    ALCHECK( alGenBuffers(Nbuffers, buffers) );
    for (uint i=0; i<Nbuffers; i++) free_buffers.push_back(buffers[i]);

    ALCHECK( alGenSources(1u, &source) );
    updateSource();

    initiated = true;
}

vector<pair<ALbyte*, int>> VRSound::extractPacket(AVPacket* packet) {
    vector<pair<ALbyte*, int>> res;
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
            int data_size = av_samples_get_buffer_size(&linesize, al->codec->channels, al->frame->nb_samples, al->codec->sample_fmt, 0);

            ALbyte* frameData;
            if (al->resampler != 0) {
                frameData = (ALbyte *)av_malloc(data_size*sizeof(uint8_t));
                avresample_convert( al->resampler, (uint8_t **)&frameData, linesize, al->frame->nb_samples, (uint8_t **)al->frame->data, al->frame->linesize[0], al->frame->nb_samples);
            } else frameData = (ALbyte*)al->frame->data[0];
            res.push_back(make_pair(frameData, data_size));
        }

        //There may be more than one frame of audio data inside the packet.
        packet->size -= len;
        packet->data += len;
    } // while packet.size > 0
    return res;
}

void VRSound::queueFrameData(ALbyte* frameData, int data_size) {
    ALint val = -1;
    ALuint bufid = getFreeBufferID();

    //cout << " alBufferData source: " << source << " bufid: " << bufid << " format: " << al->format << " data_size: " << data_size << " frequency: " << frequency << endl;

    ALCHECK( alBufferData(bufid, al->format, frameData, data_size, frequency));
    ALCHECK( alSourceQueueBuffers(source, 1, &bufid));
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    //cout << "  source playing? " << bool(val == AL_PLAYING) << endl;
    if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
}

void VRSound::queuePacket(AVPacket* packet) {
    for (auto data : extractPacket(packet)) {
        if (interrupt) { cout << "interrupt sound\n"; break; }
        queueFrameData(data.first, data.second);
    }
}

void VRSound::playFrame() {
    //cout << "VRSound::playFrame " << interrupt << " " << this << " playing: " << (al->state == AL_PLAYING) << " N buffer: " << getQueuedBuffer() << endl;

    if (al->state == AL_INITIAL) {
        if (!initiated) initiate();
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
        if (getQueuedBuffer() > 5) {
            recycleBuffer();
            return;
        }

        if (doUpdate) updateSource();
        auto avrf = av_read_frame(al->context, &al->packet);
        if (interrupt || avrf < 0) {
            if (al->packet.data) {
                //cout << "  free packet" << endl;
                av_packet_unref(&al->packet);
            }
            //cout << "  free frame" << endl;
            av_free(al->frame);
            al->state = loop ? AL_INITIAL : AL_STOPPED;

            if (al->state == AL_STOPPED)
                if (auto cb = callback.lock()) (*cb)();
            return;
        } // End of stream. Done decoding.

        if (al->packet.stream_index != stream_id) { cout << "skip non audio\n"; return; } // Skip non audio packets

        queuePacket(&al->packet);
    } // while more packets exist inside container.
}

void VRSound::playLocally() {
    if (!initiated) initiate();
    if (!al->context) return;
    if (doUpdate) updateSource();
#ifdef OLD_LIBAV
    al->frame = avcodec_alloc_frame(); // Allocate frame
#else
    al->frame = av_frame_alloc(); // Allocate frame
#endif
    av_seek_frame(al->context, stream_id, 0,  AVSEEK_FLAG_FRAME);

    while (av_read_frame(al->context, &al->packet) >= 0) {
        if (al->packet.stream_index != stream_id) { cout << "skip non audio\n"; return; } // Skip non audio packets

        while (al->packet.size > 0) { // Decodes audio data from `packet` into the frame
            if (interrupt) { cout << "interrupt sound\n"; break; }

            int finishedFrame = 0;
            int len = avcodec_decode_audio4(al->codec, al->frame, &finishedFrame, &al->packet);
            if (len < 0) { cout << "decoding error\n"; break; }

            if (finishedFrame) {
                if (interrupt) { cout << "interrupt sound\n"; break; }

                // Decoded data is now available in frame->data[0]
                int linesize;
                int data_size = av_samples_get_buffer_size(&linesize, al->codec->channels, al->frame->nb_samples, al->codec->sample_fmt, 0);

                ALbyte* frameData;
                if (al->resampler != 0) {
                    frameData = (ALbyte *)av_malloc(data_size*sizeof(uint8_t));
                    avresample_convert( al->resampler, (uint8_t **)&frameData, linesize, al->frame->nb_samples, (uint8_t **)al->frame->data, al->frame->linesize[0], al->frame->nb_samples);
                } else frameData = (ALbyte*)al->frame->data[0];

                ALint val = -1;
                ALuint bufid = 0;

                do { ALCHECK_BREAK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &val) ); } // recycle buffers
                while (val <= 0 && free_buffers.size() == 0);
                if (val <= 0 && free_buffers.size() == 0) { al->state = AL_STOPPED; return; } // no available buffer, stop!
                for(; val > 0; --val) {
                    ALCHECK( alSourceUnqueueBuffers(source, 1, &bufid));
                    free_buffers.push_back(bufid);
                    queuedBuffers = max(0,queuedBuffers-1);
                }

                bufid = free_buffers.front();
                free_buffers.pop_front();

                queuedBuffers += 1;
                ALCHECK( alBufferData(bufid, al->format, frameData, data_size, frequency));
                ALCHECK( alSourceQueueBuffers(source, 1, &bufid));
                ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
                if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
            }

            //There may be more than one frame of audio data inside the packet.
            al->packet.size -= len;
            al->packet.data += len;
        } // while packet.size > 0
    }

    if (al->packet.data) av_packet_unref(&al->packet);
    av_free(al->frame);
}

void VRSound::playBuffer(vector<short>& buffer, int sample_rate) {
    //cout << "playBuffer " << source << endl;
    ALint val = -1;
    ALuint buf = getFreeBufferID();
    alBufferData(buf, AL_FORMAT_MONO16, &buffer[0], buffer.size()*sizeof(short), sample_rate);
    ALCHECK( alSourceQueueBuffers(source, 1, &buf));
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
}

uint VRSound::getFreeBufferID() {
    recycleBuffer();

    if (free_buffers.size()) {
        queuedBuffers += 1;
        auto bufid = free_buffers.front();
        free_buffers.pop_front();
        return bufid;
    }

    ALuint bufid;
    alGenBuffers(1, &bufid);
    queuedBuffers += 1;
    return bufid;

    // no available buffer, stop!
    cout << "VRSound::playFrame Warning: no available buffers!" << endl;
    al->state = AL_STOPPED;
    return 0;
}

// carrier amplitude, carrier frequency, carrier phase, modulation amplitude, modulation frequency, modulation phase, packet duration
void VRSound::synthesize(float Ac, float wc, float pc, float Am, float wm, float pm, float duration) {
    if (!initiated) initiate();

    int sample_rate = 22050;
    size_t buf_size = duration * sample_rate;
    buf_size += buf_size%2;
    vector<short> samples(buf_size);

    double tmp = 0;
    for(uint i=0; i<buf_size; i++) {
        double t = i*2*Pi/sample_rate + synth_t0;
        samples[i] = Ac * sin( wc*t + pc + Am*sin(wm*t + pm) );
        tmp = t;
    }
    synth_t0 = tmp;

    playBuffer(samples, sample_rate);
}

vector<short> VRSound::synthSpectrum(vector<double> spectrum, uint sample_rate, float duration, float fade_factor, bool returnBuffer) {
    if (!initiated) initiate();

    /* --- fade in/out curve ---
    ::path c;
    c.addPoint(Vec3d(0,0,0), Vec3d(1,0,0));
    c.addPoint(Vec3d(1,1,0), Vec3d(1,0,0));
    c.compute(sample_rate);
    */

    //ALuint buf;
    //alGenBuffers(1, &buf);
    size_t buf_size = duration * sample_rate;
    uint fade = min(fade_factor * sample_rate, duration * sample_rate); // number of samples to fade at beginning and end

    // transform spectrum back to time domain using fftw3
    FFT fft;
    vector<double> out = fft.transform(spectrum, sample_rate);

    /*
    vector<double> out(sample_rate);
    fftw_plan ifft;
    ifft = fftw_plan_r2r_1d(sample_rate, &spectrum[0], &out[0], FFTW_DHT, FFTW_ESTIMATE);   //Setup fftw plan for ifft
    fftw_execute(ifft); // is output normalized?
    fftw_destroy_plan(ifft);*/

    vector<short> samples(buf_size);
    for(uint i=0; i<buf_size; ++i) {
        //samples[i] = (double)(SHRT_MAX - 1) * out[i] / (sample_rate * maxVal); // for fftw normalization
        samples[i] = 0.5 * SHRT_MAX * out[i]; // for fftw normalization
    }

    //uint flat = fade / 10;
    /*uint flat = fade / 2;

    for (uint i=0; i < fade; ++i) {
        if (i < flat) {
            samples[i] = 0;
            samples[buf_size-i-1] = 0;
        } else {
            samples[i] *= (float)(i - flat)/(fade - 1 - flat);
            samples[buf_size-i-1] *= (float)(i - flat)/(fade - 1 - flat);
        }
    }*/

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
        //double y = c.getPosition(t)[1];
        double y = calcFade(t);
        samples[i] *= y;
        samples[buf_size-i-1] *= y;
    }

    //float flat_samples = 100;
    //uint flat = min((float)fade / 2, flat_samples);
    /*uint flat_samples = 1000;
    uint flat = min(flat_samples, fade);

    for (uint i=0; i < fade; ++i) {
        if (i < flat) {
            if (i > 100 && i < 200){
                //samples[i] = 0.5 * SHRT_MAX;
                samples[buf_size-i-1] = 0.5 * SHRT_MAX;
            } else {
                //samples[i] = 0;
                samples[buf_size-i-1] = 0;
            }
        } else {
            double t = double(i - flat)/(fade - 1 - flat);
            //samples[i] *= calcFade(t);
            samples[buf_size-i-1] *= calcFade(t);
        }
    }*/

    /*for (uint i=0; i < fade*2; ++i) {
        double t = (double(i)-double(fade))/(fade-1);
        //double y = c.getPosition(t)[1];
        double y = calcFade(t);
        //samples[i] *= y;
        samples[buf_size-i-1] *= y;
    }*/

    playBuffer(samples, sample_rate);
    return returnBuffer ? samples : vector<short>();
}

vector<short> VRSound::synthBuffer(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float duration) {
    if (!initiated) initiate();
    // play sound
    int sample_rate = 22050;
    size_t buf_size = duration * sample_rate;
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
            samples[i] += A*Ni*phasors[j].imag();
        }
    }
    playBuffer(samples, sample_rate);
    if (true) return samples;
    return vector<short>();
}

vector<short> VRSound::synthBufferForChannel(vector<Vec2d> freqs1, vector<Vec2d> freqs2, int channel, float duration) {
    /*
        this creates synthetic sound samples based on two vectors of frequencies
        the channel is required for separation of the different static phasors
        the duration determines how long these samples will be

        this is based on the work done in synthBuffer
    */

    if (!initiated) initiate();
    // play sound
    int sample_rate = 22050;
    size_t buf_size = duration * sample_rate;
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
            samples[i] += A*Ni*phasors[channel][j].imag();
        }
    }
    return samples;
}

void VRSound::synthBufferOnChannels(vector<vector<Vec2d>> freqs1, vector<vector<Vec2d>> freqs2, float duration) {
    /*
        this expects two vectors of input vectors consisting of <frequency, amplitude> tuples
        the vectors should contain a vector with input data for every channel
        the duration determines how long the generated sound samples will be played on the audio buffer
    */
    auto num_channels = freqs1.size();
    if (num_channels != freqs2.size()) {
        cout << "synthBufferOnChannels - sizes don't match - freqs1 = " << num_channels << " freqs2 = " << freqs2.size() << endl;
        return;
    }

    // vector to generate all synth buffers into
    vector<vector<short>> synth_buffer(num_channels);

    // generate a new buffer with size for all buffers * amount of channels
    int sample_rate = 22050;
    size_t buffer_size = duration * sample_rate;
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
    for (uint i = 0; i < buffer_size; i++) {
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

    // actually put the samples on the audio buffer and play it
    ALint val = -1;
    ALuint buf = getFreeBufferID();
    alBufferData(buf, format, &buffer[0], buffer.size()*sizeof(short), sample_rate);
    ALCHECK( alSourceQueueBuffers(source, 1, &buf));
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
}

int VRSound::getQueuedBuffer() { return queuedBuffers; }

void VRSound::checkSource() {
    cout << " checkSource: " << source << endl;
    ALint val = -1;
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    if (val == -1) cout << endl << " checkSource FAILED!" << endl;
}

void VRSound::recycleBuffer() {
    if (!initiated) return;
    ALint val = -1;
    ALuint bufid = 0; // TODO: not working properly!!
    do { ALCHECK_BREAK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &val) ); // recycle buffers
        for(; val > 0; --val) {
            ALCHECK( alSourceUnqueueBuffers(source, 1, &bufid));
            free_buffers.push_back(bufid);
            if ( queuedBuffers > 0 ) queuedBuffers -= 1;
        }
    } while (val > 0);
}




