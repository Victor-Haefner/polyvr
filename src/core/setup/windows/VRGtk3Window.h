
#include "core/setup/windows/VRHeadMountedDisplay.h"

#include <GL/GLU.h>

VRGtkWindow::VRGtkWindow(GtkDrawingArea* da, string msaa) {
    cout << " --------------------- VRGtkWindow::VRGtkWindow -------------- " << endl;
    type = 2;

    g_setenv("GDK_GL_LEGACY", "1", true);

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
    gtk_gl_area_set_use_es((GtkGLArea*)widget, false);

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
    gtk_gl_area_queue_render((GtkGLArea*)widget);
    VRGuiManager::get()->updateGtk();

#ifndef WITHOUT_OPENVR
    if (hmd) hmd->render();
#endif
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

// ------------------------------ texture
#define RED 255,0,0
#define GRE 0,255,0
#define BLU 0,0,255

int texSize = 4;
vector<unsigned char> image;
unsigned int textureID;
// ------------------------------

GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };  /* Red diffuse light. */
GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };  /* Infinite light location. */
GLfloat n[6][3] = {  /* Normals for the 6 faces of a cube. */
  {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
  {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0} };
GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
  {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
  {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };
GLfloat v[8][3];  /* Will be filled in with X,Y,Z vertexes. */

void initBoxTest(void) {
    /* Setup cube vertex data. */
    v[0][0] = v[1][0] = v[2][0] = v[3][0] = -1;
    v[4][0] = v[5][0] = v[6][0] = v[7][0] = 1;
    v[0][1] = v[1][1] = v[4][1] = v[5][1] = -1;
    v[2][1] = v[3][1] = v[6][1] = v[7][1] = 1;
    v[0][2] = v[3][2] = v[4][2] = v[7][2] = 1;
    v[1][2] = v[2][2] = v[5][2] = v[6][2] = -1;

    /* Enable a single OpenGL light. */
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    /* Use depth buffering for hidden surface elimination. */
    glEnable(GL_DEPTH_TEST);

    /* Setup the view of the cube. */
    glMatrixMode(GL_PROJECTION);
    gluPerspective( /* field of view in degree */ 40.0,
        /* aspect ratio */ 1.0,
        /* Z near */ 1.0, /* Z far */ 10.0);
    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
        0.0, 0.0, 0.0,      /* center is at (0,0,0) */
        0.0, 1.0, 0.);      /* up is in positive Y direction */

      /* Adjust cube position to be asthetic angle. */
    glTranslatef(0.0, 0.0, -1.0);
    glRotatef(60, 1.0, 0.0, 0.0);
    glRotatef(-20, 0.0, 0.0, 1.0);

    // texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    image = {
        BLU, GRE, RED, GRE,
        GRE, RED, GRE, RED,
        RED, GRE, RED, GRE,
        GRE, RED, GRE, RED,
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);
}

void drawBox(void) {
    int i;
    glBindTexture(GL_TEXTURE_2D, textureID);
    glEnable(GL_TEXTURE_2D);

    for (i = 0; i < 6; i++) {
        glBegin(GL_QUADS);
        glNormal3fv(&n[i][0]);
        glTexCoord2f(0.0, 0.0);  glVertex3fv(&v[faces[i][0]][0]);
        glTexCoord2f(1.0, 0.0);  glVertex3fv(&v[faces[i][1]][0]);
        glTexCoord2f(1.0, 1.0);  glVertex3fv(&v[faces[i][2]][0]);
        glTexCoord2f(0.0, 1.0);  glVertex3fv(&v[faces[i][3]][0]);
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
}

bool VRGtkWindow::on_render(GdkGLContext* glcontext) {
    //cout << " --------------------- VRGtkWindow::on_render -------------- " << endl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    win->render(ract);
    //drawBox();

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
#ifndef WITHOUT_OPENVR
    if (hmd) hmd->initHMD();
#endif
    initBoxTest();
    GtkAllocation a;
    gtk_widget_get_allocation(widget, &a);
    resize(a.width, a.height);
    isRealized = true;

    GdkGLContext* context = gtk_gl_area_get_context((GtkGLArea*)widget);
    cout << "gdk_gl_context_is_legacy: " << gdk_gl_context_is_legacy(context) << endl;
    return;
}


