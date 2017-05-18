#include "VRGuiConsole.h"
#include "core/utils/VRFunction.h"

#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>

#include <boost/thread/recursive_mutex.hpp>
#include "VRGuiUtils.h"

typedef boost::recursive_mutex::scoped_lock PLock;
boost::recursive_mutex mtx;

using namespace OSG;

VRConsoleWidget::message::message(string m, string s, shared_ptr< VRFunction<string> > l) : msg(m), style(s), link(l) {}

VRConsoleWidget::VRConsoleWidget() {
    buffer = Gtk::TextBuffer::create();
    Gtk::TextView* term_view = Gtk::manage(new Gtk::TextView(buffer));
    Pango::FontDescription fdesc;
    fdesc.set_family("monospace");
    fdesc.set_size(10 * PANGO_SCALE);
    term_view->modify_font(fdesc);
    swin = Gtk::manage(new Gtk::ScrolledWindow());
    swin->add(*term_view);
    swin->set_size_request(-1,70);

    swin->get_vadjustment()->signal_changed().connect( sigc::mem_fun(*this, &VRConsoleWidget::forward) );
    setToolButtonCallback("toolbutton24", sigc::mem_fun(*this, &VRConsoleWidget::clear));
    setToolButtonCallback("toolbutton25", sigc::mem_fun(*this, &VRConsoleWidget::forward));
    setToolButtonCallback("pause_terminal", sigc::mem_fun(*this, &VRConsoleWidget::pause));
}

VRConsoleWidget::~VRConsoleWidget() {}

void VRConsoleWidget::write(string msg, string style, shared_ptr< VRFunction<string> > link) {
    PLock lock(mtx);
    msg_queue.push( message(msg,style,link) );
}

void VRConsoleWidget::clear() {
    PLock lock(mtx);
    std::queue<message>().swap(msg_queue);
    buffer->set_text("");
    resetColor();
}

Gtk::ScrolledWindow* VRConsoleWidget::getWindow() { return swin; }
void VRConsoleWidget::pause() { paused = getToggleButtonState("pause_terminal"); }
void VRConsoleWidget::setLabel(Gtk::Label* lbl) { label = lbl; }
void VRConsoleWidget::setOpen(bool b) {
    isOpen = b;
    if (!b) resetColor();
}

void VRConsoleWidget::setColor(string color) {
    label->modify_fg( Gtk::STATE_ACTIVE , Gdk::Color(color));
    label->modify_fg( Gtk::STATE_NORMAL , Gdk::Color(color));
}

void VRConsoleWidget::configColor( string c ) { notifyColor = c; }

void VRConsoleWidget::resetColor() {
    label->unset_fg( Gtk::STATE_ACTIVE );
    label->unset_fg( Gtk::STATE_NORMAL );
}

void VRConsoleWidget::addStyle( string style, string fg, string bg, bool italic, bool bold, bool underlined ) {
    auto tag = buffer->create_tag();
    tag->set_property("editable", false);
    tag->set_property("foreground", fg);
    tag->set_property("background", bg);
    if (underlined) tag->set_property("underline", Pango::UNDERLINE_SINGLE);
    if (italic) tag->set_property("style", Pango::STYLE_ITALIC);
    if (bold) tag->set_property("weight", Pango::WEIGHT_BOLD);
    styles[style] = tag;
}

bool VRConsoleWidget::on_link_activate(const Glib::RefPtr<Glib::Object>& obj, GdkEvent* event, const Gtk::TextIter& itr) {
    GdkEventButton* event_btn = (GdkEventButton*)event;
    if (event->type == GDK_BUTTON_PRESS && event_btn->button == 1) {
        Glib::RefPtr< Gtk::TextTag > null;
        Gtk::TextIter end = itr;
        end.forward_to_tag_toggle(null);
        for (auto mark : end.get_marks()) {
            if (links.count(mark)) {
                if (auto l = links[mark].link) (*l)( links[mark].msg );
            }
        }
        return true;
    }
    return false;
}

void VRConsoleWidget::update() {
    PLock lock(mtx);
    while(!msg_queue.empty()) {
        if (!isOpen) setColor(notifyColor);
        auto& msg = msg_queue.front();
        if (styles.count( msg.style )) {
            auto tag = styles[msg.style];
            if (msg.link) tag->signal_event().connect( sigc::mem_fun(*this, &VRConsoleWidget::on_link_activate) );
            Gtk::TextIter itr = buffer->insert_with_tag(buffer->end(), msg.msg, tag);
            //Glib::RefPtr<TextBuffer::Mark> mark;
            Glib::RefPtr<Gtk::TextBuffer::Mark> mark = Gtk::TextBuffer::Mark::create();
            buffer->add_mark(mark, itr);
            if (msg.link) links[mark] = msg;
        }
        else buffer->insert(buffer->end(), msg.msg);
		msg_queue.pop();
    }
}

void VRConsoleWidget::forward() {
    if (swin == 0) return;
    if (paused) return;
    auto a = swin->get_vadjustment();
    a->set_value(a->get_upper() - a->get_page_size());
}




