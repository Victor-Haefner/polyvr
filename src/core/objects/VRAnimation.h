#ifndef VRANIMATION_H_INCLUDED
#define VRANIMATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "core/utils/VRName.h"

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAnimation : public VRName {
    protected:
        struct interpolator {
            virtual ~interpolator();
            virtual void update(float t) = 0;
        };

        template<typename T>
        struct interpolatorT : interpolator {
            VRFunction<T>* fkt;
            T start_value, end_value;
            void update(float t) {
                T val = start_value + (end_value - start_value)*t;
                (*fkt)(val);
            }
        };

        interpolator* interp = 0 ;
        float start_time = 0;
        float duration = 0;
        float offset = 0;
        bool run = false;
        bool loop = false;

    public:
        VRAnimation(string name);

        template<typename T>
        VRAnimation(float _duration, float _offset, VRFunction<T>* _fkt, T _start, T _end, bool _loop);

        void setSimpleCallback(VRFunction<float>* fkt, float _duration);

        void setLoop(bool b);
        bool getLoop();

        void setDuration(float t);
        float getDuration();

        void start(float offset = 0);
        void stop();
        bool isActive();

        bool update(float t);
};


OSG_END_NAMESPACE;

#endif // VRANIMATION_H_INCLUDED
