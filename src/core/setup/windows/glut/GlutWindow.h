#ifndef VRGLUTWINDOW_H_INCLUDED
#define VRGLUTWINDOW_H_INCLUDED

#include <string>

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "core/setup/VRSetupFwd.h"

#include "GlutSignals.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class GlutWindow : public std::enable_shared_from_this<GlutWindow> {
	public:
	    int winID = -1;
	    string name = "unnamed";
	    Vec2i unMaximizedSize;
	    Vec2i unMaximizedPosition;

    private:
        GlutSignalsPtr signals;

        void setupAsTop(int x0, int y0, int width, int height);

	public:
		GlutWindow(string name);
		~GlutWindow();

		static GlutWindowPtr create(string name, int x0, int y0, int width, int height);
		GlutWindowPtr ptr();

        GlutWindowPtr createSubWindow(string name, int x0, int y0, int width, int height);

        void connectSignals();

        void activate();
        void setVisible(bool b);
        void setFullscreen(bool b);
        void setMaximized(bool b);
        void enableVSync(bool b);

        void setTitle(string title);
        void setPosition(Vec2i p);
        void setSize(Vec2i s);

        void setDisplayCb( GlutSignals::DisplayCallback cb );
        void setCloseCb( GlutSignals::CloseCallback cb );
        void setReshapeCb( GlutSignals::ReshapeCallback cb );
        void setKeyboardCb( GlutSignals::KeyboardCallback cb );
        void setMouseCb( GlutSignals::MouseCallback cb );
        void setMotionCb( GlutSignals::MotionCallback cb );

        static int getActiveID();
        static GlutWindowPtr getActive();
        static Vec2i getScreenSize();

        Vec2i getPosition();
        Vec2i getSize();
};

OSG_END_NAMESPACE;

#endif //VRGLUTWINDOW_H_INCLUDED
