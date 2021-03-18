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

        void addAnimation(VRAnimationPtr anim);
        void remAnimation(VRAnimationPtr anim);

        template<typename T>
        VRAnimationPtr addAnimation(float duration, float offset, std::shared_ptr< VRFunction<T> > fkt, T start, T end, bool loop = false, bool owned = false);
};

OSG_END_NAMESPACE;

#endif // VRANIMATIONMANAGER_H_INCLUDED
