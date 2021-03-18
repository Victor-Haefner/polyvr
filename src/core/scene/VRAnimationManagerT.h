#include "VRAnimationManager.h"
#include "core/objects/VRAnimation.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

template<typename T>
VRAnimation::VRAnimation(float _duration, float _offset, std::shared_ptr< VRFunction<T> > _fkt, T _start, T _end, bool _loop, bool owned) : VRAnimation(_fkt->name) {
    run = false;

    duration = _duration;
    offset = _offset;
    loop = _loop;

    auto i = new interpolatorT<T>();
    if (owned) i->sp = _fkt;
    i->fkt = _fkt;
    i->start_value = _start;
    i->end_value = _end;
    interp = i;

    setNameSpace("animation");
    setName("anim"); // TODO: _fkt->getBaseName() is an empty string??
}

template<typename T>
VRAnimationPtr VRAnimationManager::addAnimation(float duration, float offset, std::shared_ptr< VRFunction<T> > fkt, T start, T end, bool loop, bool owned) {//Todo: replace VRFunction, template?
    auto anim = VRAnimation::create(duration, offset, fkt, start, end, loop, owned);
    addAnimation(anim);
    anim->start(offset);
    return anim;
}

template<typename T>
shared_ptr<VRAnimation> VRAnimation::create(float duration, float offset, std::shared_ptr< VRFunction<T> > fkt, T start, T end, bool loop, bool owned) { return shared_ptr<VRAnimation>(new VRAnimation(duration, offset, fkt, start, end, loop, owned)); }

OSG_END_NAMESPACE;

