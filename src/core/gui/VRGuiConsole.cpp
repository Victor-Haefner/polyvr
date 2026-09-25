#include "VRGuiConsole.h"
#include "VRGuiManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRMutex.h"
#include "core/utils/toString.h"
#include "addons/LLM/VRLLM.h"

using namespace OSG;

VRMutex mtx;

VRConsoleWidget::message::message(string m, string s, VRMessageCbPtr l, int i) : msg(m), style(s), link(l), source(i) {}

VRConsoleWidget::VRConsoleWidget() {
    notifyColor = "#00aaff";
    ID = VRGuiManager::genUUID();

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

void VRConsoleWidget::setup() {
    uiSignal("newConsole", {{"ID",ID}, {"color",notifyColor}});
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
        uiSignal("clickConsoleSource", {{"source",subString(mark, 1)}});
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

void VRConsoleWidget::forward() { // TODO
    if (paused) return;
    /*GtkAdjustment* a = gtk_scrolled_window_get_vadjustment(swin);
    int p = gtk_adjustment_get_upper(a) - gtk_adjustment_get_page_size(a);
    gtk_adjustment_set_value(a, p);*/
}




VRAIConsoleWidget::VRAIConsoleWidget() {
    llm = VRLLM::create();

    onMsgCb = VRMessageCb::create("ai_console_onMsgCb", bind(&VRAIConsoleWidget::onMessage, this, placeholders::_1));
    llm->setMsgCallback(onMsgCb);

    auto mgr = OSG::VRGuiSignals::get();
    //mgr->addCallback("clickConsole", [&](OSG::VRGuiSignals::Options o) { if (o["ID"] == ID) on_link_activate( o["mark"] ); return true; }, true );

    mgr->addCallback("current_ai_config", [&](OSG::VRGuiSignals::Options o) { getKey(o["key"]); model = o["model"]; effort = o["effort"]; return true; }, true );
    mgr->addCallback("ai_dialog_setKey", [&](OSG::VRGuiSignals::Options o) { getKey(o["key"]); return true; }, true );
    mgr->addCallback("ai_model_switch", [&](OSG::VRGuiSignals::Options o) { model = o["selection"]; return true; }, true );
    mgr->addCallback("ai_effort_switch", [&](OSG::VRGuiSignals::Options o) { effort = o["selection"]; return true; }, true );

    mgr->addCallback("on_ai_console_connect", [&](OSG::VRGuiSignals::Options o) { connect(); return true; }, true );
    mgr->addCallback("on_ai_console_run", [&](OSG::VRGuiSignals::Options o) { sendQuery(o["query"]); return true; }, true );

    uiSignal("newAIConsole", {{"ID",ID}, {"color",notifyColor}});
    uiSignal("ai_diag_get_config");
}

VRAIConsoleWidget::~VRAIConsoleWidget() {}

void VRAIConsoleWidget::getKey(string keyVar) {
    const char* _key = std::getenv( keyVar.c_str() );

    if (!_key) {
        status = keyVar + " not found!";
        key = "";
    } else {
        key = _key;
        llm->checkKey(key, status);
    }

    uiSignal("ai_console_key_status", {{"status",status}});
}

void VRAIConsoleWidget::connect() {
    connected = false;
    if (key.empty()) return;

    llm->setApiKey(key);
    llm->setModel(model);
    llm->sendPyAPI();

    connected = true;
    uiSignal("ai_console_set_connected", {{"connected",toString(connected)}});
}

void VRAIConsoleWidget::sendQuery(string q) {
    if (!connected) return;
    string conversation = "singleConversation";
    llm->sendRequest(q, conversation, effort);
    uiSignal("ai_console_append", {{"msg",q}, {"role","user"}});
}

void VRAIConsoleWidget::onMessage(string m) {
    uiSignal("ai_console_append", {{"msg",m}, {"role","llm"}});
}




