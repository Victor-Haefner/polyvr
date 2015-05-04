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
    protected:
        float start_time = 0;
        float duration = 0;
        float offset = 0;
        bool run = false;
        bool loop = false;

    public:
        virtual bool update(float t) = 0;
        void start();
        void stop();
        bool isActive();
};

/**
    A VRAnimation stores a VRFunction && calls it every frame, when running the animation, with an interpolated value.
    One can use any type as long as it can be interpolated, meaning it supports the basic math operations [-, +, *].
*/

template<typename T>
class VRAnimation : public VRAnimation_base {
    private:
        VRFunction<T>* fkt;
        T start_value, end_value;

    public:
        VRAnimation();
        VRAnimation(float _duration, float _offset, VRFunction<T>* _fkt, T _start, T _end, bool _loop);

        bool update(float current_time);
};

/**
    This manager calls all stored animations every frame && updates them with the current time.
    One can add VRAnimations as objects || initiate them with parameters
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
