#ifndef __GL_AREA_H__
#define __GL_AREA_H__

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define TYPE_GL_AREA                (gl_area_get_type ())
#define GL_AREA(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GL_AREA, GLArea))
#define IS_GL_AREA(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GL_AREA))
#define GL_AREA_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GL_AREA, GLAreaClass))
#define IS_GL_AREA_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GL_AREA))
#define GL_AREA_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GL_AREA, GLAreaClass))

typedef struct _GLArea               GLArea;
typedef struct _GLAreaClass          GLAreaClass;

typedef struct {
    int x, y;
    int w, h;
    int W, H;
} GLClipping;

struct _GLArea {
    GtkWidget parent_instance;
};

struct _GLAreaClass {
    GtkWidgetClass parent_class;
    gboolean       (* render)         (GLArea        *area, GdkGLContext* context);
    void           (* resize)         (GLArea        *area, int width, int height);
    GdkGLContext * (* create_context) (GLArea        *area);
    gpointer _padding[6];
};

GType gl_area_get_type (void) G_GNUC_CONST;

GtkWidget *     gl_area_new                         (void);

void  gl_area_set_vsync(GLArea* area, gboolean b);
GLClipping gl_area_get_clipping(GLArea* area);
void gl_area_trigger_resize(GLArea* area);

void override_win32_gl_context_realize();
void disableBlur(GdkWindow* window);

void            gl_area_queue_render                (GLArea    *area);
GdkGLContext *  gl_area_get_context                 (GLArea    *area);
void            gl_area_make_current                (GLArea    *area);
void            gl_area_set_error                   (GLArea    *area, const GError *error);
GError *        gl_area_get_error                   (GLArea    *area);

G_END_DECLS

#endif /* __GL_AREA_H__ */
