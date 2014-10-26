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

    VRSound() {
        //context = avformat_alloc_context();

        alGenBuffers(1u, &buffer);
        alGenSources(1u, &source);

        alSourcef(source, AL_PITCH, 1);
        alSourcef(source, AL_GAIN, 1);
        alSource3f(source, AL_POSITION, 0, 0, 0);
        alSource3f(source, AL_VELOCITY, 0, 0, 0);
        alSourcei(source, AL_LOOPING, AL_FALSE);
    }

    ~VRSound() {
        alDeleteSources(1u, &source);
        alDeleteBuffers(1u, &buffer);
        avformat_close_input(&context);
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

VRSoundManager::VRSoundManager() {
    s_thread = new boost::thread(boost::bind(&VRSoundManager::soundThread, this));
}

void VRSoundManager::soundThread() {
    context = new VRSoundContext();

    while(t_running) {
        vector<string> sounds;
        { boost::mutex::scoped_lock lock(t_mutex);
            sounds = sound_jobs;
            sound_jobs.clear();
        }

        for (auto path : sounds) {
            VRSound* sound = getSound(path);
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

void VRSoundManager::clearSoundMap() { for (auto s : sounds) if (s.second) delete s.second; }

void VRSoundManager::playSound(string filename) {
    filename = VRSceneManager::get()->getActiveScene()->getWorkdir() + '/' + filename;
    //cout << "playSound " << filename << endl;
    boost::mutex::scoped_lock lock(t_mutex);
    sound_jobs.push_back(filename);
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
        if (av_read_frame(sound->context, &packet) < 0) break;  // End of stream. Done decoding.
        if (packet.stream_index != sound->stream_id) continue; // Skip non audio packets

        while( packet.size > 0) { // Decodes audio data from `packet` into the frame
            int finishedFrame = 0;
            len = avcodec_decode_audio4(sound->codec, decodedFrame, &finishedFrame, &packet);
            if (len < 0) { printf("error"); break; } // Error in decoding

            if (finishedFrame) {
                // Decoded data is now available in decodedFrame->data[0]
                int data_size = av_samples_get_buffer_size(NULL, sound->codec->channels, decodedFrame->nb_samples, sound->codec->sample_fmt, 1);

                // OpenAL consumes buffers in the background
                // we first need to initialize the OpenAL buffers then
                // start continous playback.
                if (sound->init) {
                    alBufferData(sound->buffer, sound->format, decodedFrame->data[0], data_size, sound->frequency);
                    alSourceQueueBuffers(sound->source, 1, &sound->buffer); // all buffers queued
                    alSourcePlay(sound->source); // start playback
                    sound->init=0;
                } else {
                    ALuint buffer;
                    ALint val = -1;
                    while (val <= 0) alGetSourcei(sound->source, AL_BUFFERS_PROCESSED, &val); // wait for openal to release one buffer

                    // fill and requeue the empty buffer
                    alSourceUnqueueBuffers(sound->source, 1, &buffer);
                    alBufferData(buffer, sound->format, decodedFrame->data[0], data_size, sound->frequency);
                    alSourceQueueBuffers(sound->source, 1, &buffer);

                    //Restart openal playback if needed
                    alGetSourcei(sound->source, AL_SOURCE_STATE, &val);
                    if(val != AL_PLAYING) alSourcePlay(sound->source);
                }
            }

            //There may be more than one frame of audio data inside the packet.
            packet.size-=len;
            packet.data+=len;
        } // while packet.size > 0
    } // while more packets exist inside container.

    if (packet.data) av_free_packet(&packet);
}

VRSound* VRSoundManager::getSound(string filename) {
    //if (sounds.count(filename)) return sounds[filename];

    //filename = "/home/victor/Projects/polyvr/examples/test.wav";
    //filename = "/home/victor/Projects/polyvr/examples/test.mp3";
    cout << "getSound " << filename << endl;

    VRSound* sound = new VRSound();
    sound->path = filename;


    if ( avformat_open_input(&sound->context, filename.c_str(), NULL, NULL) < 0 ) return 0;
    if ( avformat_find_stream_info(sound->context, NULL) < 0 ) return 0;
    //av_dump_format(sound->context, 0, filename.c_str(), false);

    int stream_id = -1;
    for (uint i=0; i < sound->context->nb_streams; i++) {
        if (sound->context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
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

    sounds[filename] = sound;
    return sound;
}

void VRSoundManager::playMusic(string filename) {
    /*stopMusic();

    _system->createStream(filename.c_str(), FMOD_HARDWARE | FMOD_LOOP_NORMAL | FMOD_2D, 0, &_backgroundMusic);
    _backgroundMusic->setMode(FMOD_LOOP_NORMAL);
    _system->playSound(FMOD_CHANNEL_FREE, _backgroundMusic, false, &_channel1);*/
}

void VRSoundManager::stopMusic() {
    /*bool playing;

    _channel1->isPlaying(&playing);
    if (playing) {
        FMOD::Sound* currentMusic;
        _channel1->getCurrentSound(&currentMusic);
        _channel1->stop();
        if (currentMusic) currentMusic->release();
    }*/
}

void VRSoundManager::setSoundVolume(float volume) { /*_channel2->setVolume(volume);*/ }
void VRSoundManager::setMusicVolume(float volume) { /*_channel1->setVolume(volume);*/ }

/*void VRSoundManager::updatePlayerPosition(Vec3f position, Vec3f forward) {
    FMOD_VECTOR p = { position[0], position[1], position[2] };
    FMOD_VECTOR f = { forward[0], forward[1], forward[2] };
    FMOD_VECTOR up = { 0.0f, -1.0f, 0.0f };
    ERRCHECK(_system->set3DListenerAttributes(0, &p, 0, &f, &up));
    ERRCHECK(_system->update());
}*/

void VRSoundManager::stopAllSounds(void) {
    /*bool playing;

    _channel2->isPlaying(&playing);
    if (playing) {
        FMOD::Sound* currentSound;
        _channel2->getCurrentSound(&currentSound);
        _channel2->stop();
        _system->update();
    }
    clearSoundMap();*/
}

OSG_END_NAMESPACE;
