#ifndef VRANIMATIONMANAGER_H_INCLUDED
#define VRANIMATIONMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>

#include "core/utils/VRFunctionFwd.h"
#include "core/tools/VRToolsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAnimationManager {
    private:
        map<string, VRAnimationPtr> anim_map;

    protected:
        VRUpdateCbPtr updateAnimationsFkt;
        void updateAnimations();

    public:
        VRAnimationManager();
        ~VRAnimationManager();

        void clear();

        void addAnimation(VRAnimationPtr anim);
        void remAnimation(VRAnimationPtr anim);

        VRAnimationPtr addAnimation(float duration, float offset, VRAnimCbPtr fkt, float start, float end, bool loop = false, bool owned = false);
};

OSG_END_NAMESPACE;

#endif // VRANIMATIONMANAGER_H_INCLUDED
