#include "VRSoundManager.h"
#include "VRSceneManager.h"
#include "VRScene.h"

extern "C" {
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <AL/al.h>
#include <AL/alc.h>

#include <boost/thread/thread.hpp>
#include <boost/thread.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct VRSound {
    uint source;
    uint buffer;
    uint frequency;
    int stream_id;
    int init = 1;
    string path;
    ALenum format;
    AVFormatContext* context = NULL;
    AVCodecContext* codec = NULL;
    bool interrupt = false;
    bool initiated = false;

    bool loop;
    float pitch, gain;
    Vec3f pos, vel;

    VRSound() {}

    ~VRSound() {
        alDeleteSources(1u, &source);
        alDeleteBuffers(1u, &buffer);
        avformat_close_input(&context);
    }

    void setLoop(bool loop) {
        this->loop = loop;
        alSourcei(source, AL_LOOPING, (loop ? AL_TRUE : AL_FALSE));
    }

    void setPitch(float pitch) {
        this->pitch = pitch;
        alSourcef(source, AL_PITCH, pitch);
    }

    void setGain(float gain) {
        this->gain = gain;
        alSourcef(source, AL_GAIN, gain);
    }

    void setUser(Vec3f p, Vec3f v) {
        pos = p;
        vel = v;
        alSource3f(source, AL_POSITION, p[0], p[1], p[2]);
        alSource3f(source, AL_VELOCITY, v[0], v[1], v[2]);
    }

    bool initiate() {
        if (initiated) return initiated;
        else initiated = true;
        alGenBuffers(1u, &buffer);
        alGenSources(1u, &source);
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

        if (sfmt == AV_SAMPLE_FMT_U8) format = AL_FORMAT_MONO8;
        if (sfmt == AV_SAMPLE_FMT_S16) format = AL_FORMAT_MONO16;

        if (codec->channels == 2) {
            if (sfmt == AV_SAMPLE_FMT_U8) format = AL_FORMAT_STEREO8;
            if (sfmt == AV_SAMPLE_FMT_S16) format = AL_FORMAT_STEREO16;
        }
        return true;
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
    }

    ~VRSoundContext() {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        alcCloseDevice(device);
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
            if (current->initiate()) stream(current);
            if (!current->loop) {
                boost::mutex::scoped_lock lock(mutex);
                current = 0;
            }
        }

        if (context) delete context;
    }

    void stream(VRSound* sound) {
        if (sound == 0) return;

        cout << "stream " << sound << " from " << this << endl;
        busy = true;

        // libav
        AVPacket packet;
        AVFrame* decodedFrame;
        decodedFrame = avcodec_alloc_frame();

        int len;

        while (1) {
            if ((sound->interrupt) || (av_read_frame(sound->context, &packet) < 0)) { cout << "end of stream\n"; break; } // End of stream. Done decoding.

            if (packet.stream_index != sound->stream_id) { cout << "skip non audio\n"; continue; } // Skip non audio packets

            while (packet.size > 0) { // Decodes audio data from `packet` into the frame
                if (sound->interrupt) { cout << "interrupt sound\n"; break; }

                int finishedFrame = 0;
                len = avcodec_decode_audio4(sound->codec, decodedFrame, &finishedFrame, &packet);

                if (len < 0) { cout << "decoding error\n"; break; }

                if (finishedFrame) {
                    if (sound->interrupt) { cout << "interrupt sound\n"; break; }

                    // Decoded data is now available in decodedFrame->data[0]
                    int data_size = av_samples_get_buffer_size(NULL, sound->codec->channels, decodedFrame->nb_samples, sound->codec->sample_fmt, 1);

                    // OpenAL consumes buffers in the background
                    // we first need to initialize the OpenAL buffers then
                    // start continous playback.
                    if (sound->init) {
                        alBufferData(sound->buffer, sound->format, decodedFrame->data[0], data_size, sound->frequency);
                        alSourceQueueBuffers(sound->source, 1, &sound->buffer);    // all buffers queued
                        alSourcePlay(sound->source);    // start playback
                        sound->init = 0;
                    } else {
                        ALuint buffer;
                        ALint val = -1;

                        while (val <= 0) alGetSourcei(sound->source, AL_BUFFERS_PROCESSED, &val);      // wait for openal to release one buffer

                        // fill and requeue the empty buffer
                        alSourceUnqueueBuffers(sound->source, 1, &buffer);
                        alBufferData(buffer, sound->format, decodedFrame->data[0], data_size, sound->frequency);
                        alSourceQueueBuffers(sound->source, 1, &buffer);

                        //Restart openal playback if needed
                        alGetSourcei(sound->source, AL_SOURCE_STATE, &val);

                        if (val != AL_PLAYING) alSourcePlay(sound->source);
                    }
                }

                //There may be more than one frame of audio data inside the packet.
                packet.size -= len;
                packet.data += len;
            } // while packet.size > 0
        } // while more packets exist inside container.

        if (packet.data) av_free_packet(&packet);
        sound->init = 1;
        cout << "stream done " << sound->path << endl;
        busy = false;
    }
};

VRSoundManager::VRSoundManager() {}
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
