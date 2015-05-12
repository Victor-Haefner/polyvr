#ifndef VRMENU_H_INCLUDED
#define VRMENU_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSelector;

class VRMenu : public VRGeometry {
    public:
        enum TYPE {
            SPRITE,
        };

        enum LAYOUT {
            LINEAR,
            CIRCULAR,
        };

    private:
        TYPE mtype = SPRITE;
        string group;

        LAYOUT layout = LINEAR;
        Vec2f scale = Vec2f(0.3,0.4);
        float param = 0.1;

        VRMenu* active = 0;
        VRMenu* parent = 0;
        VRMenu* selected = 0;

        VRFunction<VRMenu*>* callback = 0;

        VRSelector* selector = 0;

        void setActive();
        VRMenu* getActive();
        VRMenu* getTopMenu();

        void setLinear();

    public:
        VRMenu(string path = "");

        void setLeafType(TYPE l, Vec2f scale);
        void setLayout(LAYOUT l, float param);
        void setCallback(VRFunction<VRMenu*>* cb);

        VRMenu* append(string path);
        VRMenu* getParent();
        VRMenu* getSelected();

        void trigger();
        void move(int dir);

        void open();
        void close();
};

OSG_END_NAMESPACE;

#endif // VRMENU_H_INCLUDED
