#ifndef VRKEYBOARD_H_INCLUDED
#define VRKEYBOARD_H_INCLUDED

#include "VRDevice.h"
class _GdkEventKey;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRKeyboard : public VRDevice {
    private:
        _GdkEventKey* gdk_event = 0;

        vector< vector<int> > delayedEvents;

    public:
        VRKeyboard();
        ~VRKeyboard();

        static VRKeyboardPtr create();
        VRKeyboardPtr ptr();

        void applyEvents();

        void keyboard(unsigned int k, bool pressed, int x, int y, bool delayed = true);
        void keyboard_special(int k, bool pressed, int x, int y, bool delayed = true);

        bool shiftDown();
        bool ctrlDown();

#ifndef WITHOUT_GTK
        void setGtkEvent(_GdkEventKey* event);
        _GdkEventKey* getGtkEvent();
#endif
};

OSG_END_NAMESPACE;

#endif // VRKEYBOARD_H_INCLUDED
