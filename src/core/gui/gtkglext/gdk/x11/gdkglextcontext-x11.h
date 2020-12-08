/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2002-2004  Naofumi Yasufuku
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.
 */

#ifndef __GDK_GLEXT_CONTEXT_X11_H__
#define __GDK_GLEXT_CONTEXT_X11_H__

#include <GL/glx.h>

#include "../gdkglextcontext.h"
#include "../gdkglextcontextimpl.h"

G_BEGIN_DECLS

typedef struct _GdkGLExtContextImplX11      GdkGLExtContextImplX11;
typedef struct _GdkGLExtContextImplX11Class GdkGLExtContextImplX11Class;

#define GDK_TYPE_GLEXT_CONTEXT_IMPL_X11            (gdk_glext_context_impl_x11_get_type ())
#define GDK_GLEXT_CONTEXT_IMPL_X11(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GLEXT_CONTEXT_IMPL_X11, GdkGLExtContextImplX11))
#define GDK_GLEXT_CONTEXT_IMPL_X11_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GLEXT_CONTEXT_IMPL_X11, GdkGLExtContextImplX11Class))
#define GDK_IS_GLEXT_CONTEXT_IMPL_X11(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GLEXT_CONTEXT_IMPL_X11))
#define GDK_IS_GLEXT_CONTEXT_IMPL_X11_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GLEXT_CONTEXT_IMPL_X11))
#define GDK_GLEXT_CONTEXT_IMPL_X11_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GLEXT_CONTEXT_IMPL_X11, GdkGLExtContextImplX11Class))

struct _GdkGLExtContextImplX11
{
  GdkGLExtContextImpl parent_instance;

  GLXContext glxcontext;
  GdkGLExtContext *share_list;
  gboolean is_direct;
  int render_type;

  GdkGLConfig *glconfig;

  GdkGLDrawable *gldrawable;
  GdkGLDrawable *gldrawable_read; /* currently unused. */

  guint is_destroyed : 1;
  guint is_foreign   : 1;
};

struct _GdkGLExtContextImplX11Class
{
  GdkGLExtContextImplClass parent_class;

  GLXContext (*get_glxcontext) (GdkGLExtContext *glextcontext);
};

GType gdk_glext_context_impl_x11_get_type (void);

GdkGLExtContextImpl *_gdk_x11_glext_context_impl_new (GdkGLExtContext  *glextcontext,
                                                GdkGLDrawable *gldrawable,
                                                GdkGLExtContext  *share_list,
                                                gboolean       direct,
                                                int            render_type);

GdkGLExtContextImpl *_gdk_x11_glext_context_impl_new_from_glxcontext (GdkGLExtContext *glextcontext,
                                                                GdkGLConfig  *glconfig,
                                                                GdkGLExtContext *share_list,
                                                                GLXContext    glxcontext);

void _gdk_x11_glext_context_impl_set_gl_drawable (GdkGLExtContext  *glextcontext,
                                               GdkGLDrawable *gldrawable);

GdkGLExtContext *
_gdk_x11_glext_context_impl_get_current (void);

G_END_DECLS

#endif /* __GDK_GLEXT_CONTEXT_X11_H__ */
