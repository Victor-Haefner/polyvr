#include "VRSoundManager.h"
#include "VRSceneManager.h"
#include "VRScene.h"

extern "C" {
#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <ao/ao.h>
}

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSoundManager::VRSoundManager() {
    av_register_all();

    //initialize AO lib
    ao_initialize();
}

VRSoundManager::~VRSoundManager() {
    clearSoundMap();
    if (bgMusic) avformat_close_input(&bgMusic);
    ao_shutdown();
}

VRSoundManager& VRSoundManager::get() {
    static VRSoundManager* instance = new VRSoundManager();
    return *instance;
}

void VRSoundManager::clearSoundMap() {
    for (auto s : sounds) avformat_close_input(&s.second);
}

void VRSoundManager::playSound(string filename) {
    filename = VRSceneManager::get()->getActiveScene()->getWorkdir() + '/' + filename;
    cout << "playSound " << filename << endl;
    AVFormatContext* sound = getSound(filename);
    if (sound == 0) return;
    cout << "A\n";

    int stream_id = -1;
    for (uint i=0; i < sound->nb_streams; i++){
        if (sound->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            stream_id = i;
            break;
        }
    }
    if (stream_id == -1) return;
    cout << "B\n";

    AVCodecContext* ctx = sound->streams[stream_id]->codec;
    AVCodec* codec = avcodec_find_decoder(ctx->codec_id);
    if (codec == 0) return;
    cout << "C\n";
    if (avcodec_open2(ctx, codec, NULL) < 0) return;
    cout << "D\n";

    int driver = ao_default_driver_id();
    ao_sample_format sformat;
    AVSampleFormat sfmt = ctx->sample_fmt;
    if (sfmt == AV_SAMPLE_FMT_U8) {
        printf("U8\n");
        sformat.bits=8;
    } else if(sfmt == AV_SAMPLE_FMT_S16) {
        printf("S16\n");
        sformat.bits=16;
    } else if(sfmt == AV_SAMPLE_FMT_S32) {
        printf("S32\n");
        sformat.bits=32;
    }
    sformat.channels = ctx->channels;
    sformat.rate = ctx->sample_rate;
    sformat.byte_format = AO_FMT_NATIVE;
    sformat.matrix = 0;
    ao_device *adevice = ao_open_live(driver,&sformat,NULL);



    AVPacket packet;
    av_init_packet(&packet);

    AVFrame* frame = avcodec_alloc_frame();
    int buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;
    uint8_t buffer[buffer_size];
    packet.data = buffer;
    packet.size = buffer_size;

    int frameFinished = 0;
    while (av_read_frame(sound,&packet) >= 0) {
        if (packet.stream_index == stream_id) {
            avcodec_decode_audio4(ctx, frame, &frameFinished, &packet);
            if (frameFinished) ao_play(adevice, (char*)frame->extended_data[0], frame->linesize[0] );
        }
    }
}

AVFormatContext* VRSoundManager::getSound(string filename) {
    if (sounds.count(filename)) return sounds[filename];

    AVFormatContext* sound = avformat_alloc_context();
    if ( avformat_open_input(&sound, filename.c_str(), NULL, NULL) < 0 ) return 0;
    if ( avformat_find_stream_info(sound, NULL) < 0 ) return 0;

    av_dump_format(sound,0,filename.c_str(),false);

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
