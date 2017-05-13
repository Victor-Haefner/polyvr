#ifndef VRGUICONSOLE_H_INCLUDED
#define VRGUICONSOLE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <queue>
#include <glibmm/refptr.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "VRGuiFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRConsoleWidget {
    public:
        struct message {
            string msg;
            string fg;
            string bg;
            string link;

            message(string m, string fg, string bg, string l);
        };

    private:
        Glib::RefPtr<Gtk::TextBuffer> buffer;
        Glib::RefPtr<Gtk::TextTag> textTag;
        Gtk::ScrolledWindow* swin = 0;
        Gtk::Label* label = 0;
        std::queue<message> msg_queue;
        bool paused = 0;
        bool isOpen = 0;
        string notifyColor = "#006fe0";

    public:
        VRConsoleWidget();
        ~VRConsoleWidget();

        Gtk::ScrolledWindow* getWindow();

        void clear();
        void pause();
        void setOpen(bool b);
        void setLabel(Gtk::Label* lbl);
        void setColor(string color);
        void configColor(string color);
        void resetColor();
        void forward();
        void write(string s, string fg = "#000000", string bg = "#ffffff", string link = "");
        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUICONSOLE_H_INCLUDED
