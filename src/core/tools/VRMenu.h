#ifndef VRMENU_H_INCLUDED
#define VRMENU_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/VRFunction.h"
#include "core/tools/VRToolsFwd.h"

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

        VRMenuPtr active = 0;
        VRMenuPtr parent = 0;
        VRMenuPtr selected = 0;

        VRFunction<VRMenuPtr>* callback = 0;

        VRSelector* selector = 0;

        void setActive();
        VRMenuPtr getActive();
        VRMenuPtr getTopMenu();

        void setLinear();

    public:
        VRMenu(string path = "");

        static VRMenuPtr create(string path = "");
        VRMenuPtr ptr();

        void setLeafType(TYPE l, Vec2f scale);
        void setLayout(LAYOUT l, float param);
        void setCallback(VRFunction<VRMenuPtr>* cb);

        VRMenuPtr append(string path);
        VRMenuPtr getParent();
        VRMenuPtr getSelected();

        void trigger();
        void move(int dir);

        void open();
        void close();
};

OSG_END_NAMESPACE;

#endif // VRMENU_H_INCLUDED
