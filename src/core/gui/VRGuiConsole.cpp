#include "VRGuiConsole.h"

#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>

#include <boost/thread/recursive_mutex.hpp>
#include "VRGuiUtils.h"

typedef boost::recursive_mutex::scoped_lock PLock;
boost::recursive_mutex mtx;

using namespace OSG;

VRConsoleWidget::message::message(string m, string fg, string bg, string l) : msg(m), fg(fg), bg(bg), link(l) {}

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

    textTag = buffer->create_tag();
    textTag->set_property("editable", false);
}

VRConsoleWidget::~VRConsoleWidget() {}

void VRConsoleWidget::write(string s, string fg, string bg, string link) {
    PLock lock(mtx);
    msg_queue.push( message(s,fg,bg,link) );
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

void VRConsoleWidget::update() { // TODO: handle link!
    PLock lock(mtx);
    while(!msg_queue.empty()) {
        if (!isOpen) setColor(notifyColor);
        auto& msg = msg_queue.front();
        textTag->set_property("foreground", msg.fg);
        textTag->set_property("background", msg.bg);
        if (msg.link != "") textTag->set_property("underline", Pango::UNDERLINE_SINGLE);
        buffer->insert_with_tag(buffer->end(), msg.msg, textTag);
		msg_queue.pop();
    }
}

void VRConsoleWidget::forward() {
    if (swin == 0) return;
    if (paused) return;
    auto a = swin->get_vadjustment();
    a->set_value(a->get_upper() - a->get_page_size());
}




