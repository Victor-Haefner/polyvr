#ifndef VRGUICONSOLE_H_INCLUDED
#define VRGUICONSOLE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <queue>

#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "VRGuiFwd.h"


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
        string ID;
        string buffer;
        map<string, string> styles;
        map<string, message> links;
        string swin;
        string label;
        std::queue<message> msg_queue;
        bool paused = 0;
        bool isOpen = 0;
        string notifyColor = "#006fe0";

        bool on_link_activate(string object, string event, string itr);

    public:
        VRConsoleWidget();
        ~VRConsoleWidget();

        static VRConsoleWidgetPtr get(string name);
        string getWindow();

        void clear();
        void pause();
        void setOpen(bool b);
        void setLabel(string lbl);
        void setColor(string color);
        void configColor(string color);
        void resetColor();
        void forward();
        void write(string s, string style = "", shared_ptr< VRFunction<string> > link = 0);
        void addStyle( string style, string fg, string bg, bool italiq, bool bold, bool underlined, bool editable );
        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUICONSOLE_H_INCLUDED
