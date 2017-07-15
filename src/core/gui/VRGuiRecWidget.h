#ifndef VRGUIRECWIDGET_H_INCLUDED
#define VRGUIRECWIDGET_H_INCLUDED

#include "core/tools/VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include <OpenSG/OSGConfig.h>
#include <gdk/gdkevents.h>

namespace Gtk{ class Dialog; class Label; }

OSG_BEGIN_NAMESPACE;

class VRGuiRecWidget {
    private:
        VRRecorderPtr rec;

        VRUpdateCbPtr updateCb;

        Gtk::Dialog* diag = 0;
        Gtk::Label* lbl = 0;

        void on_codec_changed();
        void on_bitrate_changed();

        void update();
        void buttonHandler(int i);
        bool deleteHandler(GdkEventAny* e);

    public:
        VRGuiRecWidget();
        ~VRGuiRecWidget();

        void setVisible(bool b);
};

OSG_END_NAMESPACE;

#endif // VRGUIRECWIDGET_H_INCLUDED
