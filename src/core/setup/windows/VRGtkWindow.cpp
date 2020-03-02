#include "VRGtkWindow.h"
#include <gtkmm/drawingarea.h>
#include <gtkmm/window.h>
#include <gtkmm/label.h>
#include <gtkmm/builder.h>
#include <gdkmm/cursor.h>
#include <gdk/gdkgl.h>
#include <gtk/gtkgl.h>
#include <boost/thread/recursive_mutex.hpp>

#include "../devices/VRKeyboard.h"
#include "../devices/VRMouse.h"
#include "core/utils/VRTimer.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/VRProfiler.h"
#include "core/scene/VRScene.h"
#include "core/gui/VRGuiManager.h"

typedef boost::recursive_mutex::scoped_lock PLock;

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGtkWindow::VRGtkWindow(Gtk::DrawingArea* da) {
    type = 2;
    drawArea = da;
    widget = (GtkWidget*)drawArea->gobj();
    if (gtk_widget_get_realized(widget)) cout << "Warning: glarea is realized!\n";

    auto mode = (GdkGLConfigMode)(GDK_GL_MODE_RGB | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        mode = (GdkGLConfigMode)(GDK_GL_MODE_RGB | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_STEREO);
    GdkGLConfig* glConfigMode = gdk_gl_config_new_by_mode(mode);
    gtk_widget_set_gl_capability(widget,glConfigMode,NULL,true,GDK_GL_RGBA_TYPE);

    drawArea->show();
    drawArea->add_events((Gdk::EventMask)GDK_VISIBILITY_NOTIFY_MASK);
    drawArea->add_events((Gdk::EventMask)GDK_BUTTON_PRESS_MASK);
    drawArea->add_events((Gdk::EventMask)GDK_BUTTON_RELEASE_MASK);
    drawArea->add_events((Gdk::EventMask)GDK_POINTER_MOTION_MASK);
    drawArea->add_events((Gdk::EventMask)GDK_KEY_PRESS_MASK);
    drawArea->add_events((Gdk::EventMask)GDK_KEY_RELEASE_MASK);
    drawArea->add_events((Gdk::EventMask)GDK_SCROLL_MASK);
    drawArea->set_flags(Gtk::CAN_FOCUS);

    win = PassiveWindow::create();
    _win = win;
    win->setSize(width, height);

    signals.push_back( drawArea->signal_realize().connect(sigc::mem_fun(*this, &VRGtkWindow::on_realize)) );
    signals.push_back( drawArea->signal_expose_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_expose)) );
    signals.push_back( drawArea->signal_size_allocate().connect(sigc::mem_fun(*this, &VRGtkWindow::on_resize)) );

    signals.push_back( drawArea->signal_scroll_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_scroll)) );
    signals.push_back( drawArea->signal_button_press_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_button)) );
    signals.push_back( drawArea->signal_button_release_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_button)) );
    signals.push_back( drawArea->signal_motion_notify_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_motion)) );
    signals.push_back( drawArea->signal_key_press_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_key)) );
    signals.push_back( drawArea->signal_key_release_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_key)) );
}

VRGtkWindow::~VRGtkWindow() {
    for (unsigned int i=0; i<signals.size(); i++) signals[i].disconnect();
    win = NULL;
}

VRGtkWindowPtr VRGtkWindow::ptr() { return static_pointer_cast<VRGtkWindow>( shared_from_this() ); }
VRGtkWindowPtr VRGtkWindow::create(Gtk::DrawingArea* da) { return shared_ptr<VRGtkWindow>(new VRGtkWindow(da) ); }

void VRGtkWindow::setCursor(string c) {
    Glib::RefPtr <Gdk::Window> win = drawArea->get_window();
    if (c == "") { win->set_cursor(); return; }

    Gdk::Cursor cursor;

    if (c == "X_CURSOR") cursor = Gdk::Cursor(Gdk::X_CURSOR);
    if (c == "ARROW") cursor = Gdk::Cursor(Gdk::ARROW);
    if (c == "BASED_ARROW_DOWN") cursor = Gdk::Cursor(Gdk::BASED_ARROW_DOWN);
    if (c == "BASED_ARROW_UP") cursor = Gdk::Cursor(Gdk::BASED_ARROW_UP);
    if (c == "BOAT") cursor = Gdk::Cursor(Gdk::BOAT);
    if (c == "BOGOSITY") cursor = Gdk::Cursor(Gdk::BOGOSITY);
    if (c == "BOTTOM_LEFT_CORNER") cursor = Gdk::Cursor(Gdk::BOTTOM_LEFT_CORNER);
    if (c == "BOTTOM_RIGHT_CORNER") cursor = Gdk::Cursor(Gdk::BOTTOM_RIGHT_CORNER);
    if (c == "BOTTOM_SIDE") cursor = Gdk::Cursor(Gdk::BOTTOM_SIDE);
    if (c == "BOTTOM_TEE") cursor = Gdk::Cursor(Gdk::BOTTOM_TEE);
    if (c == "BOX_SPIRAL") cursor = Gdk::Cursor(Gdk::BOX_SPIRAL);
    if (c == "CENTER_PTR") cursor = Gdk::Cursor(Gdk::CENTER_PTR);
    if (c == "CIRCLE") cursor = Gdk::Cursor(Gdk::CIRCLE);
    if (c == "CLOCK") cursor = Gdk::Cursor(Gdk::CLOCK);
    if (c == "COFFEE_MUG") cursor = Gdk::Cursor(Gdk::COFFEE_MUG);
    if (c == "CROSS") cursor = Gdk::Cursor(Gdk::CROSS);
    if (c == "CROSS_REVERSE") cursor = Gdk::Cursor(Gdk::CROSS_REVERSE);
    if (c == "CROSSHAIR") cursor = Gdk::Cursor(Gdk::CROSSHAIR);
    if (c == "DIAMOND_CROSS") cursor = Gdk::Cursor(Gdk::DIAMOND_CROSS);
    if (c == "DOT") cursor = Gdk::Cursor(Gdk::DOT);
    if (c == "DOTBOX") cursor = Gdk::Cursor(Gdk::DOTBOX);
    if (c == "DOUBLE_ARROW") cursor = Gdk::Cursor(Gdk::DOUBLE_ARROW);
    if (c == "DRAFT_LARGE") cursor = Gdk::Cursor(Gdk::DRAFT_LARGE);
    if (c == "DRAFT_SMALL") cursor = Gdk::Cursor(Gdk::DRAFT_SMALL);
    if (c == "DRAPED_BOX") cursor = Gdk::Cursor(Gdk::DRAPED_BOX);
    if (c == "EXCHANGE") cursor = Gdk::Cursor(Gdk::EXCHANGE);
    if (c == "FLEUR") cursor = Gdk::Cursor(Gdk::FLEUR);
    if (c == "GOBBLER") cursor = Gdk::Cursor(Gdk::GOBBLER);
    if (c == "GUMBY") cursor = Gdk::Cursor(Gdk::GUMBY);
    if (c == "HAND1") cursor = Gdk::Cursor(Gdk::HAND1);
    if (c == "HAND2") cursor = Gdk::Cursor(Gdk::HAND2);
    if (c == "HEART") cursor = Gdk::Cursor(Gdk::HEART);
    if (c == "ICON") cursor = Gdk::Cursor(Gdk::ICON);
    if (c == "IRON_CROSS") cursor = Gdk::Cursor(Gdk::IRON_CROSS);
    if (c == "LEFT_PTR") cursor = Gdk::Cursor(Gdk::LEFT_PTR);
    if (c == "LEFT_SIDE") cursor = Gdk::Cursor(Gdk::LEFT_SIDE);
    if (c == "LEFT_TEE") cursor = Gdk::Cursor(Gdk::LEFT_TEE);
    if (c == "LEFTBUTTON") cursor = Gdk::Cursor(Gdk::LEFTBUTTON);
    if (c == "LL_ANGLE") cursor = Gdk::Cursor(Gdk::LL_ANGLE);
    if (c == "LR_ANGLE") cursor = Gdk::Cursor(Gdk::LR_ANGLE);
    if (c == "MAN") cursor = Gdk::Cursor(Gdk::MAN);
    if (c == "MIDDLEBUTTON") cursor = Gdk::Cursor(Gdk::MIDDLEBUTTON);
    if (c == "MOUSE") cursor = Gdk::Cursor(Gdk::MOUSE);
    if (c == "PENCIL") cursor = Gdk::Cursor(Gdk::PENCIL);
    if (c == "PIRATE") cursor = Gdk::Cursor(Gdk::PIRATE);
    if (c == "PLUS") cursor = Gdk::Cursor(Gdk::PLUS);
    if (c == "QUESTION_ARROW") cursor = Gdk::Cursor(Gdk::QUESTION_ARROW);
    if (c == "RIGHT_PTR") cursor = Gdk::Cursor(Gdk::RIGHT_PTR);
    if (c == "RIGHT_SIDE") cursor = Gdk::Cursor(Gdk::RIGHT_SIDE);
    if (c == "RIGHT_TEE") cursor = Gdk::Cursor(Gdk::RIGHT_TEE);
    if (c == "RIGHTBUTTON") cursor = Gdk::Cursor(Gdk::RIGHTBUTTON);
    if (c == "RTL_LOGO") cursor = Gdk::Cursor(Gdk::RTL_LOGO);
    if (c == "SAILBOAT") cursor = Gdk::Cursor(Gdk::SAILBOAT);
    if (c == "SB_DOWN_ARROW") cursor = Gdk::Cursor(Gdk::SB_DOWN_ARROW);
    if (c == "SB_H_DOUBLE_ARROW") cursor = Gdk::Cursor(Gdk::SB_H_DOUBLE_ARROW);
    if (c == "SB_LEFT_ARROW") cursor = Gdk::Cursor(Gdk::SB_LEFT_ARROW);
    if (c == "SB_RIGHT_ARROW") cursor = Gdk::Cursor(Gdk::SB_RIGHT_ARROW);
    if (c == "SB_UP_ARROW") cursor = Gdk::Cursor(Gdk::SB_UP_ARROW);
    if (c == "SB_V_DOUBLE_ARROW") cursor = Gdk::Cursor(Gdk::SB_V_DOUBLE_ARROW);
    if (c == "SHUTTLE") cursor = Gdk::Cursor(Gdk::SHUTTLE);
    if (c == "SIZING") cursor = Gdk::Cursor(Gdk::SIZING);
    if (c == "SPIDER") cursor = Gdk::Cursor(Gdk::SPIDER);
    if (c == "SPRAYCAN") cursor = Gdk::Cursor(Gdk::SPRAYCAN);
    if (c == "STAR") cursor = Gdk::Cursor(Gdk::STAR);
    if (c == "TARGET") cursor = Gdk::Cursor(Gdk::TARGET);
    if (c == "TCROSS") cursor = Gdk::Cursor(Gdk::TCROSS);
    if (c == "TOP_LEFT_ARROW") cursor = Gdk::Cursor(Gdk::TOP_LEFT_ARROW);
    if (c == "TOP_LEFT_CORNER") cursor = Gdk::Cursor(Gdk::TOP_LEFT_CORNER);
    if (c == "TOP_RIGHT_CORNER") cursor = Gdk::Cursor(Gdk::TOP_RIGHT_CORNER);
    if (c == "TOP_SIDE") cursor = Gdk::Cursor(Gdk::TOP_SIDE);
    if (c == "TOP_TEE") cursor = Gdk::Cursor(Gdk::TOP_TEE);
    if (c == "TREK") cursor = Gdk::Cursor(Gdk::TREK);
    if (c == "UL_ANGLE") cursor = Gdk::Cursor(Gdk::UL_ANGLE);
    if (c == "UMBRELLA") cursor = Gdk::Cursor(Gdk::UMBRELLA);
    if (c == "UR_ANGLE") cursor = Gdk::Cursor(Gdk::UR_ANGLE);
    if (c == "WATCH") cursor = Gdk::Cursor(Gdk::WATCH);
    if (c == "XTERM") cursor = Gdk::Cursor(Gdk::XTERM);

    win->set_cursor(cursor);
}

bool VRGtkWindow::on_button(GdkEventButton * event) {
    gtk_widget_grab_focus((GtkWidget*)drawArea->gobj());
    int state = 1;
    if (event->type == GDK_BUTTON_PRESS) state = 0;

    if (getMouse() == 0) return false;
    getMouse()->mouse(event->button -1,state ,event->x ,event->y);

	/*printf("\nbutton: %i %i", event->button, event->type);
	printf("\n   x : %f", event->x);
	printf("\n   y : %f", event->y);
	printf("\n");*/
	return true;
}

bool VRGtkWindow::on_motion(GdkEventMotion * event) {
    //gtk_widget_grab_focus((GtkWidget*)drawArea->gobj());
    if (getMouse() == 0) return false;
    getMouse()->motion(event->x ,event->y);

	/*printf("\nevent: %i", event->type);
	printf("\n   x : %f", event->x);
	printf("\n   y : %f", event->y);
	printf("\n");*/
	return true;
}

bool VRGtkWindow::on_key(GdkEventKey *event) {
    if (event->keyval >= 65470 && event->keyval <= 65481) return false; //F keys
    //VRKeyboard::get()->keyboard(event->keyval, 0, 0); // TODO: check the values!!
	//printf("\n KEY: %i %i %i\n", event->keyval, event->type, event->state);
	//cout << "\n KEY: " << event->keyval << " " << event->type << " " << event->state << endl;

    if (getKeyboard() == 0) return false;
    getKeyboard()->setGtkEvent(event);
    getKeyboard()->keyboard(event->keyval, (event->type == 8), 0, 0);

	return true;
}

bool VRGtkWindow::on_scroll(GdkEventScroll * event) {
    int button = 3;
    if (event->direction == GDK_SCROLL_UP) button = 4;

    if (getMouse() == 0) return false;
    getMouse()->mouse(button, 0 ,event->x ,event->y);

    return true;
}

void VRGtkWindow::clear(Color3f c) {
    Glib::RefPtr<Gdk::Window> drawable = drawArea->get_window();
    if (drawable) {
        GdkGLContext* glcontext = gtk_widget_get_gl_context (widget);
        GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (widget);
        gdk_gl_drawable_gl_begin (gldrawable, glcontext);
        resize(widget->allocation.width, widget->allocation.height);
        glClearColor(c[0], c[1], c[2], 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        gdk_gl_drawable_swap_buffers (gldrawable);
        gdk_gl_drawable_gl_end (gldrawable);
    }
}

void VRGtkWindow::render(bool fromThread) {
    if (fromThread) return;
    PLock( VRGuiManager::get()->guiMutex() );
    if (!active || !content) return;
    auto profiler = VRProfiler::get();
    int pID = profiler->regStart("gtk window render");
    Glib::RefPtr<Gdk::Window> drawable = drawArea->get_window();
    if (drawable) {
        GdkGLContext* glcontext = gtk_widget_get_gl_context (widget);
        GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (widget);
        gdk_gl_drawable_gl_begin (gldrawable, glcontext);
        resize(widget->allocation.width, widget->allocation.height);
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        VRTimer t1; t1.start();
        if (active && content) win->render(ract);
        VRGlobals::RENDER_FRAME_RATE.update(t1);
        VRTimer t2; t2.start();
        gdk_gl_drawable_swap_buffers (gldrawable);
        VRGlobals::SWAPB_FRAME_RATE.update(t2);
        gdk_gl_drawable_gl_end (gldrawable);
    }
    profiler->regStop(pID);
}

void VRGtkWindow::on_resize(Gtk::Allocation& allocation) {
    initialExpose = true;
    resize(allocation.get_width(), allocation.get_height());
}

void VRGtkWindow::on_realize() {
    initialExpose = true;
    GdkGLContext* glcontext = gtk_widget_get_gl_context (widget);   // TODO: rare x error on startup!!
    GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (widget);
    gdk_gl_drawable_gl_begin(gldrawable, glcontext);
    win->init();
    win->resize(widget->allocation.width,widget->allocation.height);
    gdk_gl_drawable_gl_end (gldrawable);
}

void printGLversion() {
    //const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString (GL_VERSION); // version as a string
    cout << "Supported OpenGL version: " << version << endl;
}

bool VRGtkWindow::on_expose(GdkEventExpose* event) {
    if (initialExpose) {
        GdkGLContext* glcontext = gtk_widget_get_gl_context (widget);   // TODO: rare x error on startup!!
        GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (widget);
        gdk_gl_drawable_gl_begin(gldrawable, glcontext);
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        gdk_gl_drawable_swap_buffers (gldrawable);
        gdk_gl_drawable_gl_end (gldrawable);
        initialExpose = false;
    }
    return true;
}

void VRGtkWindow::save(XMLElementPtr node) { VRWindow::save(node); }
void VRGtkWindow::load(XMLElementPtr node) { VRWindow::load(node); }

OSG_END_NAMESPACE;
