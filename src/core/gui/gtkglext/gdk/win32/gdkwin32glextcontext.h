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

#if !defined (__GDKGLWIN32_H_INSIDE__) && !defined (GDK_GL_COMPILATION)
#error "Only <gdk/gdkglwin32.h> can be included directly."
#endif

#ifndef __GDK_WIN32_GLEXT_CONTEXT_H__m
#define __GDK_WIN32_GLEXT_CONTEXT_H__m

#define INSIDE_GDK_GL_WIN32

#include <gdk/gdkwin32.h>
#include "../gdkgl.h"

G_BEGIN_DECLS

#define GDK_TYPE_WIN32_GLEXT_CONTEXT             (gdk_win32_glext_context_get_type ())
#define GDK_WIN32_GLEXT_CONTEXT(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WIN32_GLEXT_CONTEXT, GdkWin32GLExtContext))
#define GDK_WIN32_GLEXT_CONTEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_WIN32_GLEXT_CONTEXT, GdkWin32GLExtContextClass))
#define GDK_IS_WIN32_GLEXT_CONTEXT(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WIN32_GLEXT_CONTEXT))
#define GDK_IS_WIN32_GLEXT_CONTEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_WIN32_GLEXT_CONTEXT))
#define GDK_WIN32_GLEXT_CONTEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_WIN32_GLEXT_CONTEXT, GdkWin32GLExtContextClass))

#ifdef INSIDE_GDK_GL_WIN32
typedef struct _GdkWin32GLExtContext GdkWin32GLExtContext;
#else
typedef GdkGLExtContext GdkWin32GLExtContext;
#endif
typedef struct _GdkWin32GLExtContextClass GdkWin32GLExtContextClass;

GType          gdk_win32_glext_context_get_type (void);

GdkGLExtContext  *gdk_win32_glext_context_foreign_new     (GdkGLConfig   *glconfig,
                                                     GdkGLExtContext  *share_list,
                                                     HGLRC          hglrc);

HGLRC          gdk_win32_glext_context_get_hglrc       (GdkGLExtContext  *glextcontext);

G_END_DECLS

#ifdef INSIDE_GDK_GL_WIN32
#define GDK_GLEXT_CONTEXT_HGLRC(glextcontext)      (GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl)->hglrc)
#else
#define GDK_GLEXT_CONTEXT_HGLRC(glextcontext)      (gdk_win32_glext_context_get_hglrc (glextcontext))
#endif

#endif /* __GDK_WIN32_GLEXT_CONTEXT_H__m */
