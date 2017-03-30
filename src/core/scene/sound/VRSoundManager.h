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

    void play(VRSoundPtr sound);

public:
    static VRSoundManager& get();
    ~VRSoundManager();

    void playSound(string path, bool loop = false);
    void playPositionalSound(string path, Vec3f vec);

    void stopSound(string path);
    void stopAllSounds(void);

    void setSoundVolume(float volume);
    void updatePlayerPosition(Vec3f position, Vec3f forward);
};

OSG_END_NAMESPACE;

#endif // VRSOUNDMANAGER_H_INCLUDED
