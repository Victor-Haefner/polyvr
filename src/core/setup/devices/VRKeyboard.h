#ifndef VRKEYBOARD_H_INCLUDED
#define VRKEYBOARD_H_INCLUDED

#include "VRDevice.h"
class _GdkEventKey;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRKeyboard : public VRDevice {
    private:
        _GdkEventKey* gdk_event = 0;

    public:
        VRKeyboard();
        ~VRKeyboard();

        void keyboard(unsigned int k, bool pressed, int x, int y);
        void keyboard_special(int k, bool pressed, int x, int y);
        void setGtkEvent(_GdkEventKey* event);
        _GdkEventKey* getGtkEvent();
};

OSG_END_NAMESPACE;

#endif // VRKEYBOARD_H_INCLUDED
