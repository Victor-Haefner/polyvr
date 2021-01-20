#ifndef RPMTOOL_H_INCLUDED
#define RPMTOOL_H_INCLUDED

#include <map>
#include <OpenSG/OSGConfig.h>
#include "core/scene/sound/VRSound.h"
#include "addons/Bullet/VRPhysicsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCarSound {
    private:
        OSG::VRSoundPtr sound;
        float currentRPM = 0;
        float minRPM = 0;
        float maxRPM = 0;
        float lastRPM = 0; // previous rpm
        bool lin = true;
        float fade = 0.01; // in s
        float duration =  0.1; // in s

        unsigned int nSamples; // number of spectra provided
        unsigned int resolution; // number of frequencies per spectrum, data type needs to fit ~50k
        double maxVal = 0.;

        bool active = false;
        bool init = false;

        map<float, vector<double> > data; // map: key is rpm, value is spectrum data
        vector<double> current; // current interpolated vector

        void interpLin(double& y, const float& x, const float& x0, const float& x1, const double& y0, const double& y1);
        void interpLag3(double& y, const float &x, const float &x0, const float &x1, const float &x2, const double &y0, const double &y1, const double &y2);
        void resetValues();

    public:
        VRCarSound();
        ~VRCarSound();
        static VRCarSoundPtr create();

        void loadSoundFile(string filename);
        void play(float rpm); // (ms) default audio clip duration, fade 1% either side of sample
        const float getRPM();
        void setSound(VRSoundPtr s);
        void setFade(float f);
        void setDuration(float d);
        VRSoundPtr getSound();
        void toggleSound(bool onOff);

        void print();
        const vector<double>& getSpectrum(const float rpm);
        void readFile(string filename);
        const unsigned int getRes();
        const bool isLoaded();
        const float getMinRPM();
        const float getMaxRPM();
        const double getMaxSample();
};

OSG_END_NAMESPACE;

#endif
