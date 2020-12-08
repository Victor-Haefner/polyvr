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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "gdkglextcontext.h"
#include "gdkglprivate.h"
#include "gdkglextcontext.h"
#include "gdkgldrawable.h"
#include "gdkglconfig.h"
#include "gdkglextcontextimpl.h"

#ifdef GDKGLEXT_WINDOWING_X11
#include "x11/gdkglextcontext-x11.h"
#include <gdk/gdkx.h>
#endif
#ifdef GDKGLEXT_WINDOWING_WIN32
#include "win32/gdkglextcontext-win32.h"
#include <gdk/gdkwin32.h>
#endif

gboolean _gdk_glext_context_force_indirect = FALSE;

G_DEFINE_TYPE (GdkGLExtContext,    \
               gdk_glext_context,  \
               G_TYPE_OBJECT)

static void
gdk_glext_context_init (GdkGLExtContext *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_glext_context_class_init (GdkGLExtContextClass *klass)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

/**
 * gdk_glext_context_new:
 * @gldrawable: a #GdkGLDrawable.
 * @share_list: the #GdkGLExtContext with which to share display lists and texture
 *              objects. NULL indicates that no sharing is to take place.
 * @direct: whether rendering is to be done with a direct connection to
 *          the graphics system.
 * @render_type: GDK_GL_RGBA_TYPE.
 *
 * Creates a new OpenGL rendering context.
 *
 * Return value: the new #GdkGLExtContext.
 **/
GdkGLExtContext *
gdk_glext_context_new (GdkGLDrawable *gldrawable,
                    GdkGLExtContext  *share_list,
                    gboolean       direct,
                    int            render_type)
{
  g_return_val_if_fail (GDK_IS_GL_DRAWABLE (gldrawable), NULL);

  return GDK_GL_DRAWABLE_GET_CLASS (gldrawable)->create_glext_context (gldrawable,
                                                                    share_list,
                                                                    direct,
                                                                    render_type);
}

/**
 * gdk_glext_context_copy:
 * @glextcontext: a #GdkGLExtContext.
 * @src: the source context.
 * @mask: which portions of @src state are to be copied to @glextcontext.
 *
 * Copy state from @src rendering context to @glextcontext.
 *
 * @mask contains the bitwise-OR of the same symbolic names that are passed to
 * the glPushAttrib() function. You can use GL_ALL_ATTRIB_BITS to copy all the
 * rendering state information.
 *
 * Return value: FALSE if it fails, TRUE otherwise.
 **/
gboolean
gdk_glext_context_copy (GdkGLExtContext  *glextcontext,
                     GdkGLExtContext  *src,
                     unsigned long  mask)
{
  g_return_val_if_fail (GDK_IS_GLEXT_CONTEXT (glextcontext), FALSE);

  return GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->copy_glext_context_impl (glextcontext,
                                                                                src,
                                                                                mask);
}

/**
 * gdk_glext_context_get_gl_drawable:
 * @glextcontext: a #GdkGLExtContext.
 *
 * Gets #GdkGLDrawable to which the @glextcontext is bound.
 *
 * Return value: the #GdkGLDrawable or NULL if no #GdkGLDrawable is bound.
 **/
GdkGLDrawable *
gdk_glext_context_get_gl_drawable (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_GLEXT_CONTEXT (glextcontext), FALSE);

  return GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->get_gl_drawable (glextcontext);
}

/**
 * gdk_glext_context_get_gl_config:
 * @glextcontext: a #GdkGLExtContext.
 *
 * Gets #GdkGLConfig with which the @glextcontext is configured.
 *
 * Return value: the #GdkGLConfig.
 **/
GdkGLConfig *
gdk_glext_context_get_gl_config (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_GLEXT_CONTEXT (glextcontext), FALSE);

  return GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->get_gl_config (glextcontext);
}

/**
 * gdk_glext_context_get_share_list:
 * @glextcontext: a #GdkGLExtContext.
 *
 * Gets #GdkGLExtContext with which the @glextcontext shares the display lists and
 * texture objects.
 *
 * Return value: the #GdkGLExtContext.
 **/
GdkGLExtContext *
gdk_glext_context_get_share_list (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_GLEXT_CONTEXT (glextcontext), FALSE);

  return GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->get_share_list(glextcontext);
}

/**
 * gdk_glext_context_is_direct:
 * @glextcontext: a #GdkGLExtContext.
 *
 * Returns whether the @glextcontext is a direct rendering context.
 *
 * Return value: TRUE if the @glextcontext is a direct rendering contest.
 **/
gboolean
gdk_glext_context_is_direct (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_GLEXT_CONTEXT (glextcontext), FALSE);

  return GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->is_direct(glextcontext);
}

/**
 * gdk_glext_context_get_render_type:
 * @glextcontext: a #GdkGLExtContext.
 *
 * Gets render_type of the @glextcontext.
 *
 * Return value: GDK_GL_RGBA_TYPE.
 **/
int
gdk_glext_context_get_render_type (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_GLEXT_CONTEXT (glextcontext), FALSE);

  return GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->get_render_type(glextcontext);
}

gboolean
gdk_glext_context_make_current(GdkGLExtContext  *glextcontext,
                            GdkGLDrawable *draw,
                            GdkGLDrawable *read)
{
  g_return_val_if_fail (GDK_IS_GLEXT_CONTEXT (glextcontext), FALSE);

  return GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->make_current(glextcontext,
                                                                       draw,
                                                                       read);
}

/**
 * gdk_glext_context_release_current:
 *
 * Releases the current #GdkGLExtContext.
 **/
void
gdk_glext_context_release_current ()
{
  GdkGLExtContext *glextcontext = gdk_glext_context_get_current();

  g_return_if_fail(glextcontext != NULL);

  if (GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->make_uncurrent)
    GDK_GLEXT_CONTEXT_IMPL_GET_CLASS (glextcontext->impl)->make_uncurrent(glextcontext);
}

/**
 * gdk_glext_context_get_current:
 *
 * Returns the current #GdkGLExtContext.
 *
 * Return value: the current #GdkGLExtContext or NULL if there is no current
 *               context.
 **/
GdkGLExtContext *
gdk_glext_context_get_current ()
{
  /*
   * There can only be one current context. So we try each target
   * and take the first non-NULL result. Hopefully the underlying
   * GL implementation behaves accordingly. But we probalby need
   * a better strategy here.
   */

  GdkGLExtContext *current = NULL;

#ifdef GDKGLEXT_WINDOWING_X11
    current = _gdk_x11_glext_context_impl_get_current();
#endif
#ifdef GDKGLEXT_WINDOWING_WIN32
  if (current == NULL)
    {
      current = _gdk_win32_glext_context_impl_get_current();
    }
#endif

  return current;
}
