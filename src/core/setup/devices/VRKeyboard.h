#ifndef VRKEYBOARD_H_INCLUDED
#define VRKEYBOARD_H_INCLUDED

#include "VRDevice.h"
class _GdkEventKey;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRKeyboard : public VRDevice {
    public:
        struct KeyEvent {
            int keyval = 0;
            int state = 0;
        };

    private:
        KeyEvent event;

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
        bool altDown();
        bool lockDown();

        KeyEvent& getEvent();
};

OSG_END_NAMESPACE;

#endif // VRKEYBOARD_H_INCLUDED
