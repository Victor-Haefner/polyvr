#ifndef VRSOUNDMANAGER_H_INCLUDED
#define VRSOUNDMANAGER_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <map>
#include <vector>

#include "VRSoundFwd.h"
#include "core/utils/VRFunctionFwd.h"

struct ALCdevice_struct;
struct ALCcontext_struct;

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VRSoundChannel;

struct VRSoundContext {
    ALCdevice_struct* device = 0;
    ALCcontext_struct* context = 0;

    VRSoundContext();
    ~VRSoundContext();
    static VRSoundContextPtr create();

    void makeCurrent();
    void enumerateDevices();
};

class VRSoundManager {
public:
    map<string, VRSoundPtr> sounds;
    VRSoundChannel* channel = 0;
    VRUpdateCbPtr soundQueue = 0;

    VRSoundManager();
    VRSoundPtr getSound(string path);
    void clearSoundMap(void);

public:
    static VRSoundManagerPtr get();
    ~VRSoundManager();

    VRSoundPtr setupSound(string path, bool loop = false, bool play = false);
    void playPositionalSound(string path, Vec3d vec);
    void queueSounds(vector<VRSoundPtr> sounds);

    void stopSound(string path);
    void stopAllSounds(void);

    void setVolume(float volume);
    void updatePlayerPosition(Vec3d position, Vec3d forward);
};

OSG_END_NAMESPACE;

#endif // VRSOUNDMANAGER_H_INCLUDED
