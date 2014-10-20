#ifndef VRGUIBITS_H_INCLUDED
#define VRGUIBITS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <gtkmm/combobox.h>
#include "core/setup/devices/VRSignal.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiBits {
    private:
        GtkWidget* term_box;

        void hideAbout(int i);

    public:
        VRGuiBits();

        void setSceneSignal(VRSignal* sig);

        static void write_to_terminal(string s);

        bool toggleFullscreen(GdkEventKey* k);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUIBITS_H_INCLUDED
