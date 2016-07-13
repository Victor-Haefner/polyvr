#ifndef VRGUIRECWIDGET_H_INCLUDED
#define VRGUIRECWIDGET_H_INCLUDED

#include "core/tools/VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include <OpenSG/OSGConfig.h>

namespace Gtk{ class Dialog; class Label; }

OSG_BEGIN_NAMESPACE;

class VRGuiRecWidget {
    private:
        VRRecorderPtr rec;

        VRUpdatePtr updateCb;

        Gtk::Dialog* diag = 0;
        Gtk::Label* lbl = 0;

        void update();
        void buttonHandler(int i);

    public:
        VRGuiRecWidget();
        ~VRGuiRecWidget();

        void setVisible(bool b);
};

OSG_END_NAMESPACE;

#endif // VRGUIRECWIDGET_H_INCLUDED
