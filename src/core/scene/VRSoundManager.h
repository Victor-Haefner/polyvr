#ifndef VRSOUNDMANAGER_H_INCLUDED
#define VRSOUNDMANAGER_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <map>

class AVFormatContext;
class ao_device;

using namespace std;

OSG_BEGIN_NAMESPACE;

class VRSoundManager {
    private:
        ao_device* adevice = 0;
        AVFormatContext* bgMusic = 0;
        map<string, AVFormatContext*> sounds;

        VRSoundManager();
        VRSoundManager(const VRSoundManager& ref);

        /**
         * Loads the sound from disk or returns it from the internal cache
         * if it already resides in it.
         *
         * @param filename The file to be loaded; must be
         * @return May return nullptr if sound couldn't be loaded from file or cache
         */
        AVFormatContext* getSound(const string &filename);

        /**
         * Clears the sound map and frees used memory
         */
        void clearSoundMap(void);
    public:
        static VRSoundManager& get();
        ~VRSoundManager();

        /**
         * Play the music with the given filename.
         *
         * @param filename The name of the music file
         * @see initialize()
         */
        void playMusic(const string &filename);

        /**
         * Stops the currently playing music.
         */
        void stopMusic(void);

        /**
         * Play the sound with the given filename in 2D mode.
         *
         * @param filename The name of the sound file
         * @see initialize()
         */
        void playSound(const string &filename);

        /**
         * Play the sound with the given filename in 3D mode.
         *
         * @param filename The name of the sound file
         * @param position The position of the sound
         * @see playSound()
         * @see initialize()
         */
        void playPositionalSound(const string &filename, Vec3f vec);

        /**
         * Stops all currently playing sounds. Note that this will also
         * clear the sound map to prevent sound artifacts from being
         * played.
         */
        void stopAllSounds(void);

        /**
         * Set music volume to the specified value ranging from 0.0 to 1.0 (= 100%)
         *
         * @param volume The value to which the volume will be set
         */
        void setMusicVolume(float volume);
        void setSoundVolume(float volume);


         /**
         * Updates the sound system, telling it where the person is located in space. This
         * will affect how 3D sounds are played back.
         *
         * @param position The position of the person
         * @param forward A vector pointing in the forward direction (measured by the person)
         * @see playPositionalSound()
         */
        void updatePlayerPosition(Vec3f position, Vec3f forward);
};

OSG_END_NAMESPACE;

#endif // VRSOUNDMANAGER_H_INCLUDED
