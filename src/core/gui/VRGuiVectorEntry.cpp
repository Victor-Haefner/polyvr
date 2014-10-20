#include "VRGuiVectorEntry.h"
#include "core/utils/toString.h"
#include "VRGuiUtils.h"

#include <gtkmm/entry.h>
#include <gtkmm/paned.h>
#include <gtkmm/fixed.h>
#include <gtkmm/frame.h>
#include <gtkmm/separator.h>
#include <gtkmm/box.h>

VRGuiVectorEntry::VRGuiVectorEntry() {
    ex = ey = ez = 0;
}

bool VRGuiVectorEntry::proxy(GdkEventFocus* focus, sigc::slot<void, OSG::Vec3f&> sig, Gtk::Entry* ex, Gtk::Entry* ey, Gtk::Entry* ez) {
    OSG::Vec3f res;
    res[0] = toFloat( ex->get_text() );
    res[1] = toFloat( ey->get_text() );
    res[2] = toFloat( ez->get_text() );
    sig(res);
    return true;
}

bool VRGuiVectorEntry::proxy2D(GdkEventFocus* focus, sigc::slot<void, OSG::Vec2f&> sig, Gtk::Entry* ex, Gtk::Entry* ey) {
    OSG::Vec2f res;
    res[0] = toFloat( ex->get_text() );
    res[1] = toFloat( ey->get_text() );
    sig(res);
    return true;
}

void VRGuiVectorEntry::init(string placeholder, string label,  sigc::slot<void, OSG::Vec3f&> sig) {
    Gtk::Fixed* ph;
    VRGuiBuilder()->get_widget(placeholder.c_str(), ph);
    Gtk::Container* frame = ph->get_parent();
    frame->remove(*ph);

    Gtk::HBox* hb = new Gtk::HBox();
    frame->add(*hb);

    Gtk::Label* lbl = new Gtk::Label();
    lbl->set_text(label.c_str());
    lbl->set_size_request(50, -1);

    ex = new Gtk::Entry();
    ey = new Gtk::Entry();
    ez = new Gtk::Entry();
    ex->set_has_frame(false);
    ey->set_has_frame(false);
    ez->set_has_frame(false);
    ex->set_size_request(50, -1);
    ey->set_size_request(50, -1);
    ez->set_size_request(50, -1);

    Gtk::VSeparator *s1, *s2, *s3;
    s1 = new Gtk::VSeparator();
    s2 = new Gtk::VSeparator();
    s3 = new Gtk::VSeparator();

    hb->pack_start(*lbl, false, false, 2);
    hb->pack_start(*s1, false, false, 0);
    hb->pack_start(*ex, false, false, 0);
    hb->pack_start(*s2, false, false, 0);
    hb->pack_start(*ey, false, false, 0);
    hb->pack_start(*s3, false, false, 0);
    hb->pack_start(*ez, false, false, 0);
    frame->show_all();

    sigc::slot<bool,GdkEventFocus*> sif = sigc::bind(&VRGuiVectorEntry::proxy, sig, ex, ey, ez);
    ex->signal_focus_out_event().connect( sif );
    ey->signal_focus_out_event().connect( sif );
    ez->signal_focus_out_event().connect( sif );

    sigc::slot<bool> sia_b = sigc::bind<GdkEventFocus*>(&VRGuiVectorEntry::proxy, 0, sig, ex, ey, ez);
    sigc::slot<void> sia = sigc::hide_return( sia_b );
    ex->signal_activate().connect(sia);
    ey->signal_activate().connect(sia);
    ez->signal_activate().connect(sia);
}

void VRGuiVectorEntry::init2D(string placeholder, string label,  sigc::slot<void, OSG::Vec2f&> sig) {
    Gtk::Fixed* ph;
    VRGuiBuilder()->get_widget(placeholder.c_str(), ph);
    Gtk::Container* frame = ph->get_parent();
    frame->remove(*ph);

    Gtk::HBox* hb = new Gtk::HBox();
    frame->add(*hb);

    lbl = new Gtk::Label();
    lbl->set_text(label.c_str());
    lbl->set_size_request(50, -1);

    ex = new Gtk::Entry();
    ey = new Gtk::Entry();
    ex->set_has_frame(false);
    ey->set_has_frame(false);
    ex->set_size_request(50, -1);
    ey->set_size_request(50, -1);

    Gtk::VSeparator *s1, *s2;
    s1 = new Gtk::VSeparator();
    s2 = new Gtk::VSeparator();

    hb->pack_start(*lbl, false, false, 2);
    hb->pack_start(*s1, false, false, 0);
    hb->pack_start(*ex, false, false, 0);
    hb->pack_start(*s2, false, false, 0);
    hb->pack_start(*ey, false, false, 0);
    frame->show_all();

    sigc::slot<bool,GdkEventFocus*> sif = sigc::bind(&VRGuiVectorEntry::proxy2D, sig, ex, ey);
    ex->signal_focus_out_event().connect( sif );
    ey->signal_focus_out_event().connect( sif );

    sigc::slot<bool> sia_b = sigc::bind<GdkEventFocus*>(&VRGuiVectorEntry::proxy2D, 0, sig, ex, ey);
    sigc::slot<void> sia = sigc::hide_return( sia_b );
    ex->signal_activate().connect(sia);
    ey->signal_activate().connect(sia);
}


void VRGuiVectorEntry::set(OSG::Vec3f v) {
    ex->set_text(toString(v[0]));
    ey->set_text(toString(v[1]));
    if (ez) ez->set_text(toString(v[2]));
}

void VRGuiVectorEntry::set(OSG::Vec2f v) {
    ex->set_text(toString(v[0]));
    ey->set_text(toString(v[1]));
}

void VRGuiVectorEntry::setFontColor(OSG::Vec3f c) {
    Gdk::Color col("#FFFFFF");
    col.set_rgb_p(c[0], c[1], c[2]);
    cout << "\nCOL " << c << " " << col.to_string() << endl;

    if (lbl) lbl->modify_fg(lbl->get_state(), col);
    if (ex) ex->modify_text(ex->get_state(), col);
    if (ey) ey->modify_text(ey->get_state(), col);
    if (ez) ez->modify_text(ez->get_state(), col);
}
