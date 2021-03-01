#define GTK_COMPILATION
#define GDK_COMPILATION

#ifdef _WIN32
#include <gdk/win32/gdkwin32glcontext.h>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <epoxy/wgl.h>
#else
#include <gdk/x11/gdkx11glcontext.h>
#endif

#include "glarea.h"
#include <gtk/gtkstylecontext.h>
#include <gtk/gtkrender.h>
#include <gobject/gtype.h>
#include <gobject/gvalue.h>
#include <gobject/gsignal.h>

gboolean global_invalidate = TRUE;

#include "glareaCommon.h"

#ifndef _WIN32
#include "glareaLinux.h"
#else
#include "glareaWin.h"
#endif


void override_window_invalidate_for_new_frame(_GdkWindow* window) {
    _GdkWindowImplClass* impl_class = getGdkWindowImplClass();
#ifdef _WIN32
    impl_class->invalidate_for_new_frame = _gdk_win32_window_invalidate_for_new_frame;
    //impl_class->set_opacity = _gdk_win32_window_set_opacity;
#else
    impl_class->invalidate_for_new_frame = _gdk_x11_window_invalidate_for_new_frame;
#endif
    g_type_class_unref(impl_class);
}

typedef struct {
    GdkGLContext* context;
    GdkWindow *event_window;
    GError *error;

    GLClipping clipping;
    gboolean needs_resize;
    gboolean needs_render;
} GLAreaPrivate;

enum {
    PROP_0,

    PROP_CONTEXT,
    PROP_HAS_ALPHA,
    PROP_HAS_DEPTH_BUFFER,
    PROP_HAS_STENCIL_BUFFER,
    PROP_USE_ES,

    PROP_AUTO_RENDER,

    LAST_PROP
};

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

enum {
  RENDER,
  RESIZE,
  LAST_SIGNAL
};

static guint area_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (GLArea, gl_area, GTK_TYPE_WIDGET)

static void gl_area_get_property(GObject* gobject, guint prop_id, GValue* value, GParamSpec* pspec) {}
static void gl_area_set_property (GObject* gobject, guint prop_id, const GValue* value, GParamSpec* pspec) {}

static void gl_area_dispose(GObject *gobject) {
    GLArea *area = GL_AREA (gobject);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_clear_object (&priv->context);
    G_OBJECT_CLASS (gl_area_parent_class)->dispose (gobject);
}

#ifndef _WIN32
gint epoxy_glx_version(Display*, int);
guint epoxy_has_glx_extension(Display*, int, const char*);

gboolean gdk_x11_screen_init_gl (GdkScreen *screen) {
    GdkDisplay *display = gdk_screen_get_display (screen);
    _GdkX11Display *display_x11 = (_GdkX11Display*)display;
    int error_base, event_base;

    if (display_x11->have_glx) return TRUE;
    Display* dpy = display_x11->xdisplay;
    if (!glXQueryExtension (dpy, &error_base, &event_base)) return FALSE;
    int screen_num = ((_GdkX11Screen*)screen)->screen_num;

    display_x11->have_glx = TRUE;
    display_x11->glx_version = epoxy_glx_version (dpy, screen_num);
    display_x11->glx_error_base = error_base;
    display_x11->glx_event_base = event_base;

    display_x11->has_glx_create_context = epoxy_has_glx_extension (dpy, screen_num, "GLX_ARB_create_context_profile");
    display_x11->has_glx_create_es2_context = epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_create_context_es2_profile");
    display_x11->has_glx_swap_interval = epoxy_has_glx_extension (dpy, screen_num, "GLX_SGI_swap_control");
    display_x11->has_glx_texture_from_pixmap = epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_texture_from_pixmap");
    display_x11->has_glx_video_sync = epoxy_has_glx_extension (dpy, screen_num, "GLX_SGI_video_sync");
    display_x11->has_glx_buffer_age = epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_buffer_age");
    display_x11->has_glx_sync_control = epoxy_has_glx_extension (dpy, screen_num, "GLX_OML_sync_control");
    display_x11->has_glx_multisample = epoxy_has_glx_extension (dpy, screen_num, "GLX_ARB_multisample");
    display_x11->has_glx_visual_rating = epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_visual_rating");

    return TRUE;
}

static gboolean find_fbconfig_for_visual (GdkDisplay* display, _GdkVisual* visual, GLXFBConfig* fb_config_out, gboolean full, GError** error) {
    _GdkX11Display *display_x11 = (_GdkX11Display*)display;
    Display* dpy = display_x11->xdisplay;
    GLXFBConfig *configs;
    int n_configs, i;
    //gboolean use_rgba;
    gboolean retval = FALSE;

    _GdkX11Visual *visual_x11 = (_GdkX11Visual*)visual;
    VisualID xvisual_id = XVisualIDFromVisual(visual_x11->xvisual);

    static int attrs[] = { // those do not matter as we only want to find the config with ID xvisual_id
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_DOUBLEBUFFER    , GL_TRUE,
      GLX_RED_SIZE        , 1,
      GLX_GREEN_SIZE      , 1,
      GLX_BLUE_SIZE       , 1,
      GLX_ALPHA_SIZE      , GLX_DONT_CARE,
      None
    };
    configs = glXChooseFBConfig (dpy, DefaultScreen (dpy), attrs, &n_configs);
    printf(" creating context with minimum specs, found %i configs\n", n_configs);

    if (configs == NULL || n_configs == 0) {
      printf("AAAAAAA, glXChooseFBConfig failed!\n");
      g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_UNSUPPORTED_FORMAT, "No available configurations for the given pixel format");
      return FALSE;
    }

    for (i = 0; i < n_configs; i++)  {
      XVisualInfo *visinfo;

      visinfo = glXGetVisualFromFBConfig (dpy, configs[i]);
      if (visinfo == NULL) continue;


      //printf(" search visualid %i =? %i\n", visinfo->visualid, xvisual_id);
      if (visinfo->visualid != xvisual_id) {
          XFree (visinfo);
          continue;
      }

      if (fb_config_out != NULL) *fb_config_out = configs[i];

      XFree (visinfo);
      retval = TRUE;
      goto out;
    }

    printf("Error, find_fbconfig_for_visual failed? tried to find %i\n", xvisual_id);
    //g_set_error (error, GDK_GL_ERROR, GDK_GL_ERROR_UNSUPPORTED_FORMAT, "No available configurations for the given RGBA pixel format");

out:
    XFree (configs);
    return retval;
}

GdkGLContext* x11_window_create_gl_context(GdkWindow* window, gboolean attached, GdkGLContext *share, gboolean full, GError**error) {
  GLXFBConfig config;

  GdkDisplay* display = gdk_window_get_display (window);

  if (!gdk_x11_screen_init_gl (gdk_window_get_screen (window))) {
      g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "No GL implementation is available");
      return NULL;
  }

  _GdkVisual* visual = (_GdkVisual*)gdk_window_get_visual ((GdkWindow*)window);
  if (!find_fbconfig_for_visual (display, visual, &config, full, error)) return NULL;
  printf("visual depth %i\n", visual->depth);

  _GdkX11GLContext* context = g_object_new (GDK_TYPE_X11_GL_CONTEXT,
                          "display", display,
                          "window", window,
                          "shared-context", share,
                          NULL);

  context->glx_config = config;
  context->is_attached = attached;

  return GDK_GL_CONTEXT (context);
}

#else

static void _get_dummy_window_hwnd(GdkWGLDummy* dummy) {
    WNDCLASSEX dummy_wc;

    memset(&dummy_wc, 0, sizeof(WNDCLASSEX));

    dummy_wc.cbSize = sizeof(WNDCLASSEX);
    dummy_wc.style = CS_OWNDC;
    dummy_wc.lpfnWndProc = (WNDPROC)DefWindowProc;
    dummy_wc.cbClsExtra = 0;
    dummy_wc.cbWndExtra = 0;
    dummy_wc.hInstance = GetModuleHandle(NULL);
    dummy_wc.hIcon = 0;
    dummy_wc.hCursor = NULL;
    dummy_wc.hbrBackground = 0;
    dummy_wc.lpszMenuName = 0;
    dummy_wc.lpszClassName = "dummy";
    dummy_wc.hIconSm = 0;

    dummy->wc_atom = RegisterClassEx(&dummy_wc);

    dummy->hwnd =
        CreateWindowEx(WS_EX_APPWINDOW,
            MAKEINTATOM(dummy->wc_atom),
            "",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0,
            0,
            0,
            0,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL);
}

static void _destroy_dummy_gl_context(GdkWGLDummy dummy) {
    if (dummy.hglrc != NULL) {
        if (wglGetCurrentContext() == dummy.hglrc) wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy.hglrc);
        dummy.hglrc = NULL;
    }
    if (dummy.hdc != NULL) {
        DeleteDC(dummy.hdc);
        dummy.hdc = NULL;
    }
    if (dummy.hwnd != NULL) {
        DestroyWindow(dummy.hwnd);
        dummy.hwnd = NULL;
    }
    if (dummy.wc_atom != 0) {
        UnregisterClass(MAKEINTATOM(dummy.wc_atom), GetModuleHandle(NULL));
        dummy.wc_atom = 0;
    }
    dummy.inited = FALSE;
}

#define PIXEL_ATTRIBUTES 19

static gint vr_get_dummy_wgl_pfd(HDC hdc, PIXELFORMATDESCRIPTOR* pfd) {
    printf("   get minimal GL Context\n");

    pfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->cColorBits = 32;// GetDeviceCaps(hdc, BITSPIXEL);
    pfd->cDepthBits = 24;
    pfd->cStencilBits = 8;
    pfd->dwLayerMask = PFD_MAIN_PLANE;
    gint best_pf = ChoosePixelFormat(hdc, pfd);

    printf("    chose minimal pixel format: %i\n", best_pf);
    return best_pf;
}

static gint vr_get_wgl_pfd(HDC hdc, PIXELFORMATDESCRIPTOR* pfd, _GdkWin32Display* display) {
    printf("   vr_get_wgl_pfd, get complete GL Context\n");
    gint best_pf = 0;

    pfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);

    if (display->hasWglARBPixelFormat) {
        GdkWGLDummy dummy;
        UINT num_formats;
        gint colorbits = GetDeviceCaps(hdc, BITSPIXEL);

        printf("     device colorbits: %i\n", colorbits);

        // acquire and cache dummy Window (HWND & HDC) and dummy GL Context, we need it for wglChoosePixelFormatARB()
        memset(&dummy, 0, sizeof(GdkWGLDummy));
        best_pf = _gdk_init_dummy_context(&dummy);
        if (best_pf == 0 || !wglMakeCurrent(dummy.hdc, dummy.hglrc)) return 0;

        int pixelAttribsFull[] = {
            WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
            WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,         32,
            //WGL_ALPHA_BITS_ARB,         8, // important, if set to 0 the window might get translucent on some systems
            WGL_DEPTH_BITS_ARB,         24,
            WGL_STENCIL_BITS_ARB,       8,
            WGL_SAMPLE_BUFFERS_ARB,     1,
            WGL_SAMPLES_ARB,            8,
            0
        };

        printf(" --- === --- pixelAttribsFull %i %i\n", colorbits, 0);

        int pixelAttribsMin[] = {
            WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
            WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,         32,
            //WGL_ALPHA_BITS_ARB,         8, // important, if set to 0 the window might get translucent on some systems
            WGL_DEPTH_BITS_ARB,         24,
            WGL_STENCIL_BITS_ARB,       8,
            0
        };

        bool validPF = wglChoosePixelFormatARB(hdc, pixelAttribsFull, NULL, 1, &best_pf, &num_formats);
        if (!validPF) {
            printf(" --= WARNING! full pixel attribs failed, trying without multisampling =--\n");
            validPF = wglChoosePixelFormatARB(hdc, pixelAttribsMin, NULL, 1, &best_pf, &num_formats);
        }

        if (!validPF) {
            printf(" --= WARNING! no working pixel attribs, get fallback GL Context! ..good luck! =--\n");
            best_pf = vr_get_dummy_wgl_pfd(hdc, pfd);
        }

        wglMakeCurrent(NULL, NULL);
        _destroy_dummy_gl_context(dummy);
    } else {
        printf(" --= WARNING! on ARB, get fallback GL Context! ..good luck! =--\n");
        best_pf = vr_get_dummy_wgl_pfd(hdc, pfd);
    }

    printf("   chose pixel format: %i\n", best_pf);
    return best_pf;
}

static gint _gdk_init_dummy_context(GdkWGLDummy* dummy) {
    PIXELFORMATDESCRIPTOR pfd;
    gboolean set_pixel_format_result = FALSE;
    gint best_idx = 0;

    _get_dummy_window_hwnd(dummy);

    dummy->hdc = GetDC(dummy->hwnd);
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    best_idx = vr_get_dummy_wgl_pfd(dummy->hdc, &pfd);

    if (best_idx != 0) set_pixel_format_result = SetPixelFormat(dummy->hdc, best_idx, &pfd);
    if (best_idx == 0 || !set_pixel_format_result) return 0;

    dummy->hglrc = wglCreateContext(dummy->hdc);
    if (dummy->hglrc == NULL) return 0;

    dummy->inited = TRUE;

    return best_idx;
}

gboolean _gdk_win32_display_init_gl(GdkDisplay* display) {
    _GdkWin32Display* display_win32 = (_GdkWin32Display*)display;
    gint best_idx = 0;
    GdkWGLDummy dummy;

    if (display_win32->have_wgl) return TRUE;

    memset(&dummy, 0, sizeof(GdkWGLDummy));

    /* acquire and cache dummy Window (HWND & HDC) and
     * dummy GL Context, it is used to query functions
     * and used for other stuff as well
     */
    best_idx = _gdk_init_dummy_context(&dummy);

    if (best_idx == 0 || !wglMakeCurrent(dummy.hdc, dummy.hglrc)) return FALSE;

    display_win32->have_wgl = TRUE;
    display_win32->gl_version = epoxy_gl_version();

    display_win32->hasWglARBCreateContext = epoxy_has_wgl_extension(dummy.hdc, "WGL_ARB_create_context");
    display_win32->hasWglEXTSwapControl = epoxy_has_wgl_extension(dummy.hdc, "WGL_EXT_swap_control");
    display_win32->hasWglOMLSyncControl = epoxy_has_wgl_extension(dummy.hdc, "WGL_OML_sync_control");
    display_win32->hasWglARBPixelFormat = epoxy_has_wgl_extension(dummy.hdc, "WGL_ARB_pixel_format");
    display_win32->hasWglARBmultisample = epoxy_has_wgl_extension(dummy.hdc, "WGL_ARB_multisample");

    wglMakeCurrent(NULL, NULL);
    _destroy_dummy_gl_context(dummy);

    return TRUE;
}

GdkGLContext* win32_window_create_gl_context(GdkWindow* window, gboolean attached, GdkGLContext* share, gboolean full, GError** error) {
    GdkDisplay* display = gdk_window_get_display(window);
    _GdkWin32Display* display_win32 = (_GdkWin32Display*)display;
    _GdkWin32GLContext* context = NULL;
    GdkVisual* visual = gdk_window_get_visual(window);

    if (!_gdk_win32_display_init_gl(display)) {
        g_set_error_literal(error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "No GL implementation is available");
        return NULL;
    }

    HWND hwnd = gdk_win32_window_get_handle(window);
    HDC hdc   = GetDC(hwnd);

    display_win32->gl_hdc = hdc;
    display_win32->gl_hwnd = hwnd;

    context = g_object_new(GDK_TYPE_WIN32_GL_CONTEXT, "display", display, "window", window, "shared-context", share, NULL);
    context->gl_hdc = hdc;
    context->is_attached = attached;

    return GDK_GL_CONTEXT(context);
}


/*static void
gdk_window_begin_paint_internal(_GdkWindow* window, const cairo_region_t* region) {
    GdkRectangle clip_box;
    double sx, sy;
    gboolean needs_surface;
    cairo_content_t surface_content;

    if (GDK_WINDOW_DESTROYED(window) ||
        !gdk_window_has_impl(window))
        return;

    if (window->current_paint.surface != NULL)
    {
        g_warning("A paint operation on the window is alredy in progress. "
            "This is not allowed.");
        return;
    }

    _GdkWindowImplClass* impl_class = getGdkWindowImplClass();

    needs_surface = TRUE;
    if (impl_class->begin_paint)
        needs_surface = impl_class->begin_paint(window);

    window->current_paint.region = cairo_region_copy(region);
    cairo_region_intersect(window->current_paint.region, window->clip_region);
    cairo_region_get_extents(window->current_paint.region, &clip_box);

    window->current_paint.flushed_region = cairo_region_create();
    window->current_paint.need_blend_region = cairo_region_create();

    surface_content = gdk_window_get_content(window);

    _GdkWindow* iwindow = (_GdkWindow*)window->impl_window;
    window->current_paint.use_gl = iwindow->gl_paint_context != NULL;

    if (window->current_paint.use_gl)
    {
        GdkGLContext* context;

        int ww = gdk_window_get_width(window) * gdk_window_get_scale_factor(window);
        int wh = gdk_window_get_height(window) * gdk_window_get_scale_factor(window);

        context = gdk_window_get_paint_gl_context(window, NULL);
        if (context == NULL)
        {
            g_warning("gl rendering failed, context: %p", context);
            window->current_paint.use_gl = FALSE;
        }
        else
        {
            gdk_gl_context_make_current(context);
            // With gl we always need a surface to combine the gl drawing with the native drawing.
            needs_surface = TRUE;
            // Also, we need the surface to include alpha
            surface_content = CAIRO_CONTENT_COLOR_ALPHA;

            // Initial setup
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            glViewport(0, 0, ww, wh);
        }
    }

    if (needs_surface)
    {
        window->current_paint.surface = gdk_window_create_similar_surface(window,
            surface_content,
            MAX(clip_box.width, 1),
            MAX(clip_box.height, 1));
        sx = sy = 1;
        cairo_surface_get_device_scale(window->current_paint.surface, &sx, &sy);
        cairo_surface_set_device_offset(window->current_paint.surface, -clip_box.x * sx, -clip_box.y * sy);
        gdk_cairo_surface_mark_as_direct(window->current_paint.surface, window);

        window->current_paint.surface_needs_composite = TRUE;
    }
    else
    {
        window->current_paint.surface = gdk_window_ref_impl_surface(window);
        window->current_paint.surface_needs_composite = FALSE;
    }

    if (!cairo_region_is_empty(window->current_paint.region))
        gdk_window_clear_backing_region(window);
}

static void gdk_window_end_paint_internal(_GdkWindow* window) {
    _GdkWindow* composited;
    GdkRectangle clip_box = { 0, };
    cairo_t* cr;

    if (GDK_WINDOW_DESTROYED(window) ||
        !gdk_window_has_impl(window))
        return;

    if (window->current_paint.surface == NULL)
    {
        g_warning(G_STRLOC": no preceding call to gdk_window_begin_draw_frame(), see documentation");
        return;
    }

    _GdkWindowImplClass* impl_class = getGdkWindowImplClass();

    if (impl_class->end_paint)
        impl_class->end_paint(window);

    if (window->current_paint.surface_needs_composite)
    {
        cairo_surface_t* surface;

        cairo_region_get_extents(window->current_paint.region, &clip_box);

        if (window->current_paint.use_gl)
        {
            cairo_region_t* opaque_region = cairo_region_copy(window->current_paint.region);
            cairo_region_subtract(opaque_region, window->current_paint.flushed_region);
            cairo_region_subtract(opaque_region, window->current_paint.need_blend_region);

            gdk_gl_context_make_current(window->gl_paint_context);

            if (!cairo_region_is_empty(opaque_region))
                gdk_gl_texture_from_surface(window->current_paint.surface,
                    opaque_region);
            if (!cairo_region_is_empty(window->current_paint.need_blend_region))
            {
                glEnable(GL_BLEND);
                gdk_gl_texture_from_surface(window->current_paint.surface,
                    window->current_paint.need_blend_region);
                glDisable(GL_BLEND);
            }

            cairo_region_destroy(opaque_region);

            gdk_gl_context_end_frame(window->gl_paint_context,
                window->current_paint.region,
                window->active_update_area);
        }
        else
        {
            surface = gdk_window_ref_impl_surface(window);
            cr = cairo_create(surface);

            cairo_set_source_surface(cr, window->current_paint.surface, 0, 0);
            gdk_cairo_region(cr, window->current_paint.region);
            cairo_clip(cr);

            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_paint(cr);

            cairo_destroy(cr);

            cairo_surface_flush(surface);
            cairo_surface_destroy(surface);
        }
    }

    gdk_window_free_current_paint(window);

    // find a composited window in our hierarchy to signal its
    // parent to redraw, calculating the clip box as we go...
    // stop if parent becomes NULL since then we'd have nowhere
    // to draw (ie: 'composited' will always be non-NULL here).
    for (composited = window; composited->parent; composited = composited->parent) {
        clip_box.x += composited->x;
        clip_box.y += composited->y;

        _GdkWindow* parent = composited->parent;
        clip_box.width = MIN(clip_box.width, parent->width - clip_box.x);
        clip_box.height = MIN(clip_box.height, parent->height - clip_box.y);

        if (composited->composited) {
            gdk_window_invalidate_rect(GDK_WINDOW(composited->parent), &clip_box, FALSE);
            break;
        }
    }
}*/

#endif

GdkGLContext* gdk_window_get_paint_gl_context(_GdkWindow* window, GError** error) {
  GError *internal_error = NULL;



    printf(" gdk_window_get_paint_gl_context %p %p %p\n", window, window->impl, window->impl_window);


  _GdkWindow* iwindow = (_GdkWindow*)window->impl_window;
  if (!iwindow) return 0;
  if (iwindow->gl_paint_context == NULL) {
      _GdkWindowImplClass *impl_class = getGdkWindowImplClass();

      if (impl_class->create_gl_context == NULL) {
          g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "The current backend does not support OpenGL");
          return NULL;
        }
#ifdef _WIN32
      iwindow->gl_paint_context = win32_window_create_gl_context(iwindow, TRUE, NULL, FALSE, &internal_error);
#else
      iwindow->gl_paint_context = x11_window_create_gl_context ((GdkWindow*)iwindow, TRUE, NULL, FALSE, &internal_error);
#endif
    }

  if (internal_error != NULL) {
      g_propagate_error (error, internal_error);
      g_clear_object (&(iwindow->gl_paint_context));
      return NULL;
    }

  gdk_gl_context_realize (iwindow->gl_paint_context, &internal_error);
  if (internal_error != NULL) {
        printf("setting the gl_paint_context context failed!\n");
      g_propagate_error (error, internal_error);
      g_clear_object (&(iwindow->gl_paint_context));
      return NULL;
  }

  return iwindow->gl_paint_context;
}

GdkGLContext* _gdk_window_create_gl_context (_GdkWindow* window, GError** error) {
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  GdkGLContext* paint_context = gdk_window_get_paint_gl_context (window, error);
  if (paint_context == NULL) return NULL;

#ifdef _WIN32
  return win32_window_create_gl_context(window->impl_window, TRUE, NULL, TRUE, error);
#else
  return x11_window_create_gl_context(window->impl_window, TRUE, NULL, TRUE, error);
#endif
}

static GdkGLContext* gl_area_real_create_context(GLArea *area) {
    printf("gl_area_real_create_context\n");
    //GLAreaPrivate *priv = gl_area_get_instance_private (area);
    GtkWidget *widget = GTK_WIDGET (area);
    GError *error = NULL;
    GdkGLContext *context;

    _GdkWindow* win = (_GdkWindow*)gtk_widget_get_window(widget);

    override_window_invalidate_for_new_frame(win);
/*#ifdef _WIN32 // in GuiManager
    override_win32_gl_context_realize();
#endif*/

    //context = gdk_window_create_gl_context (gtk_widget_get_window (widget), &error);
    context = _gdk_window_create_gl_context(win, &error);
    if (!context) return 0;
    //_GdkWindow* win = gtk_widget_get_window(widget);
    //context = x11_window_create_gl_context(win->impl_window, TRUE, 0, &error);
    if (error != NULL) {
        gl_area_set_error (area, error);
        g_clear_object (&context);
        g_clear_error (&error);
        return NULL;
    }

    gdk_gl_context_set_use_es (context, 0);
    gdk_gl_context_set_required_version (context, 3, 2);

    gdk_gl_context_realize (context, &error);
    if (error != NULL) {
        gl_area_set_error (area, error);
        g_clear_object (&context);
        g_clear_error (&error);
        return NULL;
    }

    gdk_gl_context_make_current(context);

    return context;
}

static void gl_area_realize (GtkWidget *widget) {
    printf("gl_area_realize\n");
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    GtkAllocation allocation;
    GdkWindowAttr attributes;
    gint attributes_mask;

    GTK_WIDGET_CLASS (gl_area_parent_class)->realize (widget);

    gtk_widget_get_allocation (widget, &allocation);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_ONLY;
    attributes.event_mask = gtk_widget_get_events (widget);

    //_GdkWindow* window = gtk_widget_get_window(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y;

    priv->event_window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
    gtk_widget_register_window (widget, priv->event_window);

    g_clear_error (&priv->error);
    priv->context = gl_area_real_create_context(area);
    priv->needs_resize = TRUE;
}

static void gl_area_notify(GObject* object, GParamSpec* pspec) {
    if (strcmp (pspec->name, "scale-factor") == 0) {
        GLArea *area = GL_AREA (object);
        GLAreaPrivate *priv = gl_area_get_instance_private (area);
        priv->needs_resize = TRUE;
    }

    if (G_OBJECT_CLASS (gl_area_parent_class)->notify)
        G_OBJECT_CLASS (gl_area_parent_class)->notify (object, pspec);
}


static void gl_area_resize (GLArea *area, int width, int height) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    priv->needs_resize = TRUE;
    //gtk_widget_queue_draw(GTK_WIDGET(area));
}

static void gl_area_unrealize (GtkWidget *widget) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);

    if (priv->context != NULL) {
        /* Make sure to unset the context if current */
        if (priv->context == gdk_gl_context_get_current()) gdk_gl_context_clear_current ();
    }

    g_clear_object (&priv->context);
    g_clear_error (&priv->error);

    if (priv->event_window != NULL) {
        gtk_widget_unregister_window (widget, priv->event_window);
        gdk_window_destroy (priv->event_window);
        priv->event_window = NULL;
    }

    GTK_WIDGET_CLASS (gl_area_parent_class)->unrealize (widget);
}

static void gl_area_map (GtkWidget *widget) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    if (priv->event_window != NULL) gdk_window_show (priv->event_window);
    GTK_WIDGET_CLASS (gl_area_parent_class)->map (widget);
}

static void gl_area_unmap (GtkWidget *widget) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    if (priv->event_window != NULL) gdk_window_hide (priv->event_window);
    GTK_WIDGET_CLASS (gl_area_parent_class)->unmap (widget);
}

static void gl_area_size_allocate(GtkWidget* widget, GtkAllocation *allocation) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);

    GTK_WIDGET_CLASS (gl_area_parent_class)->size_allocate (widget, allocation);

    if (gtk_widget_get_realized (widget)) {
        if (priv->event_window != NULL) gdk_window_move_resize(priv->event_window,
                                allocation->x, allocation->y, allocation->width, allocation->height);

        priv->needs_resize = TRUE;
    }
}

cairo_region_t *
gdk_cairo_region_from_clip (cairo_t *cr)
{
  cairo_rectangle_list_t *rectangles;
  cairo_region_t *region;
  int i;

  rectangles = cairo_copy_clip_rectangle_list (cr);

  if (rectangles->status != CAIRO_STATUS_SUCCESS)
    return NULL;

  region = cairo_region_create ();
  for (i = 0; i < rectangles->num_rectangles; i++)
    {
      cairo_rectangle_int_t clip_rect;
      cairo_rectangle_t *rect;

      rect = &rectangles->rectangles[i];

      /* Here we assume clip rects are ints for direct targets, which
         is true for cairo */
      clip_rect.x = (int)rect->x;
      clip_rect.y = (int)rect->y;
      clip_rect.width = (int)rect->width;
      clip_rect.height = (int)rect->height;

      cairo_region_union_rectangle (region, &clip_rect);
    }

  cairo_rectangle_list_destroy (rectangles);

  return region;
}

void gdk_window_get_unscaled_size (_GdkWindow *window, int *unscaled_width, int *unscaled_height) {
    g_return_if_fail (GDK_IS_WINDOW (window));

    if (window->impl_window == window) {
        //impl_class = GDK_WINDOW_IMPL_GET_CLASS (window->impl);
        _GdkWindowImplClass* impl_class = getGdkWindowImplClass();
        impl_class->get_unscaled_size((GdkWindow*)window, unscaled_width, unscaled_height);
        return;
    }

    gint scale = gdk_window_get_scale_factor((GdkWindow*)window);
    if (unscaled_width) *unscaled_width = window->width * scale;
    if (unscaled_height) *unscaled_height = window->height * scale;
}



#define FLIP_Y(_y) (unscaled_window_height - (_y))

cairo_region_t* clip_region;
int window_scale;
int unscaled_window_width, unscaled_window_height, dx, dy;
cairo_rectangle_int_t clip_rect, dest;
_GdkWindow* impl_window;

void gl_area_trigger_resize(GLArea* area) {
    GLAreaPrivate* priv = gl_area_get_instance_private(area);
    priv->needs_resize = TRUE;
    global_invalidate = TRUE;
}

void glarea_render(GLArea* area) {
    GLAreaPrivate* priv = gl_area_get_instance_private (area);

    //if (priv->needs_resize) { // insufficient, when reducing the width, when getting small its not called anymore??
    if (priv->needs_resize || priv->clipping.w != dest.width || priv->clipping.h != dest.height) {
        priv->clipping.x = dest.x;
        priv->clipping.y = dest.y;
        priv->clipping.w = dest.width;
        priv->clipping.h = dest.height;
        priv->clipping.W = unscaled_window_width;
        priv->clipping.H = unscaled_window_height;
        g_signal_emit (area, area_signals[RESIZE], 0, 0, 0, NULL);
        global_invalidate = TRUE;
        priv->needs_resize = FALSE;
        priv->needs_render = TRUE;
    }

    if (priv->needs_render) {
        gboolean unused;
        g_signal_emit (area, area_signals[RENDER], 0, priv->context, &unused);
        priv->needs_render = FALSE;
    }
}

void cairo_draw(cairo_t* cr, GLArea* area, _GdkWindow* window, int scale, int x, int y, int width, int height) {
  if (glGetError() != GL_NO_ERROR) printf(" gl error on cairo_draw_begin beg\n");
  GLAreaPrivate* priv = gl_area_get_instance_private (area);
  impl_window = window->impl_window;
  window_scale = gdk_window_get_scale_factor ((GdkWindow*)impl_window);
  clip_region = gdk_cairo_region_from_clip (cr);
  gdk_gl_context_make_current(priv->context);
  if (glGetError() != GL_NO_ERROR) printf(" gl error on gdk_gl_context_make_current\n");
  cairo_matrix_t matrix;
  cairo_get_matrix (cr, &matrix);
  dx = matrix.x0;
  dy = matrix.y0;

  if (clip_region != NULL) {
      /* Translate to impl coords */
      cairo_region_translate(clip_region, dx, dy);
      gdk_window_get_unscaled_size (impl_window, &unscaled_window_width, &unscaled_window_height);

      for (int i = 0; i < cairo_region_num_rectangles (clip_region); i++) {
          cairo_region_get_rectangle (clip_region, i, &clip_rect);
          clip_rect.x *= window_scale;
          clip_rect.y *= window_scale;
          clip_rect.width *= window_scale;
          clip_rect.height *= window_scale;

          dest.x = dx * window_scale;
          dest.y = dy * window_scale;
          dest.width = width * window_scale / scale;
          dest.height = height * window_scale / scale;

          if (gdk_rectangle_intersect (&clip_rect, &dest, &dest)) {
              glarea_render(area);
              if (impl_window->current_paint.flushed_region) {
                  cairo_rectangle_int_t flushed_rect;

                  flushed_rect.x = dest.x / window_scale;
                  flushed_rect.y = dest.y / window_scale;
                  flushed_rect.width = (dest.x + dest.width + window_scale - 1) / window_scale - flushed_rect.x;
                  flushed_rect.height = (dest.y + dest.height + window_scale - 1) / window_scale - flushed_rect.y;

                  cairo_region_union_rectangle (impl_window->current_paint.flushed_region, &flushed_rect);
                  cairo_region_subtract_rectangle (impl_window->current_paint.need_blend_region, &flushed_rect);
                }
            }
        }
    }

    if (clip_region) cairo_region_destroy (clip_region);
}

static gboolean gl_area_draw(GtkWidget* widget, cairo_t* cr) {
    //SET_DEBUG_BREAK_POINT(B,1)
    if (glGetError() != GL_NO_ERROR) printf(" gl error on gl_area_draw beg\n");

    GLArea *area = GL_AREA(widget);
    //gtk_widget_set_double_buffered(widget, FALSE);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    //gboolean unused;
    int w, h, scale;
    //GLenum status;

    if (priv->error != NULL) return FALSE;
    if (priv->context == NULL) return FALSE;

    gl_area_make_current (area);
    if (glGetError() != GL_NO_ERROR) printf(" gl error on gl_area_make_current\n");

    //glEnable (GL_DEPTH_TEST);

    scale = gtk_widget_get_scale_factor (widget);
    w = gtk_widget_get_allocated_width (widget) * scale;
    h = gtk_widget_get_allocated_height (widget) * scale;
    cairo_draw(cr, area, (_GdkWindow*)gtk_widget_get_window(widget), scale, 0, 0, w, h);

    return TRUE;
}

gboolean _boolean_handled_accumulator(GSignalInvocationHint* ihint, GValue* return_accu, const GValue* handler_return, gpointer dummy) {
    gboolean continue_emission;
    gboolean signal_handled;

    signal_handled = g_value_get_boolean(handler_return);
    g_value_set_boolean(return_accu, signal_handled);
    continue_emission = !signal_handled;

    return continue_emission;
}

#define g_marshal_value_peek_int(v)      (v)->data[0].v_int

void _marshal_VOID__INT_INT(GClosure* closure,
    GValue* return_value G_GNUC_UNUSED,
    guint         n_param_values,
    const GValue* param_values,
    gpointer      invocation_hint G_GNUC_UNUSED,
    gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__INT_INT) (gpointer data1,
        gint arg1, gint arg2, gpointer data2);
    GCClosure* cc = (GCClosure*)closure;
    gpointer data1, data2;
    GMarshalFunc_VOID__INT_INT callback;

    g_return_if_fail(n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    } else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__INT_INT)(marshal_data ? marshal_data : cc->callback);

    callback(data1,
        g_marshal_value_peek_int(param_values + 1),
        g_marshal_value_peek_int(param_values + 2),
        data2);
}

#define I_(string) g_intern_static_string(string)

static void gl_area_class_init (GLAreaClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    klass->resize = gl_area_resize;
    klass->create_context = gl_area_real_create_context;

    widget_class->realize = gl_area_realize;
    widget_class->unrealize = gl_area_unrealize;
    widget_class->map = gl_area_map;
    widget_class->unmap = gl_area_unmap;
    widget_class->size_allocate = gl_area_size_allocate;
    widget_class->draw = gl_area_draw;

    gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_DRAWING_AREA);

    gobject_class->set_property = gl_area_set_property;
    gobject_class->get_property = gl_area_get_property;
    gobject_class->dispose = gl_area_dispose;
    gobject_class->notify = gl_area_notify;

    g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);

    area_signals[RENDER] = g_signal_new (I_("render"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GLAreaClass, render),
                  _boolean_handled_accumulator, NULL,
                  NULL,
                  G_TYPE_BOOLEAN, 1,
                  GDK_TYPE_GL_CONTEXT);

    area_signals[RESIZE] = g_signal_new (I_("resize"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GLAreaClass, resize),
                  NULL, NULL,
                  _marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
}

static void gl_area_init (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    gtk_widget_set_has_window (GTK_WIDGET (area), FALSE);
    gtk_widget_set_app_paintable (GTK_WIDGET (area), TRUE);
    priv->needs_render = TRUE;
    priv->clipping.x = priv->clipping.y = 0;
    priv->clipping.w = priv->clipping.h = 100;
    priv->clipping.W = priv->clipping.H = 100;
}

GtkWidget* gl_area_new (void) {
    GtkWidget* obj = g_object_new (TYPE_GL_AREA, NULL);
    return obj;
}

void gl_area_set_error (GLArea    *area, const GError *error) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_if_fail (IS_GL_AREA (area));
    g_clear_error (&priv->error);
    if (error) priv->error = g_error_copy (error);
}

GError* gl_area_get_error (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_val_if_fail (IS_GL_AREA (area), NULL);
    return priv->error;
}

GLClipping gl_area_get_clipping(GLArea* area) {
    GLAreaPrivate* priv = gl_area_get_instance_private (area);
    return priv->clipping;
}

#ifndef _WIN32
int glXSwapIntervalSGI(int);
#endif

void gl_area_set_vsync(GLArea* area, gboolean b) {
#ifndef _WIN32
    if (b) {
        int res = glXSwapIntervalSGI(1);
        printf(" gl_area_set_vsync %i\n", res);
    } else {
        int res = glXSwapIntervalSGI(0);
        printf(" gl_area_set_vsync %i\n", res);
    }
#endif
}

void gl_area_queue_render (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_if_fail (IS_GL_AREA (area));
    priv->needs_render = TRUE;
    gtk_widget_queue_draw (GTK_WIDGET (area));
}

GdkGLContext* gl_area_get_context (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_val_if_fail (IS_GL_AREA (area), NULL);
    return priv->context;
}

void gl_area_make_current (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_if_fail (IS_GL_AREA (area));
    GtkWidget* widget = GTK_WIDGET (area);
    g_return_if_fail (gtk_widget_get_realized (widget));
    if (glGetError() != GL_NO_ERROR) printf(" gl error on gdk_gl_context_make_current bevore\n");
    if (priv->context != NULL) gdk_gl_context_make_current (priv->context);
    if (glGetError() != GL_NO_ERROR) printf(" gl error on gdk_gl_context_make_current after\n");
}
