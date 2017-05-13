#ifndef VRGUICONSOLE_H_INCLUDED
#define VRGUICONSOLE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <queue>
#include <gtkmm/combobox.h>
#include <gtkmm/textbuffer.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "VRGuiRecWidget.h"
#include "VRGuiFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRConsoleWidget {
    private:
    public:
        Glib::RefPtr<Gtk::TextBuffer> buffer;
        Gtk::ScrolledWindow* swin = 0;
        Gtk::Label* label = 0;
        std::queue<string> msg_queue;
        bool paused = 0;
        bool isOpen = 0;

        string notifyColor = "#006fe0";

        void forward();
        void write(string s);
        void update();

        VRConsoleWidget();

        void queue(string s);
        void clear();
        void pause();
        void setOpen(bool b);
        void setLabel(Gtk::Label* lbl);
        void setColor(string color);
        void configColor(string color);
        void resetColor();
};

OSG_END_NAMESPACE;

#endif // VRGUICONSOLE_H_INCLUDED
