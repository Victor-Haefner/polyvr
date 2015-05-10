#ifndef VRMENU_H_INCLUDED
#define VRMENU_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMenu : public VRGeometry {
    private:
        enum TYPE {
            SPRITE,
        };

        TYPE mtype = SPRITE;
        string group;

        VRMenu* active;
        VRMenu* selected;

        VRFunction<int>* fkt;

    public:
        VRMenu(string path);

        VRMenu* append(string path);
        VRMenu* getActive();
        VRMenu* getSelected();

        void enter();
        void move(int dir);
};

OSG_END_NAMESPACE;

#endif // VRMENU_H_INCLUDED
