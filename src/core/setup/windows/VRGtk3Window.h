

#include "core/setup/windows/VRHeadMountedDisplay.h"
#include "core/gui/glarea/glarea.h"

GdkGLContext* onCreateGLContext(GLArea* area, gpointer user_data) {
    GdkWindow* window = gtk_widget_get_window(GTK_WIDGET(area));
    GdkGLContext* context = gdk_window_create_gl_context(window, NULL);
    cout << "onCreateGLContext " << context << endl;
    return context;
}

VRGtkWindow::VRGtkWindow(GtkDrawingArea* da, string msaa) {
    cout << " --------------------- VRGtkWindow::VRGtkWindow -------------- " << endl;
    type = 2;

    g_setenv("GDK_GL_LEGACY", "1", true); // windows

    widget = GTK_WIDGET(da);
    if (gtk_widget_get_realized(widget)) cout << "Warning: glarea is realized!\n";

    /*int MSAA = toInt(subString(msaa,1,-1));
    auto mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE | GDK_GL_MODE_STEREO);
    GdkGLConfig* glConfigMode = gdk_gl_config_new_by_mode(mode, MSAA);
    gtk_widget_set_gl_capability(widget,glConfigMode,NULL,true,GDK_GL_RGBA_TYPE);*/

    gl_area_set_auto_render((GLArea*)widget, false);
    gl_area_set_has_alpha((GLArea*)widget, false); // with alpha, transparent materials induce artifacts
    gl_area_set_has_depth_buffer((GLArea*)widget, true);
    gl_area_set_has_stencil_buffer((GLArea*)widget, true);
    //gl_area_set_required_version((GLArea*)widget, 4,4);
    gl_area_set_use_es((GLArea*)widget, false);

    //g_signal_connect(widget, "create-context", (GCallback)onCreateGLContext, NULL);

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

    connect_signal<void>(widget, bind(&VRGtkWindow::on_realize, this), "realize");
    connect_signal<bool, GdkGLContext*>(widget, bind(&VRGtkWindow::on_render , this, PL::_1), "render" );

    connect_signal<void, GdkRectangle*>(widget, bind(&VRGtkWindow::on_resize, this, PL::_1), "size_allocate");
    connect_signal<void, GdkEventScroll*>(widget, bind(&VRGtkWindow::on_scroll, this, PL::_1), "scroll_event");
    connect_signal<void, GdkEventButton*>(widget, bind(&VRGtkWindow::on_button, this, PL::_1), "button_press_event");
    connect_signal<void, GdkEventButton*>(widget, bind(&VRGtkWindow::on_button, this, PL::_1), "button_release_event");
    connect_signal<void, GdkEventMotion*>(widget, bind(&VRGtkWindow::on_motion, this, PL::_1), "motion_notify_event");
    connect_signal<void, GdkEventKey*>(widget, bind(&VRGtkWindow::on_key, this, PL::_1), "key_press_event");
    connect_signal<void, GdkEventKey*>(widget, bind(&VRGtkWindow::on_key, this, PL::_1), "key_release_event");

#ifndef WITHOUT_OPENVR
    if (VRHeadMountedDisplay::checkDeviceAttached())
        hmd = VRHeadMountedDisplay::create();
#endif
}

void VRGtkWindow::clear(Color3f c) {
    /*GdkWindow* drawable = widget->window;
    if (drawable) {
        GdkGLContext* glcontext = gtk_widget_get_gl_context (widget);
        GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (widget);
        gdk_gl_drawable_gl_begin (gldrawable, glcontext);
        resize(widget->allocation.width, widget->allocation.height);
        glClearColor(c[0], c[1], c[2], 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        gdk_gl_drawable_swap_buffers (gldrawable);
        gdk_gl_drawable_gl_end (gldrawable);
    }*/
}

void VRGtkWindow::render(bool fromThread) {
    if (fromThread) return;
    PLock( VRGuiManager::get()->guiMutex() );
    if (!active || !content || !isRealized) return;

    /*GtkAllocation a;
    gtk_widget_get_allocation(widget, &a);
    resize(a.width, a.height);*/

    gl_area_queue_render((GLArea*)widget);
}

typedef void (__stdcall PFNGLBINDFRAMEBUFFER) (GLuint program);
PFNGLBINDFRAMEBUFFER* glBindFramebuffer;

void checkMultiSampling() { // multisampling has to be defined on frame buffer creation
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFER*)wglGetProcAddress("glBindFramebuffer");

    (*glBindFramebuffer)(0);

    GLint hasMultisampling, Nsamples;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &hasMultisampling);
    glGetIntegerv(GL_SAMPLES, &Nsamples);
    cout << "VRGtkWindow::on_render " << hasMultisampling << " " << Nsamples << endl;

    (*glBindFramebuffer)(1);
}

bool VRGtkWindow::on_render(GdkGLContext* glcontext) {
    auto profiler = VRProfiler::get();
    int pID = profiler->regStart("gtk window render");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);

    //checkMultiSampling();

    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    VRTimer t1; t1.start();
    if (active && content) {
#ifndef WITHOUT_OPENVR
        if (hmd) hmd->render();
#endif
        win->render(ract);
    }
    VRGlobals::RENDER_FRAME_RATE.update(t1);

    //VRTimer t2; t2.start();
    //gdk_gl_drawable_swap_buffers (gldrawable);
    //VRGlobals::SWAPB_FRAME_RATE.update(t2);

    glFlush();

    profiler->regStop(pID);
    return true;
}

void VRGtkWindow::on_realize() {
    cout << " --------------------- VRGtkWindow::on_realize -------------- " << endl;
    initialExpose = true;
    gl_area_make_current(GL_AREA(widget));
    if (gl_area_get_error(GL_AREA(widget)) != NULL) {
        printf("VRGtkWindow::on_realize - failed to initialize buffers\n");
        return;
    }
    win->init();
#ifndef WITHOUT_OPENVR
    if (hmd) hmd->initHMD();
#endif
    GtkAllocation a;
    gtk_widget_get_allocation(widget, &a);
    resize(a.width, a.height);
    isRealized = true;

    GdkGLContext* context = gl_area_get_context((GLArea*)widget);
    cout << "gdk_gl_context_is_legacy: " << gdk_gl_context_is_legacy(context) << endl;
    return;
}


