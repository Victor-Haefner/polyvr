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

boost::mutex t_mutex;

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

    VRSound(bool loop) {
        //context = avformat_alloc_context();

        alGenBuffers(1u, &buffer);
        alGenSources(1u, &source);

        alSourcef(source, AL_PITCH, 1);
        alSourcef(source, AL_GAIN, 1);
        alSource3f(source, AL_POSITION, 0, 0, 0);
        alSource3f(source, AL_VELOCITY, 0, 0, 0);
        alSourcei(source, AL_LOOPING, (loop ? AL_TRUE : AL_FALSE));
    }

    ~VRSound() {
        alDeleteSources(1u, &source);
        alDeleteBuffers(1u, &buffer);
        avformat_close_input(&context);
    }
};

struct VRSoundJob {
    string path;
    bool looping;

    VRSoundJob(string filename, bool loop) {
        path = filename;
        looping = loop;
    }

    /*~VRSoundJob() {
        delete path;
    }*/
};

struct VRSoundContext {
    ALCdevice* device = 0;
    ALCcontext* context = 0;

    VRSoundContext() {
        av_register_all();

        device = alcOpenDevice(NULL);

        if (!device) {
            cout << "VRSoundContext() > alcOpenDevice failed!\n";
            return;
        }

        context = alcCreateContext(device, NULL);

        if (!alcMakeContextCurrent(context)) cout << "VRSoundContext() > alcMakeContextCurrent failed!\n";
    }

    ~VRSoundContext() {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }
};

VRSoundManager::VRSoundManager() {
    s_thread = new boost::thread(boost::bind(&VRSoundManager::soundThread, this));
}

void VRSoundManager::soundThread() {
    context = new VRSoundContext();

    while (t_running) {
        vector<VRSoundJob*> sounds;
        {
            boost::mutex::scoped_lock lock(t_mutex);
            sounds = soundJobs;
            soundJobs.clear();
        }

        for (auto job : sounds) {
            VRSound* sound = getSound(job);
            play(sound);
        }

        osgSleep(5);
    }

    clearSoundMap();

    if (bgMusic) delete bgMusic;

    if (context) delete context;
}

VRSoundManager& VRSoundManager::get() {
    static VRSoundManager* instance = new VRSoundManager();
    return *instance;
}

VRSoundManager::~VRSoundManager() {
    t_running = false;
    s_thread->join();
}

void VRSoundManager::clearSoundMap() {
    for (auto sound : sounds) if (sound.second) delete sound.second;

    sounds.clear();
}

void VRSoundManager::playSound(string filename, bool loop) {
    boost::mutex::scoped_lock lock(t_mutex);
    soundJobs.push_back(new VRSoundJob(filename, loop));
}

void VRSoundManager::playSound(string filename) {
    playSound(filename, false);
}

void VRSoundManager::play(VRSound* sound) {
    if (sound == 0) return;

    cout << "playSound " << sound->path << endl;

    // libav
    AVPacket packet;
    AVFrame* decodedFrame;
    decodedFrame = avcodec_alloc_frame();

    int len;

    while (1) {
        if ((sound->interrupt) || (av_read_frame(sound->context, &packet) < 0)) break; // End of stream. Done decoding.

        if (packet.stream_index != sound->stream_id) continue; // Skip non audio packets

        while (packet.size > 0) { // Decodes audio data from `packet` into the frame
            if (sound->interrupt) break;

            int finishedFrame = 0;
            len = avcodec_decode_audio4(sound->codec, decodedFrame, &finishedFrame, &packet);

            if (len < 0) {
                printf("error"); // Error in decoding
                break;
            }

            if (finishedFrame) {
                if (sound->interrupt) break;

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
                }
                else {
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
}

VRSound* VRSoundManager::getSound(VRSoundJob* soundJob) {
    cout << "getSound " << soundJob->path << endl;

    VRSound* sound = new VRSound(soundJob->looping);
    sound->path = soundJob->path;

    if (avformat_open_input(&sound->context, sound->path.c_str(), NULL, NULL) < 0) return 0;

    if (avformat_find_stream_info(sound->context, NULL) < 0) return 0;

    int stream_id = -1;

    for (uint i = 0; i < sound->context->nb_streams; i++) {
        if (sound->context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_id = i;
            break;
        }
    }

    if (stream_id == -1) return 0;

    sound->stream_id = stream_id;

    sound->codec = sound->context->streams[stream_id]->codec;
    AVCodec* codec = avcodec_find_decoder(sound->codec->codec_id);

    if (codec == 0) return 0;

    if (avcodec_open2(sound->codec, codec, NULL) < 0) return 0;

    sound->frequency = sound->codec->sample_rate;
    sound->format = AL_FORMAT_MONO16;
    AVSampleFormat sfmt = sound->codec->sample_fmt;

    if (sfmt == AV_SAMPLE_FMT_U8) sound->format = AL_FORMAT_MONO8;

    if (sfmt == AV_SAMPLE_FMT_S16) sound->format = AL_FORMAT_MONO16;

    if (sound->codec->channels == 2) {
        if (sfmt == AV_SAMPLE_FMT_U8) sound->format = AL_FORMAT_STEREO8;

        if (sfmt == AV_SAMPLE_FMT_S16) sound->format = AL_FORMAT_STEREO16;
    }

    sounds[sound->path] = sound;
    return sound;
}

void VRSoundManager::playMusic(string filename) { }

void VRSoundManager::stopMusic() { }

void VRSoundManager::setSoundVolume(float volume) {
    if (volume > 1.0f) volume = 1.0f;
    else
        if (volume < 0.0f) volume = 0.0f;

    for (auto iterator = sounds.begin(); iterator != sounds.end(); iterator++) {
        VRSound* sound = iterator->second;
        cout << "setVolume " << sound->path << endl;
        alSourcef(sound->source, AL_GAIN, volume);
    }
}

void VRSoundManager::setMusicVolume(float volume) {
    // This is just working with Ubuntu!
    if (volume > 1.0) volume = 1.0f;
    else
        if (volume < 0.0) volume = 0.0f;

    string percent = boost::lexical_cast<std::string>(static_cast<int>(volume * 100));
    string amixer = "amixer -D pulse sset Master " + percent + "% > /dev/null 2>&1";
    system(amixer.c_str());
}

/*void VRSoundManager::updatePlayerPosition(Vec3f position, Vec3f forward) { }*/

void VRSoundManager::stopSound(string path) {
    for (auto iterator : sounds) {
        VRSound* sound = iterator.second;

        if (sound->path.compare(path) == 0) {
            cout << "stopSound " << sound->path << endl;
            sound->interrupt = true;
        }
    }
}

void VRSoundManager::stopAllSounds(void) {
    for (auto iterator : sounds) {
        VRSound* sound = iterator.second;
        cout << "stopSound " << sound->path << endl;
        sound->interrupt = true;
    }
}

OSG_END_NAMESPACE;
