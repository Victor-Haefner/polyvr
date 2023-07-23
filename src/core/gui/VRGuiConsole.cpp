#include "VRGuiConsole.h"
#include "VRGuiManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRMutex.h"
#include "core/utils/toString.h"

using namespace OSG;

VRMutex mtx;

VRConsoleWidget::message::message(string m, string s, shared_ptr< VRFunction<string> > l) : msg(m), style(s), link(l) {}

VRConsoleWidget::VRConsoleWidget() {
    notifyColor = "#00aaff";
    ID = VRGuiManager::genUUID();
    uiSignal("newConsole", {{"ID",ID}, {"color",notifyColor}});

    auto sigs = OSG::VRGuiSignals::get();
    sigs->addCallback("clickConsole", [&](OSG::VRGuiSignals::Options o) { if (o["ID"] == ID) on_link_activate( o["mark"] ); return true; }, true );

    /*buffer = gtk_text_buffer_new(0);
    GtkTextView* term_view = (GtkTextView*)gtk_text_view_new_with_buffer(buffer);
    PangoFontDescription* fdesc = pango_font_description_new();
    pango_font_description_set_family(fdesc, "monospace");
    pango_font_description_set_size(fdesc, 10 * PANGO_SCALE);
    gtk_widget_modify_font((GtkWidget*)term_view, fdesc);
    pango_font_description_free(fdesc);

    swin = (GtkScrolledWindow*)gtk_scrolled_window_new(0,0);
    gtk_container_add((GtkContainer*)swin, (GtkWidget*)term_view);
    gtk_widget_set_size_request((GtkWidget*)swin, -1, 70);

    GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment(swin);
    function<void(void)> sig = bind(&VRConsoleWidget::forward, this);
    connect_signal((GtkWidget*)adj, sig, "changed");

    setToolButtonCallback("toolbutton24", bind(&VRConsoleWidget::clear, this));
    setToolButtonCallback("toolbutton25", bind(&VRConsoleWidget::forward, this));
    setToolButtonCallback("pause_terminal", bind(&VRConsoleWidget::pause, this));*/

    addStyle( "console91", "#ff3311", "#ffffff", false, false, false, true );
    addStyle( "console92", "#11ff33", "#ffffff", false, false, false, true );
    addStyle( "console93", "#aa8811", "#ffffff", false, false, false, true );
    addStyle( "console94", "#1133ff", "#ffffff", false, false, false, true );
}

VRConsoleWidget::~VRConsoleWidget() {}

VRConsoleWidgetPtr VRConsoleWidget::get(string name) {
    return VRGuiManager::get()->getConsole(name);
}

void VRConsoleWidget::write(string msg, string style, shared_ptr< VRFunction<string> > link) {
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
                if (aggregate != "") msg_queue.push( message(aggregate,style,link) );
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
        if (aggregate != "") msg_queue.push( message(aggregate,style,link) );
    } else msg_queue.push( message(msg,style,link) );
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

//bool VRConsoleWidget::on_link_activate(string object, string event, string itr) {
void VRConsoleWidget::on_link_activate(string mark) {
    /*GdkEventButton* event_btn = (GdkEventButton*)event;
    if (event->type == GDK_BUTTON_PRESS && event_btn->button == 1) {
        GtkTextIter markItr, tagToggle, lineEnd;
        tagToggle = *itr;
        lineEnd = *itr;

        gtk_text_iter_forward_to_tag_toggle(&tagToggle, 0);
        gtk_text_iter_forward_to_line_end(&lineEnd);
        gtk_text_iter_forward_char(&lineEnd);
        int c = gtk_text_iter_compare(&lineEnd, &tagToggle);
        markItr = (c == -1) ? lineEnd : tagToggle;
        GSList* marks = gtk_text_iter_get_marks(&markItr);
        GSList* p = marks;
        while (p) {
            GtkTextMark* mark = (GtkTextMark*)p->data;
            if (links.count(mark)) {
                if (auto l = links[mark].link) (*l)( links[mark].msg );
            }
            p = p->next;
        }
        g_slist_free(marks);
        return true;
    }*/

    if (links.count(mark)) {
        if (auto l = links[mark].link) {
            (*l)( links[mark].msg );
        }
    }
}

void VRConsoleWidget::update() {
    VRLock lock(mtx);
    if (msg_queue.size() > 0) cout << "VRConsoleWidget::update " << msg_queue.size() << endl;
    while(!msg_queue.empty()) {
        auto& msg = msg_queue.front();

        string tag;
        //if (styles.count( msg.style )) tag = styles[msg.style];
        if (styles.count( msg.style )) tag = msg.style;

        string mark;
        if (msg.link) {
                mark = genUUID();
                links[mark] = msg;
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




