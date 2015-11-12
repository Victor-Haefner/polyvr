#ifndef VRANIMATIONMANAGER_H_INCLUDED
#define VRANIMATIONMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>

#include "core/objects/VRAnimation.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAnimationManager {
    private:
        map<string, VRAnimation*> anim_map;

    protected:
        VRUpdatePtr updateAnimationsFkt;
        void updateAnimations();

    public:
        VRAnimationManager();

        void addAnimation(VRAnimation* anim);
        void remAnimation(VRAnimation* anim);

        template<typename T>
        VRAnimation* addAnimation(float duration, float offset, std::weak_ptr< VRFunction<T> > fkt, T start, T end, bool loop = false);
};

OSG_END_NAMESPACE;

#endif // VRANIMATIONMANAGER_H_INCLUDED
