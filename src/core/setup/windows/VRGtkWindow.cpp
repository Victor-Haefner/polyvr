#include "VRGtkWindow.h"
#include <gtkmm/drawingarea.h>
#include <gtkmm/window.h>
#include <gtkmm/label.h>
#include <gtkmm/builder.h>
#include <gdk/gdkgl.h>
#include <gtk/gtkgl.h>

#include "../devices/VRKeyboard.h"
#include "core/utils/VRTimer.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGtkWindow::VRGtkWindow(Gtk::DrawingArea* da) {
    type = 2;
    drawArea = da;
    widget = (GtkWidget*)drawArea->gobj();
    if(gtk_widget_get_realized(widget)) cout << "Warning: glarea is realized!\n";

    GdkGLConfig *glConfigMode = gdk_gl_config_new_by_mode((GdkGLConfigMode)(GDK_GL_MODE_RGB | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL));
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

    signals.push_back( drawArea->signal_scroll_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_scroll)) );
    signals.push_back( drawArea->signal_realize().connect(sigc::mem_fun(*this, &VRGtkWindow::on_realize)) );
    signals.push_back( drawArea->signal_expose_event().connect(sigc::mem_fun(*this, &VRGtkWindow::on_expose)) );
    signals.push_back( drawArea->signal_size_allocate().connect(sigc::mem_fun(*this, &VRGtkWindow::on_resize)) );
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

void VRGtkWindow::on_resize(Gtk::Allocation& allocation) {
    resize(allocation.get_width(), allocation.get_height());
}

bool VRGtkWindow::on_button(GdkEventButton * event) {
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
    gtk_widget_grab_focus((GtkWidget*)drawArea->gobj());
    //gtk_window_set_focus (window, widget);
    //widget->grab_focus();

    //if (event->type != GDK_BUTTON_PRESS) return true;
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

void VRGtkWindow::render() {
    if (!active || !content) return;
    Glib::RefPtr<Gdk::Window> drawable = drawArea->get_window();
    GdkRectangle rect; rect.x = 0; rect.y = 0; rect.width = 1; rect.height = 1;
    if (drawable) gdk_window_invalidate_rect( drawable->gobj(), &rect, false );
}

void VRGtkWindow::on_realize() {
    GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

    int w = widget->allocation.width;
    int h = widget->allocation.height;

    gdk_gl_drawable_gl_begin(gldrawable, glcontext);

    win->init();
    win->resize(w,h);

    gdk_gl_drawable_gl_end (gldrawable);
}

bool VRGtkWindow::on_expose(GdkEventExpose* event) {
    auto scene = VRSceneManager::getCurrent();
    if (scene) scene->allowScriptThreads();

    GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

    // check resize
    int w = widget->allocation.width;
    int h = widget->allocation.height;

    gdk_gl_drawable_gl_begin (gldrawable, glcontext);

    //const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
    //const GLubyte* version = glGetString (GL_VERSION); // version as a string
    //cout << "Renderer B " << endl;
    //cout << "OpenGL version supported " << version << endl;

    if (win->getWidth() != w || win->getHeight() != h) resize(w,h);

    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    if (active && content) win->render(ract);

    gdk_gl_drawable_swap_buffers (gldrawable);
    gdk_gl_drawable_gl_end (gldrawable);

    if (scene) scene->blockScriptThreads();

    return true;
}

void VRGtkWindow::save(xmlpp::Element* node) {
    VRWindow::save(node);
}

void VRGtkWindow::load(xmlpp::Element* node) {
    VRWindow::load(node);
}

OSG_END_NAMESPACE;
