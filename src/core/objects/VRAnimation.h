#ifndef VRANIMATION_H_INCLUDED
#define VRANIMATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "core/utils/VRName.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAnimation : public VRName, public std::enable_shared_from_this<VRAnimation> {
    protected:
        vector<VRAnimCbWeakPtr> weakCallbacks;
        vector<VRAnimCbPtr> ownedCallbacks;
        float start_value = 0;
        float end_value = 1;

        void execCallbacks(float t);

        float start_time = 0;
        float update_time = 0;
        float pause_time = 0;
        float duration = 1;
        float offset = 0;
        float t = 0;
        bool run = false;
        bool paused = false;
        bool loop = false;

    public:
        VRAnimation(string name = "animation");
        ~VRAnimation();
        static VRAnimationPtr create(string name = "animation");

        VRAnimation(float _duration, float _offset, VRAnimCbPtr _fkt, float _start, float _end, bool _loop, bool owned);
        static VRAnimationPtr create(float _duration, float _offset, VRAnimCbPtr _fkt, float _start, float _end, bool _loop, bool owned);

        void addCallback(VRAnimCbPtr fkt);
        void addUnownedCallback(VRAnimCbPtr fkt);

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
