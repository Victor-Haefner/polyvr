#ifndef VRFLYSTICK_H_INCLUDED
#define VRFLYSTICK_H_INCLUDED

#include "VRDevice.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFlystick : public VRDevice {
    public:
        VRFlystick();

        void clearSignals();

        void update(int Nb, int* buttons, int Ns, float* sliders);
};


OSG_END_NAMESPACE;

#endif // VRFLYSTICK_H_INCLUDED
