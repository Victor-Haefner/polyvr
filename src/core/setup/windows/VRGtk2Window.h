#include <gtk/gtkdrawingarea.h>
#include <gdk/gdkgl.h>
#include <gdk/gdkglconfig.h>
#include <gtk/gtkgl.h>

static GdkGLConfig* gdk_gl_config_new_rgb(GdkScreen* screen, GdkGLConfigMode  mode, int msaa) {
  int list[32];
  int n = 0;

  list[n++] = GDK_GL_RGBA;
  list[n++] = GDK_GL_RED_SIZE;
  list[n++] = 1;
  list[n++] = GDK_GL_GREEN_SIZE;
  list[n++] = 1;
  list[n++] = GDK_GL_BLUE_SIZE;
  list[n++] = 1;
  if (mode & GDK_GL_MODE_ALPHA)
    {
      list[n++] = GDK_GL_ALPHA_SIZE;
      list[n++] = 1;
    }
  if (mode & GDK_GL_MODE_DOUBLE) list[n++] = GDK_GL_DOUBLEBUFFER;
  if (mode & GDK_GL_MODE_STEREO) list[n++] = GDK_GL_STEREO;
  if (mode & GDK_GL_MODE_DEPTH) {
      list[n++] = GDK_GL_DEPTH_SIZE;
      list[n++] = 1;
    }
  if (mode & GDK_GL_MODE_STENCIL) {
      list[n++] = GDK_GL_STENCIL_SIZE;
      list[n++] = 1;
    }
  if (mode & GDK_GL_MODE_ACCUM) {
      list[n++] = GDK_GL_ACCUM_RED_SIZE;
      list[n++] = 1;
      list[n++] = GDK_GL_ACCUM_GREEN_SIZE;
      list[n++] = 1;
      list[n++] = GDK_GL_ACCUM_BLUE_SIZE;
      list[n++] = 1;
      if (mode & GDK_GL_MODE_ALPHA) {
          list[n++] = GDK_GL_ACCUM_ALPHA_SIZE;
          list[n++] = 1;
        }
    }
   if (mode & GDK_GL_MODE_MULTISAMPLE && msaa >= 2) {
       list[n++] = GDK_GL_SAMPLE_BUFFERS;
       list[n++] = 1;
       list[n++] = GDK_GL_SAMPLES;
       list[n++] = msaa; // FSAA // 2x 4x 16x
    }
  list[n] = GDK_GL_ATTRIB_LIST_NONE;

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  return gdk_gl_config_new_for_screen(screen, list);
#else
  return gdk_gl_config_new (list);
#endif
}

GdkGLConfig* gdk_gl_config_new_by_mode (GdkGLConfigMode mode, int msaa) {
#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  GdkScreen* screen = gdk_screen_get_default ();
#else
  GdkScreen* screen = NULL;
#endif
  return gdk_gl_config_new_rgb(screen, mode, msaa);
}

VRGtkWindow::VRGtkWindow(GtkDrawingArea* da, string msaa) {
    type = 2;
    drawArea = da;
    widget = (GtkWidget*)drawArea;
    if (gtk_widget_get_realized(widget)) cout << "Warning: glarea is realized!\n";


    int MSAA = toInt(subString(msaa,1,-1));
    auto mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE | GDK_GL_MODE_STEREO);
    GdkGLConfig* glConfigMode = gdk_gl_config_new_by_mode(mode, MSAA);
    gtk_widget_set_gl_capability(widget,glConfigMode,NULL,true,GDK_GL_RGBA_TYPE);

    gtk_widget_show(widget);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_VISIBILITY_NOTIFY_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_POINTER_MOTION_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_KEY_RELEASE_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_SCROLL_MASK);
    GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);

    win = PassiveWindow::create();
    _win = win;
    win->setSize(width, height);

    connect_signal<void>(drawArea, bind(&VRGtkWindow::on_realize, this), "realize");
    connect_signal<void, GdkEventExpose*>(drawArea, bind(&VRGtkWindow::on_expose, this, PL::_1), "expose_event");
    connect_signal<void, GdkRectangle*>(drawArea, bind(&VRGtkWindow::on_resize, this, PL::_1), "size_allocate");
    connect_signal<void, GdkEventScroll*>(drawArea, bind(&VRGtkWindow::on_scroll, this, PL::_1), "scroll_event");
    connect_signal<void, GdkEventButton*>(drawArea, bind(&VRGtkWindow::on_button, this, PL::_1), "button_press_event");
    connect_signal<void, GdkEventButton*>(drawArea, bind(&VRGtkWindow::on_button, this, PL::_1), "button_release_event");
    connect_signal<void, GdkEventMotion*>(drawArea, bind(&VRGtkWindow::on_motion, this, PL::_1), "motion_notify_event");
    connect_signal<void, GdkEventKey*>(drawArea, bind(&VRGtkWindow::on_key, this, PL::_1), "key_press_event");
    connect_signal<void, GdkEventKey*>(drawArea, bind(&VRGtkWindow::on_key, this, PL::_1), "key_release_event");
}

void VRGtkWindow::clear(Color3f c) {
    GdkWindow* drawable = widget->window;
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
    GdkWindow* drawable = widget->window;
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

void VRGtkWindow::on_realize() {
    initialExpose = true;
    GdkGLContext* glcontext = gtk_widget_get_gl_context (widget);   // TODO: rare x error on startup!!
    GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (widget);
    gdk_gl_drawable_gl_begin(gldrawable, glcontext);
    win->init();
    resize(widget->allocation.width,widget->allocation.height);
    gdk_gl_drawable_gl_end (gldrawable);
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
