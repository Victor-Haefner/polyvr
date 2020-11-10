
VRGtkWindow::VRGtkWindow(GtkDrawingArea* da, string msaa) {
    cout << " --------------------- VRGtkWindow::VRGtkWindow -------------- " << endl;
    type = 2;

    GtkWidget* box = gtk_widget_get_parent((GtkWidget*)da);
    gtk_container_remove(GTK_CONTAINER(box), (GtkWidget*)da);

    widget = gtk_gl_area_new();
    gtk_box_pack_start(GTK_BOX(box), widget, true, true, 0);
    if (gtk_widget_get_realized(widget)) cout << "Warning: glarea is realized!\n";

    /*int MSAA = toInt(subString(msaa,1,-1));
    auto mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE | GDK_GL_MODE_STEREO);
    GdkGLConfig* glConfigMode = gdk_gl_config_new_by_mode(mode, MSAA);
    gtk_widget_set_gl_capability(widget,glConfigMode,NULL,true,GDK_GL_RGBA_TYPE);*/

    gtk_gl_area_set_auto_render((GtkGLArea*)widget, false);
    gtk_gl_area_set_has_alpha((GtkGLArea*)widget, true);
    gtk_gl_area_set_has_depth_buffer((GtkGLArea*)widget, true);
    gtk_gl_area_set_has_stencil_buffer((GtkGLArea*)widget, true);
    gtk_gl_area_set_required_version((GtkGLArea*)widget, 4,4);

    gtk_widget_show_all(widget);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_VISIBILITY_NOTIFY_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_POINTER_MOTION_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_KEY_RELEASE_MASK);
    gtk_widget_add_events(widget, (GdkEventMask)GDK_SCROLL_MASK);
    //GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);

    win = PassiveWindow::create();
    _win = win;
    win->setSize(width, height);

    connect_signal<void>(widget, bind(&VRGtkWindow::on_realize, this), "realize");
    connect_signal<bool, GdkGLContext*>(widget, bind(&VRGtkWindow::on_render , this, PL::_1), "render" );

    /*connect_signal<void, GdkRectangle*>(drawArea, bind(&VRGtkWindow::on_resize, this, PL::_1), "size_allocate");
    connect_signal<void, GdkEventScroll*>(drawArea, bind(&VRGtkWindow::on_scroll, this, PL::_1), "scroll_event");
    connect_signal<void, GdkEventButton*>(drawArea, bind(&VRGtkWindow::on_button, this, PL::_1), "button_press_event");
    connect_signal<void, GdkEventButton*>(drawArea, bind(&VRGtkWindow::on_button, this, PL::_1), "button_release_event");
    connect_signal<void, GdkEventMotion*>(drawArea, bind(&VRGtkWindow::on_motion, this, PL::_1), "motion_notify_event");
    connect_signal<void, GdkEventKey*>(drawArea, bind(&VRGtkWindow::on_key, this, PL::_1), "key_press_event");
    connect_signal<void, GdkEventKey*>(drawArea, bind(&VRGtkWindow::on_key, this, PL::_1), "key_release_event");*/
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
    gtk_gl_area_queue_render((GtkGLArea*)widget);
    VRGuiManager::get()->updateGtk();
    return;


    if (fromThread) return;
    PLock( VRGuiManager::get()->guiMutex() );
    if (!active || !content || !isRealized) return;
    cout << " -------------------------- VRGtkWindow::render " << endl;
    auto profiler = VRProfiler::get();
    int pID = profiler->regStart("gtk window render");
    //GdkWindow* drawable = widget->window;
    //if (drawable) {
        GdkGLContext* glcontext = gtk_gl_area_get_context((GtkGLArea*)widget);
        gtk_gl_area_make_current((GtkGLArea*)widget);
        gtk_gl_area_attach_buffers((GtkGLArea*)widget);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        //GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (widget);
        //gdk_gl_drawable_gl_begin (gldrawable, glcontext);
        GtkAllocation a;
        gtk_widget_get_allocation(widget, &a);
        resize(a.width, a.height);

        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        VRTimer t1; t1.start();
        if (active && content) win->render(ract);
        VRGlobals::RENDER_FRAME_RATE.update(t1);
        VRTimer t2; t2.start();
        //gdk_gl_drawable_swap_buffers (gldrawable);
        VRGlobals::SWAPB_FRAME_RATE.update(t2);
        //gdk_gl_drawable_gl_end (gldrawable);
    //}
    profiler->regStop(pID);
}

bool VRGtkWindow::on_render(GdkGLContext* glcontext) {
    //cout << " --------------------- VRGtkWindow::on_render -------------- " << endl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    //glClearColor(0.2, 0.2, 0.2, 1.0);
    glClearColor(0.2, 0.2, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    win->render(ract);

    glFlush();
    return true;
}

void VRGtkWindow::on_realize() {
    cout << " --------------------- VRGtkWindow::on_realize -------------- " << endl;
    initialExpose = true;
    gtk_gl_area_make_current(GTK_GL_AREA(widget));
    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != NULL) {
        printf("VRGtkWindow::on_realize - failed to initialize buffers\n");
        return;
    }
    win->init();
    GtkAllocation a;
    gtk_widget_get_allocation(widget, &a);
    resize(a.width, a.height);
    isRealized = true;
    return;
}


