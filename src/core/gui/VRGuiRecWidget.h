#ifndef VRGUIRECWIDGET_H_INCLUDED
#define VRGUIRECWIDGET_H_INCLUDED

#include "core/tools/VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include <OpenSG/OSGConfig.h>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGuiRecWidget {
    private:
        VRRecorderPtr rec;
        VRUpdateCbPtr updateCb;

        string diag;
        string lbl;
        string screenshots_path;

        void on_res_changed();
        void on_codec_changed();
        void on_bitrate_changed();
        void on_toggle_vsync();

        void onSCPathChanged();
        void onSCTrigger();
        void onSCChangeDir();

        void update();
        void buttonHandler(int i);

    public:
        VRGuiRecWidget();
        ~VRGuiRecWidget();

        void setVisible(bool b);
};

OSG_END_NAMESPACE;

#endif // VRGUIRECWIDGET_H_INCLUDED
