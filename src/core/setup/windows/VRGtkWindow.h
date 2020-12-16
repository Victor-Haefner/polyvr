#ifndef VRGTKWINDOW_H_INCLUDED
#define VRGTKWINDOW_H_INCLUDED

#include "VRWindow.h"

#include <OpenSG/OSGPassiveWindow.h>

#if GTK_MAJOR_VERSION == 2
#define GDKRECTANGLE _GdkRectangle
#else
#define GDKRECTANGLE _cairo_rectangle_int
#endif

struct _GtkWidget;
struct _GdkEventScroll;
struct _GdkEventExpose;
struct GDKRECTANGLE;
struct _GdkEventButton;
struct _GdkEventMotion;
struct _GdkEventKey;

struct _GdkGLContext;
struct _cairo;

OSG_BEGIN_NAMESPACE;
using namespace std;


class VRGtkWindow : public VRWindow {
    private:
		_GtkWidget* area = 0;
        _GtkWidget* widget = 0;
        _GdkGLContext* glcontext = 0;
        VRHeadMountedDisplayPtr hmd = 0;
        PassiveWindowMTRecPtr win;
        bool initialExpose = true;
        bool isRealized = false;

        bool on_scroll(_GdkEventScroll* e);
        void on_resize(GDKRECTANGLE* a);
        bool on_button(_GdkEventButton* e);
        bool on_motion(_GdkEventMotion* e);
        bool on_key(_GdkEventKey* e);

        bool on_expose(_cairo* e);
        void on_realize();
        bool on_render(_GdkGLContext* glcontext);

    public:
        VRGtkWindow(_GtkWidget* glarea, string msaa);
        ~VRGtkWindow();

        static VRGtkWindowPtr create(_GtkWidget* da, string msaa);
        VRGtkWindowPtr ptr();

        void render(bool fromThread = false);
        void clear(Color3f c);

        void forceSize(int W, int H);
        void enableVSync(bool b);

        void setBlitID(int ID, int bType);

        void setCursor(string c);

        void save(XMLElementPtr node);
        void load(XMLElementPtr node);
};

OSG_END_NAMESPACE;

#endif // VRGTKWINDOW_H_INCLUDED
