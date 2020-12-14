/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2012 Thomas Zimmermann
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "../gdkglextcontext.h"
#include "../gdkgldebug.h"
#include "gdkglwin32.h"

#include "gdkwin32glextcontext.h"
#include "gdkglextcontext-win32.h"

struct _GdkWin32GLExtContext
{
  GdkGLExtContext parent;
};

struct _GdkWin32GLExtContextClass
{
  GdkGLExtContextClass parent_class;
};

G_DEFINE_TYPE (GdkWin32GLExtContext, gdk_win32_glext_context, GDK_TYPE_GLEXT_CONTEXT);

static void
gdk_win32_glext_context_init (GdkWin32GLExtContext *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_win32_glext_context_finalize (GObject *object)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  G_OBJECT_CLASS (gdk_win32_glext_context_parent_class)->finalize (object);
}

static void
gdk_win32_glext_context_class_init (GdkWin32GLExtContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  object_class->finalize = gdk_win32_glext_context_finalize;
}

GdkGLExtContext *
gdk_win32_glext_context_foreign_new (GdkGLConfig  *glconfig,
                                  GdkGLExtContext *share_list,
                                  HGLRC         hglrc)
{
  GdkGLExtContext *glextcontext;
  GdkGLExtContextImpl *impl;

  GDK_GL_NOTE_FUNC ();

  glextcontext = g_object_new(GDK_TYPE_WIN32_GLEXT_CONTEXT, NULL);

  g_return_val_if_fail(glextcontext != NULL, NULL);

  impl = _gdk_win32_glext_context_impl_new_from_hglrc (glextcontext,
                                                    glconfig,
                                                    share_list,
                                                    hglrc);
  if (impl == NULL)
    g_object_unref(glconfig);

  g_return_val_if_fail(impl != NULL, NULL);

  return glextcontext;
}

HGLRC
gdk_win32_glext_context_get_hglrc (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_WIN32_CLASS (glextcontext->impl)->get_hglrc(glextcontext);
}
