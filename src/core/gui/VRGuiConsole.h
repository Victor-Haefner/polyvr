#ifndef VRGUICONSOLE_H_INCLUDED
#define VRGUICONSOLE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <queue>
#include <gdkmm/event.h>
#include <gtkmm/textbuffer.h>

#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "VRGuiFwd.h"

namespace Gtk{ class TextIter; };

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRConsoleWidget {
    public:
        struct message {
            string msg;
            string style;
            shared_ptr< VRFunction<string> > link;

            message() {}
            message(string m, string s, shared_ptr< VRFunction<string> > l);
        };

    private:
        Glib::RefPtr<Gtk::TextBuffer> buffer;
        map<string, Glib::RefPtr<Gtk::TextTag>> styles;
        map<Glib::RefPtr<Gtk::TextBuffer::Mark>, message> links;
        Gtk::ScrolledWindow* swin = 0;
        Gtk::Label* label = 0;
        std::queue<message> msg_queue;
        bool paused = 0;
        bool isOpen = 0;
        string notifyColor = "#006fe0";

        //bool on_link_activate(GdkEvent* event, const Gtk::TextIter& itr);
        bool on_link_activate(const Glib::RefPtr<Glib::Object>& obj, GdkEvent* event, const Gtk::TextIter& itr);
        //bool on_link_activate(GdkEvent* event);

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
        void write(string s, string style = "", shared_ptr< VRFunction<string> > link = 0);
        void addStyle( string style, string fg, string bg, bool italiq, bool bold, bool underlined );
        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUICONSOLE_H_INCLUDED
