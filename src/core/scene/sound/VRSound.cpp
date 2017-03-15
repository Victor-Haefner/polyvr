#include "VRSound.h"
#include "VRSoundUtils.h"

extern "C" {
#include <libavresample/avresample.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#if _WIN32
#include <al.h>
#include <alc.h>
#include <alext.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif

/*
not compiling?
open a terminal and type:
sudo apt-get install libfftw3-dev
*/

#include <fstream>
#include <fftw3.h>
#include <map>

#include "contrib/rpm/arrayOut.h" // TESTING
#include <climits>

using namespace OSG;

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
    buffers = new uint[Nbuffers];
    al = shared_ptr<ALData>( new ALData() );
}

VRSound::~VRSound() {
    close();
    delete[] buffers;
}

VRSoundPtr VRSound::create() { return VRSoundPtr( new VRSound() ); }

int VRSound::getState() { return al->state; }
string VRSound::getPath() { return path; }
void VRSound::setPath( string p ) { path = p; }

void VRSound::setLoop(bool loop) { this->loop = loop; doUpdate = true; }
void VRSound::setPitch(float pitch) { this->pitch = pitch; doUpdate = true; }
void VRSound::setGain(float gain) { this->gain = gain; doUpdate = true; }
void VRSound::setUser(Vec3f p, Vec3f v) { pos = p; vel = v; doUpdate = true; }
bool VRSound::isRunning() { return al->state == AL_PLAYING; }
void VRSound::stop() { interrupt = true; }

void VRSound::close() {
    ALCHECK( alDeleteSources(1u, &source));
    ALCHECK( alDeleteBuffers(Nbuffers, buffers));
    if(al->context) avformat_close_input(&al->context);
    if(al->resampler) avresample_free(&al->resampler);
    al->context = 0;
    al->resampler = 0;
    init = 0;
}

void VRSound::reset() { al->state = AL_INITIAL; }

void VRSound::updateSource() {
    cout << "update source" << endl;
    ALCHECK( alSourcef(source, AL_PITCH, pitch));
    ALCHECK( alSourcef(source, AL_GAIN, gain));
    ALCHECK( alSource3f(source, AL_POSITION, pos[0], pos[1], pos[2]));
    ALCHECK( alSource3f(source, AL_VELOCITY, vel[0], vel[1], vel[2]));
    doUpdate = false;
}

bool VRSound::initiate() {
    cout << "init sound\n";
    initiated = true;

    ALCHECK( alGenBuffers(Nbuffers, buffers) );
    for (uint i=0; i<Nbuffers; i++) free_buffers.push_back(buffers[i]);

    ALCHECK( alGenSources(1u, &source) );
    updateSource();

    if (path == "") return 1;

    if (avformat_open_input(&al->context, path.c_str(), NULL, NULL) < 0) { cout << "ERROR! avformat_open_input failed\n"; return 0; }
    if (avformat_find_stream_info(al->context, NULL) < 0) { cout << "ERROR! avformat_find_stream_info failed\n"; return 0; }
    av_dump_format(al->context, 0, path.c_str(), 0);

    stream_id = av_find_best_stream(al->context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (stream_id == -1) return 0;

    al->codec = al->context->streams[stream_id]->codec;
    AVCodec* avcodec = avcodec_find_decoder(al->codec->codec_id);
    if (avcodec == 0) return 0;
    if (avcodec_open2(al->codec, avcodec, NULL) < 0) return 0;

    if (al->codec->channel_layout == 0) {
        if (al->codec->channels == 1) al->codec->channel_layout = AV_CH_LAYOUT_MONO;
        if (al->codec->channels == 2) al->codec->channel_layout = AV_CH_LAYOUT_STEREO;
        if (al->codec->channel_layout == 0) cout << "WARNING! channel_layout is 0.\n";
    }

    frequency = al->codec->sample_rate;
    al->format = AL_FORMAT_MONO16;
    AVSampleFormat sfmt = al->codec->sample_fmt;

    if (sfmt == AV_SAMPLE_FMT_NONE) cout << "unsupported format: none\n";

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

    return true;
}

void VRSound::playFrame() {
    cout << "play frame " << endl;
    if (al->state == AL_INITIAL) {
        cout << "reset sound " << endl;
        if (!initiated) initiate();
        if (!al->context) return;
        al->frame = avcodec_alloc_frame();
        av_seek_frame(al->context, stream_id, 0,  AVSEEK_FLAG_FRAME);
        al->state = AL_PLAYING;
    }

    int len;
    if (al->state == AL_PLAYING) {
        if (doUpdate) updateSource();
        auto avrf = av_read_frame(al->context, &al->packet);
        if (interrupt || avrf < 0) {
            if (al->packet.data) {
                cout << "  free packet" << endl;
                av_free_packet(&al->packet);
            }
            cout << "  free frame" << endl;
            av_free(al->frame);
            al->state = loop ? AL_INITIAL : AL_STOPPED;
            return;
        } // End of stream. Done decoding.

        if (al->packet.stream_index != stream_id) { cout << "skip non audio\n"; return; } // Skip non audio packets

        while (al->packet.size > 0) { // Decodes audio data from `packet` into the frame
            if (interrupt) { cout << "interrupt sound\n"; break; }

            int finishedFrame = 0;
            len = avcodec_decode_audio4(al->codec, al->frame, &finishedFrame, &al->packet);
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
    } // while more packets exist inside container.
}

void VRSound::play() {
    if (!initiated) initiate();
    if (!al->context) return;
    if (doUpdate) updateSource();

    al->frame = avcodec_alloc_frame();
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

    if (al->packet.data) av_free_packet(&al->packet);
    av_free(al->frame);
}

void VRSound::playBuffer(short* buffer, size_t N, int sample_rate) {
    recycleBuffer();

    ALint val = -1;
    ALuint buf;
    alGenBuffers(1, &buf);
    alBufferData(buf, AL_FORMAT_MONO16, buffer, N, sample_rate);

    queuedBuffers += 1;
    ALCHECK( alSourceQueueBuffers(source, 1, &buf));
    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
    if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
}

// carrier amplitude, carrier frequency, carrier phase, modulation amplitude, modulation frequency, modulation phase, packet duration
void VRSound::synthesize(float Ac, float wc, float pc, float Am, float wm, float pm, float duration) {
    if (!initiated) initiate();

    int sample_rate = 22050;
    size_t buf_size = duration * sample_rate;
    buf_size += buf_size%2;
    short* samples = new short[buf_size];

    for(uint i=0; i<buf_size; i++) {
        float t = i*2*Pi/sample_rate;
        samples[i] = Ac * sin( wc*t + pc + Am*sin(wm*t + pm) );
    }

    playBuffer(samples, buf_size, sample_rate);
    delete samples;
}


void VRSound::synthesizeSpectrum(double* spectrum, uint sample_rate, float duration, float fade_factor) {
    if (!initiated) initiate();

    //ALuint buf;
    //alGenBuffers(1, &buf);
    size_t buf_size = duration * sample_rate;
    uint fade = fade_factor * sample_rate; // number of samples to fade at beginning and end

    // transform spectrum back to time domain using fftw3
    double* out = new double[sample_rate];
    // create plan
    fftw_plan ifft;
    //out = (double *) malloc(size*sizeof(double));

    ifft = fftw_plan_r2r_1d(sample_rate, spectrum, out, FFTW_DHT, FFTW_ESTIMATE);   //Setup fftw plan for ifft

    fftw_execute(ifft); // is output normalized?

    fftw_destroy_plan(ifft);

    short* samples = new short[buf_size];
    for(uint i=0; i<buf_size; ++i) {
        //samples[i] = (double)(SHRT_MAX - 1) * out[i] / (sample_rate * maxVal); // for fftw normalization
        samples[i] = 0.5 * SHRT_MAX * out[i]; // for fftw normalization
    }
    for (uint i=0; i < fade; ++i) {
        samples[i] *= i/(fade-1);
        samples[buf_size-i-1] *= i/(fade-1);
    }
//#define SPECTRUM_OUTPUT
#ifdef SPECTRUM_OUTPUT
    arrayToFile a2f_spectrum("../spectrumTestData/synthesizeSpectrum", spectrum, sample_rate); // TESTING
    arrayToFile a2f_double("../spectrumTestData/synthesizeAudioDouble", out, sample_rate); // TESTING
    arrayToFile a2f_short("../spectrumTestData/synthesizeAudioShort", samples, sample_rate); // TESTING
#endif
    delete out;

    playBuffer(samples, buf_size, sample_rate);
    delete samples;

}

void VRSound::synthBuffer(vector<Vec2d> freqs1, vector<Vec2d> freqs2, float duration) {
    if (!initiated) initiate();

    // fourier transform TODO!!
    /*vector<Vec2d> wave;
    int N = frequencies.size();
    wave.resize(N);
    Vec2d* in = &frequencies[0];
    Vec2d* out = &wave[0];
    fftw_plan p = fftw_plan_dft_1d(N, (double(*)[2])in, (double(*)[2])out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    ofstream synthIn;
    ofstream synthOut;
    synthIn.open("synthIn.dat");
    synthOut.open("synthOut.dat");
    for (int i=0; i<N; i++) {
        synthIn << i << " " << frequencies[i][0] << endl;
        synthOut << i << " " << wave[i][0] << endl;
    }
    synthIn.close();
    synthOut.close();
    auto interpolate = [&](const float& t) {
        int t1 = floor(t);
        int t2 = ceil(t);
        if (t2 >= N) return 0.0;
        return (wave[t2][1]-wave[t1][1])*(t-t1) + wave[t1][1];
    };*/

    // play sound
    int sample_rate = 22050;
    //int sample_rate = 10;
    size_t buf_size = duration * sample_rate;
    buf_size += buf_size%2;
    short* samples = new short[buf_size];

    float Ni1 = 1.0/freqs1.size();
    float Ni2 = 1.0/freqs2.size();

    static map<int, float> ma0;
    static map<int, float> ma;
    static float t0 = 0;
    static float last_sample  = 0;
    static float last_sample2 = 0;

    for (auto fA : freqs1) {
        int f = fA[0];
        if (!ma0.count(f)) ma0[f] = 0;
        if (!ma.count(f)) ma[f] = 0;
    }
    for (auto fA : freqs2) {
        int f = fA[0];
        if (!ma0.count(f)) ma0[f] = 0;
        if (!ma.count(f)) ma[f] = 0;
    }

    float t;
    for (uint i=0; i<buf_size; i++) {
        float k = float(i)/buf_size;
        t = i*2*Pi/sample_rate;

        float w = 1.0;
        float wL = 0.25;
        if (k < wL) w = k/wL;
        if (k > 1.0-wL) w = (1.0-k)/wL;

        samples[i] = 0;

        for (int j=0; j<freqs1.size(); j++) {
            float A = freqs1[j][1]*(1.0-k) + freqs2[j][1]*k;
            float f = freqs1[j][0]*(1.0-k) + freqs2[j][0]*k;
            //float a0 = ma0[int(freqs1[j][0])];
            float a = f*t;
            //float a = a0 + f*t; // TODO
            samples[i] += A*sin( a )*Ni1;
            //ma[int(freqs2[j][0])] = a;
        }
    }
    //t0 = t;

    float n1 = samples[1] - samples[0];
    float n2 = last_sample - last_sample2;
    cout << " first sample " << samples[0] << " last sample " << last_sample << " first normal " << n1 << " last normal " << n2 << endl;
    last_sample  = samples[buf_size-1];
    last_sample2 = samples[buf_size-2];

    for (auto& a0 : ma0) {
        int f = a0.first;
        a0.second = ma[f];
        //while (a0.second > 2*Pi) a0.second -= 2*Pi;
    }

    //for (auto& a0 : ma0) cout << " f " << a0.first << " a0 " << a0.second*180/Pi << endl;

    playBuffer(samples, buf_size, sample_rate);
    delete samples;
}

int VRSound::getQueuedBuffer() { return queuedBuffers; }

void VRSound::recycleBuffer() {
    ALint val = -1;
    ALuint bufid = 0; // TODO: not working properly!!
    do { ALCHECK_BREAK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &val) ); // recycle buffers
        for(; val > 0; --val) {
            ALCHECK( alSourceUnqueueBuffers(source, 1, &bufid));
            if ( queuedBuffers > 0 ) queuedBuffers -= 1;
        }
    } while (val > 0);
}





