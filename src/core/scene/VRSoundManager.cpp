
extern "C" {
#include <libavresample/avresample.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}


#include "VRSoundManager.h"
#include "VRSceneManager.h"
#include "VRScene.h"

#if _WIN32
#include <al.h>
#include <alc.h>
#include <alext.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif

#include <boost/thread/thread.hpp>
#include <boost/thread.hpp>

#define ALCHECK(x) { \
x; \
ALenum error = alGetError(); \
if (error != AL_NO_ERROR) { \
        fprintf(stderr, "\nRuntime error: %s got %s at %s:%d", #x, toString(error).c_str(), __FILE__, __LINE__); \
        /*exit(0);*/ \
} }

OSG_BEGIN_NAMESPACE;
using namespace std;

string toString(ALenum error) {
    if(error == AL_INVALID_NAME) return "Invalid name";
    if(error == AL_INVALID_ENUM) return "Invalid enum ";
    if(error == AL_INVALID_VALUE) return "Invalid value ";
    if(error == AL_INVALID_OPERATION) return "Invalid operation ";
    if(error == AL_OUT_OF_MEMORY) return "Out of memory";
    return "Unknown error " + toString(error);
}

struct VRSound {
    uint source = 0;
    uint* buffers = 0;
    list<uint> free_buffers;
    uint frequency = 0;
    uint Nbuffers = 50;
    int stream_id = 0;
    int init = 0;
    string path;
    ALenum sample = 0;
    ALenum format = 0;
    ALenum layout = 0;
    ALenum state = AL_INITIAL;
    AVFormatContext* context = 0;
    AVAudioResampleContext* resampler = 0;
    AVCodecContext* codec = NULL;
    AVPacket packet;
    AVFrame* frame;
    bool interrupt = false;
    bool initiated = false;
    bool doUpdate = false;

    bool loop = false;
    float pitch = 1;
    float gain = 1;
    Vec3f pos, vel;

    VRSound() {
        buffers = new uint[Nbuffers];
    }

    ~VRSound() {
        close();
        delete[] buffers;
    }

    void setLoop(bool loop) { this->loop = loop; doUpdate = true; }
    void setPitch(float pitch) { this->pitch = pitch; doUpdate = true; }
    void setGain(float gain) { this->gain = gain; doUpdate = true; }
    void setUser(Vec3f p, Vec3f v) { pos = p; vel = v; doUpdate = true; }

    void close() {
        ALCHECK( alDeleteSources(1u, &source));
        ALCHECK( alDeleteBuffers(Nbuffers, buffers));
        if(context) avformat_close_input(&context);
        if(resampler) avresample_free(&resampler);
        context = 0;
        resampler = 0;
        init = 0;
    }

    void updateSource() {
        ALCHECK( alSourcef(source, AL_PITCH, pitch));
        ALCHECK( alSourcef(source, AL_GAIN, gain));
        ALCHECK( alSource3f(source, AL_POSITION, pos[0], pos[1], pos[2]));
        ALCHECK( alSource3f(source, AL_VELOCITY, vel[0], vel[1], vel[2]));
        doUpdate = false;
    }

    bool initiate() {
        initiated = true;

        ALCHECK( alGenBuffers(Nbuffers, buffers) );
        for (uint i=0; i<Nbuffers; i++) free_buffers.push_back(buffers[i]);

        ALCHECK( alGenSources(1u, &source) );
        updateSource();

        if (avformat_open_input(&context, path.c_str(), NULL, NULL) < 0) return 0;
        if (avformat_find_stream_info(context, NULL) < 0) return 0;
        av_dump_format(context, 0, path.c_str(), 0);

        stream_id = av_find_best_stream(context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if (stream_id == -1) return 0;

        codec = context->streams[stream_id]->codec;
        AVCodec* avcodec = avcodec_find_decoder(codec->codec_id);
        if (avcodec == 0) return 0;
        if (avcodec_open2(codec, avcodec, NULL) < 0) return 0;

        if (codec->channel_layout == 0) {
            if (codec->channels == 1) codec->channel_layout = AV_CH_LAYOUT_MONO;
            if (codec->channels == 2) codec->channel_layout = AV_CH_LAYOUT_STEREO;
            if (codec->channel_layout == 0) cout << "WARNING! channel_layout is 0.\n";
        }

        frequency = codec->sample_rate;
        format = AL_FORMAT_MONO16;
        AVSampleFormat sfmt = codec->sample_fmt;

        if (sfmt == AV_SAMPLE_FMT_NONE) cout << "unsupported format: none\n";

        if (sfmt == AV_SAMPLE_FMT_U8) sample = AL_UNSIGNED_BYTE_SOFT;
        if (sfmt == AV_SAMPLE_FMT_S16) sample = AL_SHORT_SOFT;
        if (sfmt == AV_SAMPLE_FMT_S32) sample = AL_INT_SOFT;
        if (sfmt == AV_SAMPLE_FMT_FLT) sample = AL_FLOAT_SOFT;
        if (sfmt == AV_SAMPLE_FMT_DBL) sample = AL_DOUBLE_SOFT;

        if (sfmt == AV_SAMPLE_FMT_U8P) sample = AL_UNSIGNED_BYTE_SOFT;
        if (sfmt == AV_SAMPLE_FMT_S16P) sample = AL_SHORT_SOFT;
        if (sfmt == AV_SAMPLE_FMT_S32P) sample = AL_INT_SOFT;
        if (sfmt == AV_SAMPLE_FMT_FLTP) sample = AL_FLOAT_SOFT;
        if (sfmt == AV_SAMPLE_FMT_DBLP) sample = AL_DOUBLE_SOFT;

        if (codec->channel_layout == AV_CH_LAYOUT_MONO) layout = AL_MONO_SOFT;
        if (codec->channel_layout == AV_CH_LAYOUT_STEREO) layout = AL_STEREO_SOFT;
        if (codec->channel_layout == AV_CH_LAYOUT_QUAD) layout = AL_QUAD_SOFT;
        if (codec->channel_layout == AV_CH_LAYOUT_5POINT1) layout = AL_5POINT1_SOFT;
        if (codec->channel_layout == AV_CH_LAYOUT_7POINT1) layout = AL_7POINT1_SOFT;

        switch(sample) {
            case AL_UNSIGNED_BYTE_SOFT:
                switch(layout) {
                    case AL_MONO_SOFT:    format = AL_FORMAT_MONO8; break;
                    case AL_STEREO_SOFT:  format = AL_FORMAT_STEREO8; break;
                    case AL_QUAD_SOFT:    format = alGetEnumValue("AL_FORMAT_QUAD8"); break;
                    case AL_5POINT1_SOFT: format = alGetEnumValue("AL_FORMAT_51CHN8"); break;
                    case AL_7POINT1_SOFT: format = alGetEnumValue("AL_FORMAT_71CHN8"); break;
                    default: cout << "OpenAL unsupported format 8\n"; break;
                } break;
            case AL_SHORT_SOFT:
                switch(layout) {
                    case AL_MONO_SOFT:    format = AL_FORMAT_MONO16; break;
                    case AL_STEREO_SOFT:  format = AL_FORMAT_STEREO16; break;
                    case AL_QUAD_SOFT:    format = alGetEnumValue("AL_FORMAT_QUAD16"); break;
                    case AL_5POINT1_SOFT: format = alGetEnumValue("AL_FORMAT_51CHN16"); break;
                    case AL_7POINT1_SOFT: format = alGetEnumValue("AL_FORMAT_71CHN16"); break;
                    default: cout << "OpenAL unsupported format 16\n"; break;
                } break;
            case AL_FLOAT_SOFT:
                switch(layout) {
                    case AL_MONO_SOFT:    format = alGetEnumValue("AL_FORMAT_MONO_FLOAT32"); break;
                    case AL_STEREO_SOFT:  format = alGetEnumValue("AL_FORMAT_STEREO_FLOAT32"); break;
                    case AL_QUAD_SOFT:    format = alGetEnumValue("AL_FORMAT_QUAD32"); break;
                    case AL_5POINT1_SOFT: format = alGetEnumValue("AL_FORMAT_51CHN32"); break;
                    case AL_7POINT1_SOFT: format = alGetEnumValue("AL_FORMAT_71CHN32"); break;
                    default: cout << "OpenAL unsupported format 32\n"; break;
                } break;
            case AL_DOUBLE_SOFT:
                switch(layout) {
                    case AL_MONO_SOFT:    format = alGetEnumValue("AL_FORMAT_MONO_DOUBLE"); break;
                    case AL_STEREO_SOFT:  format = alGetEnumValue("AL_FORMAT_STEREO_DOUBLE"); break;
                    default: cout << "OpenAL unsupported format 64\n"; break;
                } break;
            default: cout << "OpenAL unsupported format";
        }

        if (av_sample_fmt_is_planar(codec->sample_fmt)) {
            int out_sample_fmt;
            switch(codec->sample_fmt) {
                case AV_SAMPLE_FMT_U8P:  out_sample_fmt = AV_SAMPLE_FMT_U8; break;
                case AV_SAMPLE_FMT_S16P: out_sample_fmt = AV_SAMPLE_FMT_S16; break;
                case AV_SAMPLE_FMT_S32P: out_sample_fmt = AV_SAMPLE_FMT_S32; break;
                case AV_SAMPLE_FMT_DBLP: out_sample_fmt = AV_SAMPLE_FMT_DBL; break;
                case AV_SAMPLE_FMT_FLTP:
                default: out_sample_fmt = AV_SAMPLE_FMT_FLT;
            }

            resampler = avresample_alloc_context();
            av_opt_set_int(resampler, "in_channel_layout",  codec->channel_layout, 0);
            av_opt_set_int(resampler, "in_sample_fmt",      codec->sample_fmt,     0);
            av_opt_set_int(resampler, "in_sample_rate",     codec->sample_rate,    0);
            av_opt_set_int(resampler, "out_channel_layout", codec->channel_layout, 0);
            av_opt_set_int(resampler, "out_sample_fmt",     out_sample_fmt,        0);
            av_opt_set_int(resampler, "out_sample_rate",    codec->sample_rate,    0);
            avresample_open(resampler);
        }

        return true;
    }

    void playFrame() {
        if (state == AL_INITIAL) {
            if (!initiated) initiate();
            frame = avcodec_alloc_frame();
            av_seek_frame(context, stream_id, 0,  AVSEEK_FLAG_FRAME);
            state = AL_PLAYING;
        }

        int len;
        if (state == AL_PLAYING) {
            if (doUpdate) updateSource();
            if (interrupt || (av_read_frame(context, &packet) < 0)) {
                if (packet.data) av_free_packet(&packet);
                av_free(frame);
                state = loop ? AL_INITIAL : AL_STOPPED;
                return;
            } // End of stream. Done decoding.

            if (packet.stream_index != stream_id) { cout << "skip non audio\n"; return; } // Skip non audio packets

            while (packet.size > 0) { // Decodes audio data from `packet` into the frame
                if (interrupt) { cout << "interrupt sound\n"; break; }

                int finishedFrame = 0;
                len = avcodec_decode_audio4(codec, frame, &finishedFrame, &packet);
                if (len < 0) { cout << "decoding error\n"; break; }

                if (finishedFrame) {
                    if (interrupt) { cout << "interrupt sound\n"; break; }

                    // Decoded data is now available in frame->data[0]
                    int linesize;
                    int data_size = av_samples_get_buffer_size(&linesize, codec->channels, frame->nb_samples, codec->sample_fmt, 0);

                    ALbyte* frameData;
                    if (resampler != 0) {
                        frameData = (ALbyte *)av_malloc(data_size*sizeof(uint8_t));
                        avresample_convert( resampler, (uint8_t **)&frameData, linesize, frame->nb_samples, (uint8_t **)frame->data, frame->linesize[0], frame->nb_samples);
                    } else frameData = (ALbyte*)frame->data[0];

                    ALint val = -1;
                    ALuint bufid = 0;

                    while (free_buffers.size() == 0) { // recycle buffers
                        while (val <= 0) ALCHECK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &val));      // wait for openal to release one buffer
                        for(; val > 0; --val) {
                            ALCHECK( alSourceUnqueueBuffers(source, 1, &bufid));
                            free_buffers.push_back(bufid);
                        }
                    }

                    bufid = free_buffers.front();
                    free_buffers.pop_front();

                    ALCHECK( alBufferData(bufid, format, frameData, data_size, frequency));
                    ALCHECK( alSourceQueueBuffers(source, 1, &bufid));
                    ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
                    if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
                }

                //There may be more than one frame of audio data inside the packet.
                packet.size -= len;
                packet.data += len;
            } // while packet.size > 0
        } // while more packets exist inside container.
    }
};

struct VRSoundContext {
    ALCdevice* device = 0;
    ALCcontext* context = 0;

    VRSoundContext() {
        av_register_all();
        device = alcOpenDevice(NULL);
        if (!device) { cout << "VRSoundContext() > alcOpenDevice failed!\n"; return; }

        context = alcCreateContext(device, NULL);
        makeCurrent();

        alGetError();
        //enumerateDevices();
    }

    ~VRSoundContext() {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

    void makeCurrent() {
        if (!alcMakeContextCurrent(context)) cout << "VRSoundContext() > alcMakeContextCurrent failed!\n";
    }

    void enumerateDevices() {
        bool enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
        if (enumeration) {
            cout << "Enumerate OpenAL devices:\n";
            cout << string( alcGetString(NULL, ALC_DEVICE_SPECIFIER) ) << endl;
        }
    }
};

struct VRSoundChannel {
    bool running = true;
    boost::thread* thread;
    VRSoundContext* context;
    boost::mutex mutex;
    map<int, VRSound*> current;

    VRSoundChannel() {
        thread = new boost::thread(boost::bind(&VRSoundChannel::soundThread, this));
    }

    ~VRSoundChannel() {
        running = false;
        thread->join();
        delete thread;
    }

    void play(VRSound* sound) {
        boost::mutex::scoped_lock lock(mutex);
        current[current.size()] = sound;
    }

    void soundThread() {
        context = new VRSoundContext();

        while (running) {
            osgSleep(1);
            boost::mutex::scoped_lock lock(mutex);
            if (current.size() == 0) continue;

            for (auto c : current) {
                c.second->playFrame();
                if (c.second->state == AL_STOPPED) current.erase(c.first);
            }
        }

        if (context) delete context;
    }
};

VRSoundManager::VRSoundManager() {
    channel = new VRSoundChannel();
}

VRSoundManager::~VRSoundManager() { clearSoundMap(); }
VRSoundManager& VRSoundManager::get() {
    static VRSoundManager* instance = new VRSoundManager();
    return *instance;
}

void VRSoundManager::clearSoundMap() {
    for (auto sound : sounds) if (sound.second) delete sound.second;
    sounds.clear();
}

void VRSoundManager::playSound(string path, bool loop) {
    VRSound* sound = getSound(path);
    sound->setLoop(loop);
    channel->play(sound);
}

VRSound* VRSoundManager::getSound(string path) {
    VRSound* sound = new VRSound();
    sound->path = path;
    sounds[sound->path] = sound;
    return sound;
}

void VRSoundManager::setSoundVolume(float volume) {
    volume = max(volume, 0.f);
    volume = min(volume, 1.f);
    for (auto s : sounds) s.second->setGain(volume);
}

/*void VRSoundManager::updatePlayerPosition(Vec3f position, Vec3f forward) { }*/

void VRSoundManager::stopSound(string path) {
    for (auto s : sounds) {
        if (s.second->path != path) continue;
        s.second->interrupt = true;
        return;
    }
}

void VRSoundManager::stopAllSounds(void) {
    for (auto s : sounds) s.second->interrupt = true;
}

OSG_END_NAMESPACE;
