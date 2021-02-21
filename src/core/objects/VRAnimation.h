#ifndef VRANIMATION_H_INCLUDED
#define VRANIMATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "core/utils/VRName.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAnimation : public VRName, public std::enable_shared_from_this<VRAnimation> {
    protected:
        struct interpolator {
            virtual ~interpolator();
            virtual void update(float t) = 0;
            virtual void own(bool b) = 0;
        };

        template<typename T>
        struct interpolatorT : interpolator {
            std::shared_ptr< VRFunction<T> > sp;
            std::weak_ptr< VRFunction<T> > fkt;
            T start_value, end_value;
            void update(float t) {
                T val = start_value + (end_value - start_value)*t;
                if ( auto sp = fkt.lock() ) (*sp)(val);
            }

            void own(bool b) {
                if (b) sp = fkt.lock();
                else sp = 0;
            }
        };

        interpolator* interp = 0 ;
        float start_time = 0;
        float update_time = 0;
        float pause_time = 0;
        float duration = 0;
        float offset = 0;
        float t = 0;
        bool run = false;
        bool paused = false;
        bool loop = false;

    public:
        VRAnimation(string name = "animation");
        ~VRAnimation();
        static VRAnimationPtr create(string name = "animation");

        template<typename T>
        VRAnimation(float _duration, float _offset, std::shared_ptr< VRFunction<T> > _fkt, T _start, T _end, bool _loop, bool owned);

        template<typename T>
        static VRAnimationPtr create(float _duration, float _offset, std::shared_ptr< VRFunction<T> > _fkt, T _start, T _end, bool _loop, bool owned);

        void setUnownedCallback(VRAnimCbPtr fkt);
        void setCallback(VRAnimCbPtr fkt);

        void setLoop(bool b);
        bool getLoop();

        void setDuration(float t);
        float getDuration();

        void start(float offset = 0);
        void stop();
        bool isActive();

        void pause();
        void resume();
        bool isPaused();
        void goTo(float t);

        bool update(float t);
};


OSG_END_NAMESPACE;

#endif // VRANIMATION_H_INCLUDED
