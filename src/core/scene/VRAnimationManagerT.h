#include "VRAnimationManager.h"
#include <GL/glut.h>
#include <boost/bind.hpp>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

template<typename T>
VRAnimation::VRAnimation(float _duration, float _offset, VRFunction<T>* _fkt, T _start, T _end, bool _loop) : VRAnimation(_fkt->getBaseName()) {
    run = false;

    duration = _duration;
    offset = _offset;
    loop = _loop;

    auto i = new interpolatorT<T>();
    i->fkt = _fkt;
    i->start_value = _start;
    i->end_value = _end;
    interp = i;

    setNameSpace("animation");
    setName("anim"); // TODO: _fkt->getBaseName() is an empty string??
}

template<typename T>
VRAnimation* VRAnimationManager::addAnimation(float duration, float offset, VRFunction<T>* fkt, T start, T end, bool loop) {//Todo: replace VRFunction, template?
    VRAnimation* anim = new VRAnimation(duration, offset, fkt, start, end, loop);
    addAnimation(anim);
    anim->start(offset);
    return anim;
}

OSG_END_NAMESPACE;

