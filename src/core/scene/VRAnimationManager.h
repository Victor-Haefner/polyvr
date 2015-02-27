#ifndef VRANIMATIONMANAGER_H_INCLUDED
#define VRANIMATIONMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

/**
    Base class of all template animations
*/

class VRAnimation_base {
    public:
        virtual bool update(float t) = 0;
};

/**
    A VRAnimation stores a VRFunction && calls it every frame, when running the animation, with an interpolated value.
    One can use any type as long as it can be interpolated, meaning it supports the basic math operations [-, +, *].
*/

template<typename T>
class VRAnimation : public VRAnimation_base {
        float start_time, duration, offset;
        VRFunction<T>* fkt;
        bool run, loop;

        T start_value, end_value;

    public:
        VRAnimation(float _duration, float _offset, VRFunction<T>* _fkt, T _start, T _end, bool _loop);

        void start();

        void end();

        bool update(float current_time);
};

/**
    This manager calls all stored animations every frame && updates them with the current time.
    One can add VRAnimations as objects or initiate them with parameters
*/

class VRAnimationManager {

    private:
        int id;
        map<int, VRAnimation_base*> anim_map;

    protected:
        VRFunction<int>* updateAnimationsFkt;
        void updateAnimations();

    public:
        VRAnimationManager();

        /**
            Add a VRAnimation && starts it.
        */

        template<typename T>
        int addAnimation(VRAnimation<T>* anim);

        /**
            Add a VRAnimation && starts it.
            One can use any type as long as it can be interpolated, meaning it supports the basic math operations [-, +, *].
        */

        template<typename T>
        int addAnimation(float duration, float offset, VRFunction<T>* fkt, T start, T end, bool loop = false);

        void stopAnimation(int i);
};

OSG_END_NAMESPACE;

#endif // VRANIMATIONMANAGER_H_INCLUDED
