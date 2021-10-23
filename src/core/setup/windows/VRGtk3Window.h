

#include "core/setup/windows/VRHeadMountedDisplay.h"
#include "core/gui/glarea/glarea.h"
#include "core/gui/VRGuiBuilder.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/OSGObject.h"
#include "core/utils/system/VRSystem.h"

#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGRenderBuffer.h>

GdkGLContext* onCreateGLContext(GLArea* area, gpointer user_data) {
    GdkWindow* window = gtk_widget_get_window(GTK_WIDGET(area));
    GdkGLContext* context = gdk_window_create_gl_context(window, NULL);
    cout << "onCreateGLContext " << context << endl;
    return context;
}

VRGtkWindow::VRGtkWindow(GtkWidget* area, string msaa) {
    cout << " --------------------- VRGtkWindow::VRGtkWindow -------------- " << endl;
    type = 2;

    g_setenv("GDK_GL_LEGACY", "1", true); // windows

    widget = area;
    if (gtk_widget_get_realized(widget)) cout << "Warning: glarea is realized!\n";

    //int MSAA = toInt(subString(msaa, 1, -1));
    //gl_area_set_samples((GLArea*)widget, MSAA); // TODO: pass the samples somehow

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

    connect_signal<void, int, int>(widget, bind(&VRGtkWindow::on_resize, this, PL::_1, PL::_2), "resize");
    connect_signal<void, GdkEventScroll*>(widget, bind(&VRGtkWindow::on_scroll, this, PL::_1), "scroll_event");
    connect_signal<void, GdkEventButton*>(widget, bind(&VRGtkWindow::on_button, this, PL::_1), "button_press_event");
    connect_signal<void, GdkEventButton*>(widget, bind(&VRGtkWindow::on_button, this, PL::_1), "button_release_event");
    connect_signal<void, GdkEventMotion*>(widget, bind(&VRGtkWindow::on_motion, this, PL::_1), "motion_notify_event");
    connect_signal<void, GdkEventKey*>(widget, bind(&VRGtkWindow::on_key, this, PL::_1), "key_press_event");
    connect_signal<void, GdkEventKey*>(widget, bind(&VRGtkWindow::on_key, this, PL::_1), "key_release_event");

    gl_area_trigger_resize(GL_AREA(widget));

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

Vec2i VRGtkWindow::rebaseMousePosition(int x, int y) {
    auto clipping = gl_area_get_clipping(GL_AREA(widget));
    return Vec2i(clipping.x+x, clipping.y+y);
}

void VRGtkWindow::render(bool fromThread) {
    if (fromThread) return;
    VRLock lock( VRGuiManager::get()->guiMutex() );
    if (!active || !content || !isRealized) return;
    gl_area_queue_render((GLArea*)widget);
}

bool VRGtkWindow::on_render(GdkGLContext* glcontext) {
    auto profiler = VRProfiler::get();
    int pID = profiler->regStart("gtk window render");

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_ALPHA_TEST);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.2, 0.2, 0.2, 1.0);

    VRTimer t1; t1.start();
    if (active && content) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef WITHOUT_OPENVR
        if (hmd) hmd->render();
#endif
        //cout << "   VRGtkWindow::on_render win" << endl;


        /*if (VRScene::getCurrent()) {
            auto box = VRScene::getCurrent()->get("Rollen");
            if (box) {
                Matrix p = box->getNode()->node->getToWorld();
                //Vec3d p = dynamic_pointer_cast<VRTransform>(box)->getWorldPosition();
                //if (abs(p[3][1]-1.0) > 1e-6)
                    cout << VRGlobals::CURRENT_FRAME << " doRender " << p[3][1] << endl;
            }
        }
        printBacktrace();*/

        VRGlobals::GTK_LAST_RENDER = VRGlobals::CURRENT_FRAME;
        win->render(ract);
        //cout << "   VRGtkWindow::on_render win done" << endl;
    } else {
        auto clipping = gl_area_get_clipping(GL_AREA(widget));
        double Y = clipping.H-clipping.y-clipping.h;

        glEnable(GL_SCISSOR_TEST);
        glScissor(clipping.x, Y, clipping.w, clipping.h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
    }
    VRGlobals::RENDER_FRAME_RATE.update(t1);

    //VRTimer t2; t2.start();
    //gdk_gl_drawable_swap_buffers (gldrawable);
    //VRGlobals::SWAPB_FRAME_RATE.update(t2);

    glFlush();

    profiler->regStop(pID);
    return true;
}

string typeToString(GLenum type) {
    if (type == GL_DEBUG_TYPE_ERROR) return "GL_DEBUG_TYPE_ERROR";
    if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
    if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR) return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
    if (type == GL_DEBUG_TYPE_PORTABILITY) return "GL_DEBUG_TYPE_PORTABILITY";
    if (type == GL_DEBUG_TYPE_PERFORMANCE) return "GL_DEBUG_TYPE_PERFORMANCE";
    if (type == GL_DEBUG_TYPE_MARKER) return "GL_DEBUG_TYPE_MARKER";
    if (type == GL_DEBUG_TYPE_PUSH_GROUP) return "GL_DEBUG_TYPE_PUSH_GROUP";
    if (type == GL_DEBUG_TYPE_POP_GROUP) return "GL_DEBUG_TYPE_POP_GROUP";
    if (type == GL_DEBUG_TYPE_OTHER) return "GL_DEBUG_TYPE_OTHER";
    return "";
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
        fprintf( stderr, "GL CALLBACK: type = %s, severity = 0x%x, message = %s\n",
                typeToString(type).c_str(), severity, message );
}

void VRGtkWindow::on_realize() {
    cout << " --------------------- VRGtkWindow::on_realize -------------- " << endl;
    initialExpose = true;
    gl_area_make_current(GL_AREA(widget));
    if (gl_area_get_error(GL_AREA(widget)) != NULL) {
        printf("VRGtkWindow::on_realize - failed to initialize buffers\n");
        return;
    }

    //glEnable              ( GL_DEBUG_OUTPUT );
    //glDebugMessageCallback( MessageCallback, 0 );

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


