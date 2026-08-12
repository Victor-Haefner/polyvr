#include "VRGuiConsole.h"
#include "VRGuiManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRMutex.h"
#include "core/utils/toString.h"

using namespace OSG;

VRMutex mtx;

VRConsoleWidget::message::message(string m, string s, VRMessageCbPtr l, int i) : msg(m), style(s), link(l), source(i) {}

VRConsoleWidget::VRConsoleWidget() {
    notifyColor = "#00aaff";
    ID = VRGuiManager::genUUID();
    uiSignal("newConsole", {{"ID",ID}, {"color",notifyColor}});

    auto sigs = OSG::VRGuiSignals::get();
    sigs->addCallback("clickConsole", [&](OSG::VRGuiSignals::Options o) { if (o["ID"] == ID) on_link_activate( o["mark"] ); return true; }, true );

    addStyle( "console91", "#ff3311", "#ffffff", false, false, false, true );
    addStyle( "console92", "#11ff33", "#ffffff", false, false, false, true );
    addStyle( "console93", "#aa8811", "#ffffff", false, false, false, true );
    addStyle( "console94", "#1133ff", "#ffffff", false, false, false, true );
}

VRConsoleWidget::~VRConsoleWidget() {}

VRConsoleWidgetPtr VRConsoleWidget::get(string name) {
    return VRGuiManager::get()->getConsole(name);
}

void VRConsoleWidget::write(string msg, string style, VRMessageCbPtr link, int sourceID) {
    //cout << " - - - - - - - VRConsoleWidget::write " << msg << endl;
    VRLock lock(mtx);

    if (style == "" && msg.find('\033') != string::npos) { // check for style tags
        string aggregate = "";
        string tag = "";
        bool inTag = false;
        for (auto c : msg) {
            if (c == '\033') {
                inTag = true;
                tag = "";
                if (aggregate != "") msg_queue.push( message(aggregate,style,link,sourceID) );
                aggregate = "";
                continue;
            }

            if (inTag) {
                if (c == 'm') {
                    inTag = false;
                    if (tag == "[0") style = "";
                    else if (tag == "[91") style = "console91";
                    else if (tag == "[92") style = "console92";
                    else if (tag == "[93") style = "console93";
                    else if (tag == "[94") style = "console94";
                    continue;
                }
                tag += c;
                continue;
            }

            aggregate += c;
        }
        if (aggregate != "") msg_queue.push( message(aggregate,style,link,sourceID) );
    } else msg_queue.push( message(msg,style,link,sourceID) );
}

void VRConsoleWidget::clear() {
    VRLock lock(mtx);
    std::queue<message>().swap(msg_queue);
    uiSignal("clearConsole", {{"ID",ID}});
}

string VRConsoleWidget::getWindow() { return swin; }
void VRConsoleWidget::pause() { /*paused = getToggleToolButtonState("pause_terminal");*/ }

void VRConsoleWidget::setLabel(string lbl) {
    label = lbl;
    uiSignal("setupConsole", {{"ID",ID}, {"name",lbl}});
}

void VRConsoleWidget::configColor( string c ) {
    notifyColor = c;
    uiSignal("setConsoleLabelColor", {{"ID",ID}, {"color",c}});
}

void VRConsoleWidget::addStyle( string style, string fg, string bg, bool italic, bool bold, bool underlined, bool editable ) {
    /*GtkTextTag* tag = gtk_text_buffer_create_tag(buffer, NULL, NULL);
    function<bool(GObject*, GdkEvent*, GtkTextIter*)> sig = bind(&VRConsoleWidget::on_link_activate, this, placeholders::_1, placeholders::_2, placeholders::_3);
    connect_signal((GtkWidget*)tag, sig, "event");
    g_object_set(tag, "editable", editable, NULL);
    g_object_set(tag, "foreground", fg.c_str(), NULL);
    g_object_set(tag, "background", bg.c_str(), NULL);
    if (underlined) g_object_set(tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
    if (italic) g_object_set(tag, "style", PANGO_STYLE_ITALIC, NULL);
    if (bold) g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);*/
    styles[style] = "";
}

void VRConsoleWidget::on_link_activate(string mark) {
    if (mark[0] == 'S') {
        ;
    }

    if (mark[0] == 'L') {
        if (links.count(mark)) {
            if (auto l = links[mark].link) {
                (*l)( links[mark].msg );
            }
        }
    }
}

void VRConsoleWidget::update() {
    VRLock lock(mtx);
    //if (msg_queue.size() > 0) cout << "VRConsoleWidget::update " << msg_queue.size() << endl;
    while(!msg_queue.empty()) {
        auto& msg = msg_queue.front();

        string tag;
        //if (styles.count( msg.style )) tag = styles[msg.style];
        if (styles.count( msg.style )) tag = msg.style;

        string mark;
        if (msg.link) {
            mark = "L"+genUUID();
            links[mark] = msg;
        }

        if (msg.source != -1) {
            mark = "S"+toString(msg.source);
        }

        uiSignal("pushConsole", {{"ID",ID}, {"string",msg.msg}, {"style",tag}, {"mark",mark}});
		msg_queue.pop();
    }
}

void VRConsoleWidget::forward() {
    //if (swin == 0) return;
    if (paused) return;
    /*GtkAdjustment* a = gtk_scrolled_window_get_vadjustment(swin);
    int p = gtk_adjustment_get_upper(a) - gtk_adjustment_get_page_size(a);
    gtk_adjustment_set_value(a, p);*/
}




