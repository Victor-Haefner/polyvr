#ifndef VRGUICONSOLE_H_INCLUDED
#define VRGUICONSOLE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <queue>

#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "addons/LLM/VRLLMFwd.h"
#include "VRGuiFwd.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

class VRConsoleWidget {
    public:
        struct message {
            string msg;
            string style;
            VRMessageCbPtr link;
            int source = -1;

            message() {}
            message(string m, string s, VRMessageCbPtr l, int i);
        };

    protected:
        string ID;
        string buffer;
        map<string, string> styles;
        map<string, message> links;
        string swin;
        string label;
        std::queue<message> msg_queue;
        bool paused = 0;
        bool isOpen = 0;
        string notifyColor;

        void on_link_activate(string mark);

    public:
        VRConsoleWidget();
        ~VRConsoleWidget();

        static VRConsoleWidgetPtr get(string name);
        string getWindow();

        void setup();
        void clear();
        void pause();
        void setLabel(string lbl);
        void configColor(string color);
        void forward();
        void write(string s, string style = "", VRMessageCbPtr link = 0, int sourceID = -1);
        void addStyle( string style, string fg, string bg, bool italiq, bool bold, bool underlined, bool editable );
        void update();
};

class VRAIConsoleWidget : public VRConsoleWidget {
    private:
        bool connected = false;
        string status;

        string key;
        string model;
        string effort;

        VRMessageCbPtr onMsgCb;
        VRLLMPtr llm;

        void onMessage(string m);

    public:
        VRAIConsoleWidget();
        ~VRAIConsoleWidget();

        void connect();
        void getKey(string keyVar);
        void sendQuery(string q);
};

OSG_END_NAMESPACE;

#endif // VRGUICONSOLE_H_INCLUDED
