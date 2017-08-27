
extern "C" {
#include <libavresample/avresample.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}


#include "VRSoundManager.h"
#include "VRSound.h"
#include "../VRSceneManager.h"
#include "../VRScene.h"

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
//#include <boost/thread.hpp>

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
    boost::thread* thread = 0;
    VRSoundContext* context = 0;
    boost::mutex mutex;
    map<int, VRSoundPtr> current;

    VRSoundChannel() {
        thread = new boost::thread(boost::bind(&VRSoundChannel::soundThread, this));
    }

    ~VRSoundChannel() {
        running = false;
        thread->join();
        delete thread;
    }

    void add(VRSoundPtr sound) {
        boost::mutex::scoped_lock lock(mutex);
        current[current.size()] = sound;
    }

    void soundThread() {
        context = new VRSoundContext();

        while (running) {
            osgSleep(1);
            boost::mutex::scoped_lock lock(mutex);
            if (current.size() == 0) continue;

            vector<int> toErase;
            for (auto c : current) {
                c.second->playFrame();
                /*if (c.second->getState() == AL_STOPPED) {
                    cout << "soundThread " << current.size() << endl;
                    toErase.push_back( c.first );
                }*/
            }

            for (auto e : toErase) current.erase(e);
        }

        if (context) delete context;
    }
};

VRSoundManager::VRSoundManager() {
    cout << "Init VRSoundManager..";
    //channel = new VRSoundChannel(); // TODO: catch internal abort when problems with pulseaudio
    cout << " done" << endl;
}

VRSoundManager::~VRSoundManager() {
    clearSoundMap();
    if (channel) delete channel;
}

VRSoundManagerPtr VRSoundManager::get() {
    static VRSoundManagerPtr instance;
    if (instance && !instance->channel) instance->channel = new VRSoundChannel(); // delay init of channel
    if (!instance) instance = VRSoundManagerPtr( new VRSoundManager() );
    return instance;
}

void VRSoundManager::clearSoundMap() {
    sounds.clear();
}

VRSoundPtr VRSoundManager::setupSound(string path, bool loop, bool play) {
    cout << "VRSoundManager::setupSound " << path << " " << loop << endl;
    if (!channel) channel = new VRSoundChannel();
    auto sound = getSound(path);
    if (sound->isRunning()) { cout << "sound is playing, ignoring" << endl; return sound; }

    cout << " VRSoundManager::setupSound reset " << endl;
    sound->setLoop(loop);
    if (play) sound->play();
    channel->add(sound);
    return sound;
}

VRSoundPtr VRSoundManager::getSound(string path) {
    if (sounds.count(path) == 0) { // TODO: WORKAROUND
        sounds[path] = VRSound::create();
        sounds[path]->setPath( path );
    } return sounds[path];
}

void VRSoundManager::setVolume(float volume) {
    volume = max(volume, 0.f);
    volume = min(volume, 1.f);
    for (auto s : sounds) s.second->setGain(volume);
}

/*void VRSoundManager::updatePlayerPosition(Vec3d position, Vec3d forward) { }*/

void VRSoundManager::stopSound(string path) {
    if (sounds.count(path) == 0) return;
    sounds[path]->stop();
}

void VRSoundManager::stopAllSounds(void) {
    for (auto s : sounds) s.second->stop();
}


OSG_END_NAMESPACE;
