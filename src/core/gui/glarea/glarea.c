#define GTK_COMPILATION
#define GDK_COMPILATION


#include "glarea.h"
#include "glareaCommon.h"

_GdkWindowImplClass* getGdkWindowImplClass() {
    static _GdkWindowImplClass* windowType = 0;
    if (windowType) return windowType;

    int typeID = 0;
    GType type = 0;
    if (!type) { type = g_type_from_name("GdkWindowImplX11"); typeID = 1; }
    if (!type) { type = g_type_from_name("GdkWindowImplWin32"); typeID = 2; }
    if (!type) { type = g_type_from_name("GdkWindowImplQuartz"); typeID = 3; }
    if (!type) { type = g_type_from_name("GdkWindowImplWayland"); typeID = 4; }
    if (!type) { type = g_type_from_name("GdkWindowImplBroadway"); typeID = 5; }
    windowType = g_type_class_ref(type);

#ifndef _WIN32
    if (typeID != 1 && typeID != 4) printf("ERROR in glareaCommon.h/getGdkWindowImplClass ! expected window type X11 or Wayland but found %i this is not supported yet!\n", typeID);
#else
    if (typeID != 2) printf("ERROR in glareaCommon.h/getGdkWindowImplClass ! expected window type Win32 but found %i this is not supported yet!\n", typeID);
#endif

    return windowType;
}

gboolean global_invalidate = TRUE;

gboolean* getGlobalInvalidate() { return &global_invalidate; }

//#define WAYLAND

#ifndef _WIN32

#ifdef WAYLAND
#include "glareaWayland.h"
#else
#include "glareaX11.h"
#endif

#else
#include "glareaWin.h"
#endif

#include <GL/glu.h>

#define CHECK_GL_ERROR(msg) \
{ \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        static int i=0; i++; \
        if (i <= 4) printf(" gl error on %s: %s\n", msg, gluErrorString(err)); \
        if (i == 4) printf("  ..ignoring further errors\n"); \
    } \
}

void override_window_invalidate_for_new_frame(_GdkWindow* window) {
    printf("override_window_invalidate_for_new_frame, window: %p\n", window);
    _GdkWindowImplClass* impl_class = getGdkWindowImplClass();
    printf(" impl_class: %p\n", impl_class);
    if (!impl_class) {
        printf(" getGdkWindowImplClass failed! ignore override\n");
        return;
    }

#ifdef _WIN32
    impl_class->invalidate_for_new_frame = _gdk_win32_window_invalidate_for_new_frame;
    //impl_class->set_opacity = _gdk_win32_window_set_opacity;
#else
#ifdef WAYLAND
    impl_class->invalidate_for_new_frame = _gdk_wayland_window_invalidate_for_new_frame;
#else
    impl_class->invalidate_for_new_frame = _gdk_x11_window_invalidate_for_new_frame;
#endif
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

GdkGLContext* gdk_window_get_paint_gl_context(_GdkWindow* window, GError** error) {
    GError *internal_error = NULL;

    printf(" gdk_window_get_paint_gl_context %p %p %p\n", window, window->impl, window->impl_window);


    _GdkWindow* iwindow = (_GdkWindow*)window->impl_window;
    if (!iwindow) return 0;
    if (iwindow->gl_paint_context == NULL) {
        _GdkWindowImplClass* impl_class = getGdkWindowImplClass();

        if (!impl_class) {
            g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "ERROR: no implementation class!");
            return NULL;
        }

        if (impl_class->create_gl_context == NULL) {
            g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "The current backend does not support OpenGL");
            return NULL;
        }

#ifdef _WIN32
        iwindow->gl_paint_context = win32_window_create_gl_context(iwindow, TRUE, NULL, FALSE, &internal_error);
#else

#ifdef WAYLAND
        iwindow->gl_paint_context = gdk_wayland_window_create_gl_context ((GdkWindow*)iwindow, TRUE, NULL, &internal_error);
#else
        iwindow->gl_paint_context = x11_window_create_gl_context ((GdkWindow*)iwindow, TRUE, NULL, FALSE, &internal_error);
#endif

#endif
        printf(" gdk_window_get_paint_gl_context - set gl_paint_context %p\n", iwindow->gl_paint_context);
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

#ifdef WAYLAND
  GdkGLContext* paint_context = gdk_window_get_paint_gl_context (window, error);
#else
  GdkGLContext* paint_context = gdk_window_get_paint_gl_context (window, error);
#endif
  if (paint_context == NULL) return NULL;

#ifdef _WIN32
  return win32_window_create_gl_context(window->impl_window, TRUE, NULL, TRUE, error);
#else

#ifdef WAYLAND
  return gdk_wayland_window_create_gl_context(window->impl_window, TRUE, NULL, error);
#else
  return x11_window_create_gl_context(window->impl_window, TRUE, NULL, TRUE, error);
#endif
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
cairo_rectangle_int_t clip_rect, dest, fdest;
_GdkWindow* impl_window;

void gl_area_trigger_resize(GLArea* area) {
    GLAreaPrivate* priv = gl_area_get_instance_private(area);
    priv->needs_resize = TRUE;
    global_invalidate = TRUE;
}

void glarea_render(GLArea* area) {
    GLAreaPrivate* priv = gl_area_get_instance_private (area);
    //printf("glarea_render %i, %i %i, %i %i\n", priv->needs_resize, priv->clipping.w, dest.width, priv->clipping.h, dest.height);

    //if (priv->needs_resize) { // insufficient, when reducing the width, when getting small its not called anymore??
    if (priv->needs_resize || priv->clipping.w != dest.width || priv->clipping.h != dest.height) {
        priv->clipping.x = dest.x;
        priv->clipping.y = dest.y;
        priv->clipping.w = dest.width;
        priv->clipping.h = dest.height;
        priv->clipping.W = unscaled_window_width;
        priv->clipping.H = unscaled_window_height;
        //printf("glarea_render, emit resize %i, %i %i, %i %i\n", priv->needs_resize, priv->clipping.w, dest.width, priv->clipping.h, dest.height);
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
    CHECK_GL_ERROR("cairo_draw A1");
    GLAreaPrivate* priv = gl_area_get_instance_private (area);
    impl_window = window->impl_window;
    window_scale = gdk_window_get_scale_factor ((GdkWindow*)impl_window);
    clip_region = gdk_cairo_region_from_clip (cr);
    gdk_gl_context_make_current(priv->context);
    CHECK_GL_ERROR("cairo_draw A2");
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

          if (gdk_rectangle_intersect (&clip_rect, &dest, &fdest)) {
    CHECK_GL_ERROR("cairo_draw A3");
              glarea_render(area);
    CHECK_GL_ERROR("cairo_draw A4");
              if (impl_window->current_paint.flushed_region) {
                  cairo_rectangle_int_t flushed_rect;

                  flushed_rect.x = fdest.x / window_scale;
                  flushed_rect.y = fdest.y / window_scale;
                  flushed_rect.width = (fdest.x + fdest.width + window_scale - 1) / window_scale - flushed_rect.x;
                  flushed_rect.height = (fdest.y + fdest.height + window_scale - 1) / window_scale - flushed_rect.y;

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
    CHECK_GL_ERROR("gl_area_draw A1");

    GLArea *area = GL_AREA(widget);
    //gtk_widget_set_double_buffered(widget, FALSE);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    //gboolean unused;
    int w, h, scale;
    //GLenum status;

    if (priv->error != NULL) return FALSE;
    if (priv->context == NULL) return FALSE;

    gl_area_make_current (area);
    CHECK_GL_ERROR("gl_area_draw A2");

    //glEnable (GL_DEPTH_TEST);

    scale = gtk_widget_get_scale_factor (widget);
    w = gtk_widget_get_allocated_width (widget) * scale;
    h = gtk_widget_get_allocated_height (widget) * scale;
    cairo_draw(cr, area, (_GdkWindow*)gtk_widget_get_window(widget), scale, 0, 0, w, h);
    CHECK_GL_ERROR("gl_area_draw A3");

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
    CHECK_GL_ERROR("gl_area_queue_render A1");
    gtk_widget_queue_draw (GTK_WIDGET (area));
    CHECK_GL_ERROR("gl_area_queue_render A2");
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
    CHECK_GL_ERROR("gdk_gl_context_make_current bevore");
    if (priv->context != NULL) gdk_gl_context_make_current (priv->context);
    CHECK_GL_ERROR("gdk_gl_context_make_current after");
}
