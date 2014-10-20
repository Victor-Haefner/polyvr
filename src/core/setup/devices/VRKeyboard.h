#ifndef VRKEYBOARD_H_INCLUDED
#define VRKEYBOARD_H_INCLUDED

#include "VRDevice.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRKeyboard : public VRDevice {
    public:
        VRKeyboard();
        ~VRKeyboard();

        void keyboard(unsigned int k, bool pressed, int x, int y);
        void keyboard_special(int k, bool pressed, int x, int y);
};

OSG_END_NAMESPACE;

#endif // VRKEYBOARD_H_INCLUDED
