#include "VRSoundManager.h"
#include "VRSceneManager.h"
#include "VRScene.h"

extern "C" {
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavresample/avresample.h>
}

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

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
    uint frequency = 0;
    uint Nbuffers = 5;
    int stream_id = 0;
    int init = 0;
    string path;
    ALenum sample = 0;
    ALenum format = 0;
    ALenum layout = 0;
    AVFormatContext* context = 0;
    AVAudioResampleContext* resampler = 0;
    AVCodecContext* codec = NULL;
    bool interrupt = false;
    bool initiated = false;

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

    void setLoop(bool loop) { this->loop = loop; }
    void setPitch(float pitch) { this->pitch = pitch; }
    void setGain(float gain) { this->gain = gain; }
    void setUser(Vec3f p, Vec3f v) { pos = p; vel = v; }

    void close() {
        ALCHECK( alDeleteSources(1u, &source));
        ALCHECK( alDeleteBuffers(Nbuffers, buffers));
        if(context) avformat_close_input(&context);
        if(resampler) avresample_free(&resampler);
        context = 0;
        resampler = 0;
        init = 0;
    }

    bool initiate() {
        if (initiated) close();
        initiated = true;

        ALCHECK( alGenBuffers(Nbuffers, buffers) );
        ALCHECK( alGenSources(1u, &source) );
        ALCHECK( alSourcef(source, AL_PITCH, pitch));
        ALCHECK( alSourcef(source, AL_GAIN, gain));
        ALCHECK( alSource3f(source, AL_POSITION, pos[0], pos[1], pos[2]));
        ALCHECK( alSource3f(source, AL_VELOCITY, vel[0], vel[1], vel[2]));
        //ALCHECK( alSourcei(source, AL_LOOPING, (loop ? AL_TRUE : AL_FALSE)) );

        if (avformat_open_input(&context, path.c_str(), NULL, NULL) < 0) return 0;
        if (avformat_find_stream_info(context, NULL) < 0) return 0;

        int stream_id = -1;
        for (uint i = 0; i < context->nb_streams; i++) {
            if (context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                stream_id = i;
                break;
            }
        }
        if (stream_id == -1) return 0;

        this->stream_id = stream_id;
        codec = context->streams[stream_id]->codec;
        AVCodec* avcodec = avcodec_find_decoder(codec->codec_id);
        if (avcodec == 0) return 0;
        if (avcodec_open2(codec, avcodec, NULL) < 0) return 0;

        if (codec->channel_layout == 0) codec->channel_layout = AV_CH_LAYOUT_MONO;
        //codec->sample_fmt = AV_SAMPLE_FMT_FLT; // override

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

        cout << "init audio stream" << endl;
        cout << " AL format=" << format << endl;
        cout << " sample_fmt=" << codec->sample_fmt << endl;
        cout << " channel_layout=" << codec->channel_layout << endl;
        cout << " sample_rate=" << codec->sample_rate << endl;
        cout << " bit_rate=" << codec->bit_rate << endl;

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
            cout << "converting sample format to " << out_sample_fmt << endl;
        }

        return true;
    }

    void play() {
        AVPacket packet; // libav
        AVFrame* frame = avcodec_alloc_frame();

        int len;
        while (1) {
            if (interrupt || (av_read_frame(context, &packet) < 0)) { cout << "end of stream\n"; break; } // End of stream. Done decoding.
            if (packet.stream_index != stream_id) { cout << "skip non audio\n"; continue; } // Skip non audio packets

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

                    if (init < Nbuffers) { // initialize OpenAL buffers
                        ALint val = -1;
                        ALCHECK( alBufferData(buffers[init], format, frame->data[0], data_size, frequency));
                        ALCHECK( alSourceQueueBuffers(source, 1, &buffers[init]));    // all buffers queued
                        ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
                        if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
                        init++;
                    } else { // start continous playback.
                        ALint val = -1;
                        ALuint bufid = 0; // get empty buffer

                        while (val <= 0) ALCHECK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &val));      // wait for openal to release one buffer
                        for(; val > 0; --val) ALCHECK( alSourceUnqueueBuffers(source, 1, &bufid));

                        // fill and requeue the empty buffer
                        ALCHECK( alBufferData(bufid, format, frame->data[0], data_size, frequency));
                        ALCHECK( alSourceQueueBuffers(source, 1, &bufid));

                        //Restart openal playback if needed
                        ALCHECK( alGetSourcei(source, AL_SOURCE_STATE, &val));
                        if (val != AL_PLAYING) ALCHECK( alSourcePlay(source));
                    }
                }

                //There may be more than one frame of audio data inside the packet.
                packet.size -= len;
                packet.data += len;
            } // while packet.size > 0
        } // while more packets exist inside container.

        if (packet.data) av_free_packet(&packet);
        init = 0;
        cout << "stream done " << path << endl;
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
        if (!alcMakeContextCurrent(context)) cout << "VRSoundContext() > alcMakeContextCurrent failed!\n";

        alGetError();
        //enumerateDevices();
    }

    ~VRSoundContext() {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        alcCloseDevice(device);
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
    bool busy = false;
    boost::thread* thread;
    VRSoundContext* context;
    boost::mutex mutex;
    VRSound* current = 0;

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
        current = sound;
    }

    void soundThread() {
        context = new VRSoundContext();

        while (running) {
            osgSleep(1);

            {
                boost::mutex::scoped_lock lock(mutex);
                if (current == 0) continue;
            }

            cout << "PLAY " << current->loop << endl;
            busy = true;
            if (current->initiate()) current->play();
            if (!current->loop) {
                boost::mutex::scoped_lock lock(mutex);
                current = 0;
            }
            busy = false;
        }

        if (context) delete context;
    }
};

VRSoundManager::VRSoundManager() {
    ;
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

    VRSoundChannel* channel = 0;
    for (auto c : channels) if (!c->busy) { channel = c; break; }
    if (channel == 0) {
        channel = new VRSoundChannel();
        channels.push_back(channel);
    }
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

    for (auto s : sounds) alSourcef(s.second->source, AL_GAIN, volume);

    // This is just working with Ubuntu!
    //string percent = boost::lexical_cast<std::string>(static_cast<int>(volume * 100));
    //string amixer = "amixer -D pulse sset Master " + percent + "% > /dev/null 2>&1";
    //system(amixer.c_str());
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
