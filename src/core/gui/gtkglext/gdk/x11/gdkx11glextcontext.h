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

#if !defined (__GDKGLX_H_INSIDE__) && !defined (GDK_GL_COMPILATION)
#error "Only <gdk/gdkglx.h> can be included directly."
#endif

#ifndef __GDK_X11_GLEXT_CONTEXT_H__m
#define __GDK_X11_GLEXT_CONTEXT_H__m

#include <gdk/gdkx.h>
#include "../gdkgl.h"



#define GDK_TYPE_X11_GLEXT_CONTEXT             (gdk_x11_glext_context_get_type ())
#define GDK_X11_GLEXT_CONTEXT(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_X11_GLEXT_CONTEXT, GdkX11GLExtContext))
#define GDK_X11_GLEXT_CONTEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_X11_GLEXT_CONTEXT, GdkX11GLExtContextClass))
#define GDK_IS_X11_GLEXT_CONTEXT(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_X11_GLEXT_CONTEXT))
#define GDK_IS_X11_GLEXT_CONTEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_X11_GLEXT_CONTEXT))
#define GDK_X11_GLEXT_CONTEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_X11_GLEXT_CONTEXT, GdkX11GLExtContextClass))

#define INSIDE_GDK_GL_X11

#ifdef INSIDE_GDK_GL_X11
typedef struct _GdkX11GLExtContext GdkX11GLExtContext;
#else
typedef GdkGLExtContext GdkX11GLExtContext;
#endif
typedef struct _GdkX11GLExtContextClass GdkX11GLExtContextClass;

GType         gdk_x11_glext_context_get_type       (void);

GdkGLExtContext *gdk_x11_glext_context_foreign_new    (GdkGLConfig  *glconfig,
                                                 GdkGLExtContext *share_list,
                                                 GLXContext    glxcontext);

GLXContext    gdk_x11_glext_context_get_glxcontext (GdkGLExtContext *glextcontext);

#ifdef INSIDE_GDK_GL_X11

#define GDK_GLEXT_CONTEXT_GLXCONTEXT(glextcontext)   (GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl)->glxcontext)

#else

#define GDK_GLEXT_CONTEXT_GLXCONTEXT(glextcontext)   (gdk_x11_glext_context_get_glxcontext (glextcontext))

#endif



#endif /* __GDK_GL_GL_CONFIG_H__m */
