#include "VRAnimationManager.h"
#include <GL/glut.h>
#include <boost/bind.hpp>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

template<typename T>
VRAnimation<T>::VRAnimation() {;}

template<typename T>
VRAnimation<T>::VRAnimation(float _duration, float _offset, VRFunction<T>* _fkt, T _start, T _end, bool _loop) {
    run = false;

    duration = _duration;
    fkt = _fkt;
    start_value = _start;
    end_value = _end;
    offset = _offset;
    loop = _loop;
}

template<typename T>
bool VRAnimation<T>::update(float current_time) {
    if (!run) return false;

    float t = current_time - start_time - offset;
    if (t < 0) return true;

    if (duration > 0.00001) t /= duration;
    else t = 2;

    if (t > 1) {
        if (loop) start();
        else {
            stop();
            (*fkt)(end_value);
        }
        return true;
    }

    T val = start_value + (end_value - start_value)*t;
    (*fkt)(val);

    return true;
}

template<typename T>
int VRAnimationManager::addAnimation(VRAnimation<T>* anim) {
    anim->start();

    id++;
    anim_map[id] = anim;
    return id;
}

template<typename T>
int VRAnimationManager::addAnimation(float duration, float offset, VRFunction<T>* fkt, T start, T end, bool loop) {//Todo: replace VRFunction, template?
    VRAnimation<T>* anim = new VRAnimation<T>(duration, offset, fkt, start, end, loop);
    return addAnimation(anim);
}

OSG_END_NAMESPACE;

