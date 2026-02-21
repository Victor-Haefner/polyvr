#ifndef VRGLUTWINDOW_H_INCLUDED
#define VRGLUTWINDOW_H_INCLUDED

#include <string>

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "core/setup/VRSetupFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class GlutWindow : public std::enable_shared_from_this<GlutWindow> {
	public:
	    int winID = -1;
	    string name = "unnamed";

    private:

        void setupAsTop(int x0, int y0, int width, int height);

	public:
		GlutWindow(string name);
		~GlutWindow();

		static GlutWindowPtr create(string name, int x0, int y0, int width, int height);
		GlutWindowPtr ptr();

        GlutWindowPtr createSubWindow(string name, int x0, int y0, int width, int height);

        void activate();
        void enableVSync(bool b);

        void setTitle(string title);

        static Vec2i getScreenSize();
        Vec2i getPosition();
        Vec2i getSize();
};

OSG_END_NAMESPACE;

#endif //VRGLUTWINDOW_H_INCLUDED
