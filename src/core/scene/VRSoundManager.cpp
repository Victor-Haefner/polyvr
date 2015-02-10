#include "VRSoundManager.h"
#include "VRSceneManager.h"
#include "VRScene.h"

extern "C" {
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <boost/thread/thread.hpp>
#include <boost/thread.hpp>

#define ALCHECK(x) { \
x; \
ALenum error = alGetError(); \
if (error != AL_NO_ERROR) cout << "AL error: " << toString(error) << endl; \
}

OSG_BEGIN_NAMESPACE;
using namespace std;

string toString(ALenum error) {
    if(error == AL_INVALID_NAME) return "Invalid name";
    if(error == AL_INVALID_ENUM) return "Invalid enum ";
    if(error == AL_INVALID_VALUE) return "Invalid value ";
    if(error == AL_INVALID_OPERATION) return "Invalid operation ";
    if(error == AL_OUT_OF_MEMORY) return "Out of memory";
    return "Unknown error";
}

struct VRSound {
    uint source;
    uint* buffers;
    uint frequency;
    uint Nbuffers = 1; // TODO: raise this!
    int stream_id;
    int init = 1;
    string path;
    ALenum sample;
    ALenum format;
    ALenum layout;
    AVFormatContext* context = NULL;
    AVCodecContext* codec = NULL;
    bool interrupt = false;
    bool initiated = false;

    bool loop;
    float pitch, gain;
    Vec3f pos, vel;

    VRSound() {
        buffers = new uint[Nbuffers];
    }

    ~VRSound() {
        close();
        delete buffers;
    }

    void setLoop(bool loop) {
        this->loop = loop;
        ALCHECK( alSourcei(source, AL_LOOPING, (loop ? AL_TRUE : AL_FALSE)) );
    }

    void setPitch(float pitch) {
        this->pitch = pitch;
        ALCHECK( alSourcef(source, AL_PITCH, pitch));
    }

    void setGain(float gain) {
        this->gain = gain;
        ALCHECK( alSourcef(source, AL_GAIN, gain));
    }

    void setUser(Vec3f p, Vec3f v) {
        pos = p;
        vel = v;
        ALCHECK( alSource3f(source, AL_POSITION, p[0], p[1], p[2]));
        ALCHECK( alSource3f(source, AL_VELOCITY, v[0], v[1], v[2]));
    }

    void close() {
        ALCHECK( alDeleteSources(1u, &source));
        ALCHECK( alDeleteBuffers(Nbuffers, buffers));
        avformat_close_input(&context);
    }

    bool initiate() {
        if (initiated) close();
        initiated = true;

        ALCHECK( alGenBuffers(Nbuffers, buffers) );
        ALCHECK( alGenSources(1u, &source) );
        setPitch(1);
        setGain(1);
        setUser(Vec3f(), Vec3f());

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

        if (codec->channels == 1) layout = AL_MONO_SOFT;
        if (codec->channels == 2) layout = AL_STEREO_SOFT;
        if (codec->channels == 4) layout = AL_QUAD_SOFT;
        if (codec->channels == 6) layout = AL_5POINT1_SOFT;
        if (codec->channels == 8) layout = AL_7POINT1_SOFT; // TODO: check the channel count, AV_CH_LAYOUT_MONO

        switch(sample) {
            case AL_UNSIGNED_BYTE_SOFT:
                switch(layout) {
                    case AL_MONO_SOFT:    return AL_FORMAT_MONO8;
                    case AL_STEREO_SOFT:  return AL_FORMAT_STEREO8;
                    case AL_QUAD_SOFT:    return alGetEnumValue("AL_FORMAT_QUAD8");
                    case AL_5POINT1_SOFT: return alGetEnumValue("AL_FORMAT_51CHN8");
                    case AL_7POINT1_SOFT: return alGetEnumValue("AL_FORMAT_71CHN8");
                    default: cout << "OpenAL unsupported format";
                } break;
            case AL_SHORT_SOFT:
                switch(layout) {
                    case AL_MONO_SOFT:    return AL_FORMAT_MONO16;
                    case AL_STEREO_SOFT:  return AL_FORMAT_STEREO16;
                    case AL_QUAD_SOFT:    return alGetEnumValue("AL_FORMAT_QUAD16");
                    case AL_5POINT1_SOFT: return alGetEnumValue("AL_FORMAT_51CHN16");
                    case AL_7POINT1_SOFT: return alGetEnumValue("AL_FORMAT_71CHN16");
                    default: cout << "OpenAL unsupported format";
                } break;
            case AL_FLOAT_SOFT:
                switch(layout) {
                    case AL_MONO_SOFT:    return alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
                    case AL_STEREO_SOFT:  return alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
                    case AL_QUAD_SOFT:    return alGetEnumValue("AL_FORMAT_QUAD32");
                    case AL_5POINT1_SOFT: return alGetEnumValue("AL_FORMAT_51CHN32");
                    case AL_7POINT1_SOFT: return alGetEnumValue("AL_FORMAT_71CHN32");
                    default: cout << "OpenAL unsupported format";
                } break;
            case AL_DOUBLE_SOFT:
                switch(layout) {
                    case AL_MONO_SOFT:    return alGetEnumValue("AL_FORMAT_MONO_DOUBLE");
                    case AL_STEREO_SOFT:  return alGetEnumValue("AL_FORMAT_STEREO_DOUBLE");
                    default: cout << "OpenAL unsupported format";
                } break;
            default: cout << "OpenAL unsupported format";
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

                    if (init) { // initialize OpenAL buffers
                        ALCHECK( alBufferData(buffers[0], format, frame->data[0], data_size, frequency));
                        ALCHECK( alSourceQueueBuffers(source, Nbuffers, buffers));    // all buffers queued
                        ALCHECK( alSourcePlay(source));    // start playback
                        init = 0;
                    } else { // start continous playback.
                        //ALuint buffer;
                        ALint val = -1;
                        while (val <= 0) ALCHECK( alGetSourcei(source, AL_BUFFERS_PROCESSED, &val));      // wait for openal to release one buffer

                        ALuint bufid; // get empty buffer
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
        init = 1;
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
