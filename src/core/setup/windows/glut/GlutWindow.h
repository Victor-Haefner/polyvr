#ifndef VRGLUTWINDOW_H_INCLUDED
#define VRGLUTWINDOW_H_INCLUDED

#include <string>

#include <OpenSG/OSGConfig.h>
#include "core/setup/VRSetupFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class GlutWindow : public std::enable_shared_from_this<GlutWindow> {
	public:
	    int winID = -1;

    private:
	    string name = "unnamed";

        void setupAsTop();

	public:
		GlutWindow(string name);
		~GlutWindow();

		static GlutWindowPtr create(string name);
		GlutWindowPtr ptr();

        GlutWindowPtr createSubWindow(string name, int x0, int y0, int width, int height);

        void activate();
};

OSG_END_NAMESPACE;

#endif //VRGLUTWINDOW_H_INCLUDED
