#ifndef OSGSKYROOM_H_INCLUDED
#define OSGSKYROOM_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGColor.h>
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Background; OSG_GEN_CONTAINERPTR(Background);

class VRBackgroundBase;

class VRBackground : public VRStorage {
    public:
        enum TYPE { SOLID, IMAGE, SKYBOX, SKY };

    private:
        VRBackgroundBase* base = 0;

    protected:
        void update();

    public:
        VRBackground();
        ~VRBackground();

        void setBackground(TYPE t);
        BackgroundRecPtr getBackground();
        TYPE getBackgroundType();

        void setBackgroundColor(Color3f c);
        void setBackgroundPath(string s);
        Color3f getBackgroundColor();
        string getBackgroundPath();

        void setSkyBGExtension(string f);
        string getSkyBGExtension();

        void updateBackground();
};

OSG_END_NAMESPACE;

#endif // OSGSKYROOM_H_INCLUDED
