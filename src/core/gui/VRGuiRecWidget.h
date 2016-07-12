#ifndef VRGUIRECWIDGET_H_INCLUDED
#define VRGUIRECWIDGET_H_INCLUDED

#include "core/tools/VRToolsFwd.h"
#include <OpenSG/OSGConfig.h>

namespace Gtk{ class Dialog; }

OSG_BEGIN_NAMESPACE;

class VRGuiRecWidget {
    private:
        VRRecorderPtr rec;

        Gtk::Dialog* diag = 0;

    public:
        VRGuiRecWidget();
        ~VRGuiRecWidget();

        void setVisible(bool b);
};

OSG_END_NAMESPACE;

#endif // VRGUIRECWIDGET_H_INCLUDED
