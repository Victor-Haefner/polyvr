#ifndef VRSOUNDMANAGER_H_INCLUDED
#define VRSOUNDMANAGER_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <map>

#include "VRSoundFwd.h"

namespace boost { class thread; }

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VRSoundChannel;

class VRSoundManager {
public:
    map<string, VRSoundPtr> sounds;
    VRSoundChannel* channel = 0;

    VRSoundManager();
    VRSoundPtr getSound(string path);
    void clearSoundMap(void);

public:
    static VRSoundManagerPtr get();
    ~VRSoundManager();

    VRSoundPtr setupSound(string path, bool loop = false);
    void playPositionalSound(string path, Vec3d vec);

    void stopSound(string path);
    void stopAllSounds(void);

    void setVolume(float volume);
    void updatePlayerPosition(Vec3d position, Vec3d forward);
};

OSG_END_NAMESPACE;

#endif // VRSOUNDMANAGER_H_INCLUDED
