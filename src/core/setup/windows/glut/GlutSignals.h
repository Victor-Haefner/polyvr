#ifndef VRGLUTSIGNALS_H_INCLUDED
#define VRGLUTSIGNALS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <functional>
#include <string>
#include "core/setup/VRSetupFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class GlutSignals : public std::enable_shared_from_this<GlutSignals> {
    public:
        using DisplayCallback  = function<void()>;
        using CloseCallback    = function<void()>;
        using ReshapeCallback  = function<void(int,int)>;
        using KeyboardCallback = function<void(int,bool,bool,int,int)>;
        using MouseCallback = function<void(int,int,int,int)>;
        using MotionCallback = function<void(int,int,bool)>;

	private:
	    string name;

	    DisplayCallback onDisplayCb;
	    CloseCallback onCloseCb;
	    ReshapeCallback onReshapeCb;
	    KeyboardCallback onKeyboardCb;
	    MouseCallback onMouseCb;
	    MotionCallback onMotionCb;

	public:
		GlutSignals(string name);
		~GlutSignals();

		static GlutSignalsPtr create(string name);
		GlutSignalsPtr ptr();

        void onDisplay();
        void onReshape(int w, int h);
        void onClose();
        void onKeyboard(int k, bool down, bool special, int x, int y);
        void onMouse(int b, int s, int x, int y);
        void onMotion(int b, int s, bool p);

		void connect(GlutWindowPtr win);
        void setDisplayCb( DisplayCallback cb );
        void setCloseCb( CloseCallback cb );
        void setReshapeCb( ReshapeCallback cb );
        void setKeyboardCb( KeyboardCallback cb );
        void setMouseCb( MouseCallback cb );
        void setMotionCb( MotionCallback cb );
};

OSG_END_NAMESPACE;

#endif //VRGLUTSIGNALS_H_INCLUDED
