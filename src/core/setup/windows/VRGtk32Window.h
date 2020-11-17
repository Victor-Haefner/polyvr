#include "core/gui/gtkglext/gtk/gtkgl.h"

#include "core/setup/windows/VRHeadMountedDisplay.h"

VRGtkWindow::VRGtkWindow(GtkDrawingArea* da, string msaa) {
    cout << " -= VRGtkWindow init =-" << endl;
    type = 2;
    drawArea = da;
    widget = (GtkWidget*)drawArea;
    if (gtk_widget_get_realized(widget)) cout << " --- !!! Warning: glarea is realized!\n";

    int MSAA = toInt(subString(msaa,1,-1));
    auto mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE | GDK_GL_MODE_STEREO);

    GdkGLConfig* glConfigMode = gdk_gl_config_new_by_mode(mode, MSAA);
    if (!glConfigMode) {
        mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL); // try without multisampling
        glConfigMode = gdk_gl_config_new_by_mode(mode, 0);
    }

    cout << "  glConfigMode: " << glConfigMode << endl;

    bool r = gtk_widget_set_gl_capability(widget,glConfigMode,NULL,true,GDK_GL_RGBA_TYPE);
    cout << "  gtk_widget_set_gl_capability: " << r << endl;

    gtk_widget_show_all(widget);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_VISIBILITY_NOTIFY_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_POINTER_MOTION_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_KEY_RELEASE_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_SCROLL_MASK);
    gtk_widget_set_can_focus(widget, true);

    win = PassiveWindow::create();
    _win = win;
    win->setSize(width, height);

    connect_signal<void>(drawArea, bind(&VRGtkWindow::on_realize, this), "realize");
    connect_signal<bool, CairoContext*>(drawArea, bind(&VRGtkWindow::on_expose, this, PL::_1), "draw");
    connect_signal<void, GdkRectangle*>(drawArea, bind(&VRGtkWindow::on_resize, this, PL::_1), "size_allocate");
    connect_signal<bool, GdkEventScroll*>(drawArea, bind(&VRGtkWindow::on_scroll, this, PL::_1), "scroll_event");
    connect_signal<bool, GdkEventButton*>(drawArea, bind(&VRGtkWindow::on_button, this, PL::_1), "button_press_event");
    connect_signal<bool, GdkEventButton*>(drawArea, bind(&VRGtkWindow::on_button, this, PL::_1), "button_release_event");
    connect_signal<bool, GdkEventMotion*>(drawArea, bind(&VRGtkWindow::on_motion, this, PL::_1), "motion_notify_event");
    connect_signal<bool, GdkEventKey*>(drawArea, bind(&VRGtkWindow::on_key, this, PL::_1), "key_press_event");
    connect_signal<bool, GdkEventKey*>(drawArea, bind(&VRGtkWindow::on_key, this, PL::_1), "key_release_event");
    cout << "  VRGtkWindow init done" << endl;

#ifndef WITHOUT_OPENVR
    if (VRHeadMountedDisplay::checkDeviceAttached())
        hmd = VRHeadMountedDisplay::create();
#endif
}

void VRGtkWindow::clear(Color3f c) {
    cout << "VRGtkWindow::clear with color " << c << endl;
    GdkWindow* drawable = gtk_widget_get_window(widget);
    //GdkWindow* drawable = widget->window;
    if (drawable) {
        gtk_widget_begin_gl(widget);
        GtkAllocation a;
        gtk_widget_get_allocation(widget, &a);
        resize(a.width, a.height);
        glClearColor(c[0], c[1], c[2], 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        gtk_widget_end_gl(widget, true);
    }
}

void VRGtkWindow::render(bool fromThread) {
    if (fromThread) return;
    PLock( VRGuiManager::get()->guiMutex() );
    if (!active || !content) return;
    auto profiler = VRProfiler::get();
    int pID = profiler->regStart("gtk window render");
    GdkWindow* drawable = gtk_widget_get_window(widget);
    if (drawable) {
        gtk_widget_begin_gl(widget);
#ifndef WITHOUT_OPENVR
        if (hmd) hmd->render();
#endif
        GtkAllocation a;
        gtk_widget_get_allocation(widget, &a);
        resize(a.width, a.height);
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        VRTimer t1; t1.start();
        if (active && content) win->render(ract);
        VRGlobals::RENDER_FRAME_RATE.update(t1);
        VRTimer t2; t2.start();
        gtk_widget_end_gl(widget, true);
        VRGlobals::SWAPB_FRAME_RATE.update(t2);
    }
    profiler->regStop(pID);
}

void VRGtkWindow::on_realize() {
    cout << "VRGtkWindow::on_realize, init OSG window" << endl;
    initialExpose = true;
    gtk_widget_begin_gl(widget);
#ifndef WITHOUT_OPENVR
    if (hmd) hmd->initHMD();
#endif
    win->init();
    GtkAllocation a;
    gtk_widget_get_allocation(widget, &a);
    cout << " on realize resize to " << a.width << " x " << a.height << endl;
    resize(a.width, a.height);
    gtk_widget_end_gl(widget, true);
}

bool VRGtkWindow::on_expose(CairoContext* event) {
    if (initialExpose) {
        gtk_widget_begin_gl(widget);
        GtkAllocation a;
        gtk_widget_get_allocation(widget, &a);
        cout << " on initial expose resize to " << a.width << " x " << a.height << endl;
        resize(a.width, a.height);
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        gtk_widget_end_gl(widget, true);
        initialExpose = false;
    }
    return true;
}
