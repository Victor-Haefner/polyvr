/* GTK - The GIMP Toolkit
 *
 * gtkglarea.h: A GL drawing area
 *
 * Copyright © 2014  Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

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

/**
 * GLArea:
 *
 * A #GtkWidget used for drawing with OpenGL.
 *
 * Since: 3.16
 */
struct _GLArea
{
  /*< private >*/
  GtkWidget parent_instance;
};

/**
 * GLAreaClass:
 * @render: class closure for the #GLArea::render signal
 * @resize: class closeure for the #GLArea::resize signal
 * @create_context: class closure for the #GLArea::create-context signal
 *
 * The `GLAreaClass` structure contains only private data.
 *
 * Since: 3.16
 */
struct _GLAreaClass
{
  /*< private >*/
  GtkWidgetClass parent_class;

  /*< public >*/
  gboolean       (* render)         (GLArea        *area,
                                     GdkGLContext     *context);
  void           (* resize)         (GLArea        *area,
                                     int               width,
                                     int               height);
  GdkGLContext * (* create_context) (GLArea        *area);

  /*< private >*/
  gpointer _padding[6];
};

GDK_AVAILABLE_IN_3_16
GType gl_area_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_16
GtkWidget *     gl_area_new                         (void);

GDK_AVAILABLE_IN_3_22
void            gl_area_set_use_es                  (GLArea    *area,
                                                         gboolean      use_es);
GDK_AVAILABLE_IN_3_22
gboolean        gl_area_get_use_es                  (GLArea    *area);
GDK_AVAILABLE_IN_3_16
void            gl_area_set_required_version        (GLArea    *area,
                                                         gint          major,
                                                         gint          minor);
GDK_AVAILABLE_IN_3_16
void            gl_area_get_required_version        (GLArea    *area,
                                                         gint         *major,
                                                         gint         *minor);
GDK_AVAILABLE_IN_3_16
gboolean        gl_area_get_has_alpha               (GLArea    *area);
GDK_AVAILABLE_IN_3_16
void            gl_area_set_has_alpha               (GLArea    *area,
                                                         gboolean      has_alpha);
GDK_AVAILABLE_IN_3_16
gboolean        gl_area_get_has_depth_buffer        (GLArea    *area);
GDK_AVAILABLE_IN_3_16
void            gl_area_set_has_depth_buffer        (GLArea    *area,
                                                         gboolean      has_depth_buffer);
GDK_AVAILABLE_IN_3_16
gboolean        gl_area_get_has_stencil_buffer      (GLArea    *area);
GDK_AVAILABLE_IN_3_16
void            gl_area_set_has_stencil_buffer      (GLArea    *area,
                                                         gboolean      has_stencil_buffer);
GDK_AVAILABLE_IN_3_16
gboolean        gl_area_get_auto_render             (GLArea    *area);
GDK_AVAILABLE_IN_3_16
void            gl_area_set_auto_render             (GLArea    *area,
                                                         gboolean      auto_render);
GDK_AVAILABLE_IN_3_16
void           gl_area_queue_render                 (GLArea    *area);


GDK_AVAILABLE_IN_3_16
GdkGLContext *  gl_area_get_context                 (GLArea    *area);

GDK_AVAILABLE_IN_3_16
void            gl_area_make_current                (GLArea    *area);
GDK_AVAILABLE_IN_3_16
void            gl_area_attach_buffers              (GLArea    *area);

GDK_AVAILABLE_IN_3_16
void            gl_area_set_error                   (GLArea    *area,
                                                         const GError *error);
GDK_AVAILABLE_IN_3_16
GError *        gl_area_get_error                   (GLArea    *area);

G_END_DECLS

#endif /* __GL_AREA_H__ */
