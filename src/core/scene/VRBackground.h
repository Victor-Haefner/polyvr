#ifndef OSGSKYROOM_H_INCLUDED
#define OSGSKYROOM_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGColor.h>
#include "core/utils/VRStorage.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Background; OSG_GEN_CONTAINERPTR(Background);

class VRBackgroundBase;

class VRBackground : public VRStorage {
    public:
        enum TYPE { SOLID, IMAGE, SKYBOX, SKY };

    private:
        VRBackgroundBasePtr base;

    protected:
        void update();

    public:
        VRBackground();
        ~VRBackground();

        void setBackground(TYPE t);
        BackgroundRecPtr getBackground();
        TYPE getBackgroundType();
        VRSkyPtr getSky();

        void setBackgroundColor(Color3f c);
        void setBackgroundPath(string s);
        void setShowSplash(bool b);
        void setSplashPath(string p);
        Color3f getBackgroundColor();
        string getBackgroundPath();
        bool getShowSplash();
        string getSplashPath();

        void setSkyBGExtension(string f);
        string getSkyBGExtension();

        void setSplash(bool b);
        void updateBackground();
};

OSG_END_NAMESPACE;

#endif // OSGSKYROOM_H_INCLUDED
