#ifndef VRGTKWINDOW_H_INCLUDED
#define VRGTKWINDOW_H_INCLUDED

#include "VRWindow.h"

#include <OpenSG/OSGPassiveWindow.h>

struct _GtkDrawingArea;
struct _GtkWidget;
struct _GdkEventScroll;
struct _GdkEventExpose;
struct _GdkRectangle;
struct _GdkEventButton;
struct _GdkEventMotion;
struct _GdkEventKey;

OSG_BEGIN_NAMESPACE;
using namespace std;


class VRGtkWindow : public VRWindow {
    private:
        _GtkDrawingArea* drawArea = 0;
        _GtkWidget* widget = 0;
        PassiveWindowMTRecPtr win;
        bool initialExpose = true;

        bool on_scroll(_GdkEventScroll* e);
        void on_realize();
        bool on_expose(_GdkEventExpose* e);
        void on_resize(_GdkRectangle* a);
        bool on_button(_GdkEventButton* e);
        bool on_motion(_GdkEventMotion* e);
        bool on_key(_GdkEventKey* e);

    public:
        VRGtkWindow(_GtkDrawingArea* glarea, string msaa);
        ~VRGtkWindow();

        static VRGtkWindowPtr create(_GtkDrawingArea* da, string msaa);
        VRGtkWindowPtr ptr();

        PassiveWindowMTRecPtr getOSGWindow();
        void render(bool fromThread = false);
        void clear(Color3f c);

        void setCursor(string c);

        void save(XMLElementPtr node);
        void load(XMLElementPtr node);
};

OSG_END_NAMESPACE;

#endif // VRGTKWINDOW_H_INCLUDED
