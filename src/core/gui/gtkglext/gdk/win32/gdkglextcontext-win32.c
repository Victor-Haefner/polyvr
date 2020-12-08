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

#include "../gdkglextcontext.h"
#include "gdkglwin32.h"
#include "gdkglprivate-win32.h"
#include "gdkglconfig-win32.h"
#include "gdkglwindow-win32.h"
#include "gdkglextcontext-win32.h"

static void          gdk_glext_context_insert (GdkGLExtContext *glextcontext);
static void          gdk_glext_context_remove (GdkGLExtContext *glextcontext);
static GdkGLExtContext *gdk_glext_context_lookup (HGLRC         hglrc);

static gboolean        _gdk_win32_glext_context_impl_copy (GdkGLExtContext  *glextcontext,
                                                        GdkGLExtContext  *src,
                                                        unsigned long  mask);
static GdkGLDrawable  *_gdk_win32_glext_context_impl_get_gl_drawable (GdkGLExtContext *glextcontext);
static GdkGLConfig    *_gdk_win32_glext_context_impl_get_gl_config (GdkGLExtContext *glextcontext);
static GdkGLExtContext   *_gdk_win32_glext_context_impl_get_share_list (GdkGLExtContext *glextcontext);
static gboolean        _gdk_win32_glext_context_impl_is_direct (GdkGLExtContext *glextcontext);
static int             _gdk_win32_glext_context_impl_get_render_type (GdkGLExtContext *glextcontext);
static gboolean        _gdk_win32_glext_context_impl_make_current (GdkGLExtContext  *glextcontext,
                                                                GdkGLDrawable *draw,
                                                                GdkGLDrawable *read);
static void            _gdk_win32_glext_context_impl_make_uncurrent (GdkGLExtContext *glextcontext);
static HGLRC           _gdk_win32_glext_context_impl_get_hglrc      (GdkGLExtContext *glextcontext);

G_DEFINE_TYPE (GdkGLExtContextImplWin32,              \
               gdk_glext_context_impl_win32,          \
               GDK_TYPE_GLEXT_CONTEXT_IMPL)

static void
gdk_glext_context_impl_win32_init (GdkGLExtContextImplWin32 *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  self->hglrc = NULL;
  self->share_list = NULL;
  self->render_type = 0;
  self->glconfig = NULL;
  self->gldrawable = NULL;
  self->gldrawable_read = NULL;
  self->is_destroyed = 0;
  self->is_foreign = 0;
}

static void
_gdk_win32_glext_context_impl_destroy (GdkGLExtContext *glextcontext)
{
  GdkGLExtContextImplWin32 *impl = GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->is_destroyed)
    return;

  gdk_glext_context_remove (glextcontext);

  if (impl->hglrc == wglGetCurrentContext ())
    {
      glFinish ();

      GDK_GL_NOTE_FUNC_IMPL ("wglMakeCurrent");
      wglMakeCurrent (NULL, NULL);
    }

  if (!impl->is_foreign)
    {
      GDK_GL_NOTE_FUNC_IMPL ("wglDeleteContext");
      wglDeleteContext (impl->hglrc);
      impl->hglrc = NULL;
    }

  if (impl->gldrawable != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (impl->gldrawable),
                                    (gpointer *) &(impl->gldrawable));
      impl->gldrawable = NULL;
    }

  /* currently unused. */
  /*
  if (impl->gldrawable_read != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (impl->gldrawable_read),
                                    (gpointer *) &(impl->gldrawable_read));
      impl->gldrawable_read = NULL;
    }
  */

  impl->is_destroyed = TRUE;
}

static void
gdk_glext_context_impl_win32_finalize (GObject *object)
{
  GdkGLExtContextImplWin32 *impl = GDK_GLEXT_CONTEXT_IMPL_WIN32 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  _gdk_win32_glext_context_impl_destroy (GDK_GLEXT_CONTEXT (object));

  g_object_unref (G_OBJECT (impl->glconfig));

  if (impl->share_list != NULL)
    g_object_unref (G_OBJECT (impl->share_list));

  G_OBJECT_CLASS (gdk_glext_context_impl_win32_parent_class)->finalize (object);
}

static void
gdk_glext_context_impl_win32_class_init (GdkGLExtContextImplWin32Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  klass->get_hglrc = _gdk_win32_glext_context_impl_get_hglrc;

  klass->parent_class.copy_glext_context_impl = _gdk_win32_glext_context_impl_copy;
  klass->parent_class.get_gl_drawable = _gdk_win32_glext_context_impl_get_gl_drawable;
  klass->parent_class.get_gl_config   = _gdk_win32_glext_context_impl_get_gl_config;
  klass->parent_class.get_share_list  = _gdk_win32_glext_context_impl_get_share_list;
  klass->parent_class.is_direct       = _gdk_win32_glext_context_impl_is_direct;
  klass->parent_class.get_render_type = _gdk_win32_glext_context_impl_get_render_type;
  klass->parent_class.make_current    = _gdk_win32_glext_context_impl_make_current;
  klass->parent_class.make_uncurrent  = _gdk_win32_glext_context_impl_make_uncurrent;

  object_class->finalize = gdk_glext_context_impl_win32_finalize;
}

static GdkGLExtContextImpl *
gdk_win32_glext_context_impl_new_common (GdkGLExtContext  *glextcontext,
                                      GdkGLConfig   *glconfig,
                                      GdkGLExtContext  *share_list,
                                      int            render_type,
                                      HGLRC          hglrc,
                                      gboolean       is_foreign)
{
  GdkGLExtContextImpl *impl;
  GdkGLExtContextImplWin32 *win32_impl;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Instantiate the GdkGLExtContextImplWin32 object.
   */

  impl = g_object_new (GDK_TYPE_GLEXT_CONTEXT_IMPL_WIN32, NULL);
  win32_impl = GDK_GLEXT_CONTEXT_IMPL_WIN32 (impl);

  win32_impl->hglrc = hglrc;

  if (share_list != NULL && GDK_IS_GLEXT_CONTEXT (share_list))
    {
      win32_impl->share_list = share_list;
      g_object_ref (G_OBJECT (win32_impl->share_list));
    }
  else
    {
      win32_impl->share_list = NULL;
    }

  win32_impl->render_type = render_type;

  win32_impl->glconfig = glconfig;
  g_object_ref (G_OBJECT (win32_impl->glconfig));

  win32_impl->gldrawable = NULL;
  win32_impl->gldrawable_read = NULL;

  win32_impl->is_foreign = is_foreign;

  win32_impl->is_destroyed = FALSE;

  glextcontext->impl = impl;

  /*
   * Insert into the GL context hash table.
   */

  gdk_glext_context_insert (glextcontext);

  return impl;
}

/*< private >*/
GdkGLExtContextImpl *
_gdk_win32_glext_context_impl_new (GdkGLExtContext  *glextcontext,
                                GdkGLDrawable *gldrawable,
                                GdkGLExtContext  *share_list,
                                gboolean       direct,
                                int            render_type)
{
  GdkGLConfig *glconfig;
  HDC hdc;
  HGLRC hglrc;
  GdkGLExtContextImplWin32 *share_impl = NULL;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Create an OpenGL rendering context.
   */

  glconfig = gdk_gl_drawable_get_gl_config (gldrawable);

  /* Get DC. */
  hdc = gdk_win32_gl_window_get_hdc (GDK_GL_WINDOW (gldrawable));
  if (hdc == NULL)
    return NULL;

  GDK_GL_NOTE_FUNC_IMPL ("wglCreateContext");

  hglrc = wglCreateContext (hdc);

  /* Release DC. */
  gdk_win32_gl_window_release_hdc (GDK_GL_WINDOW (gldrawable));

  if (hglrc == NULL)
    return NULL;

  if (share_list != NULL && GDK_IS_GLEXT_CONTEXT (share_list))
    {
      GDK_GL_NOTE_FUNC_IMPL ("wglShareLists");

      share_impl = GDK_GLEXT_CONTEXT_IMPL_WIN32 (share_list);
      if (!wglShareLists (share_impl->hglrc, hglrc))
        {
          wglDeleteContext (hglrc);
          return NULL;
        }
    }

  /*
   * Instantiate the GdkGLExtContextImplWin32 object.
   */

  return gdk_win32_glext_context_impl_new_common (glextcontext,
                                               glconfig,
                                               share_list,
                                               render_type,
                                               hglrc,
                                               FALSE);
}

GdkGLExtContextImpl *
_gdk_win32_glext_context_impl_new_from_hglrc (GdkGLExtContext *glextcontext,
                                           GdkGLConfig  *glconfig,
                                           GdkGLExtContext *share_list,
                                           HGLRC         hglrc)
{
  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_WIN32_GL_CONFIG (glconfig), NULL);
  g_return_val_if_fail (hglrc != NULL, NULL);

  /*
   * Instantiate the GdkGLExtContextImplWin32 object.
   */

  return gdk_win32_glext_context_impl_new_common (glextcontext,
                                               glconfig,
                                               share_list,
                                               GDK_GL_RGBA_TYPE,
                                               hglrc,
                                               TRUE);
}

static gboolean
_gdk_win32_glext_context_impl_copy (GdkGLExtContext  *glextcontext,
                                 GdkGLExtContext  *src,
                                 unsigned long  mask)
{
  HGLRC dst_hglrc, src_hglrc;

  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext->impl), FALSE);
  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (src), FALSE);

  dst_hglrc = GDK_GLEXT_CONTEXT_HGLRC (glextcontext);
  if (dst_hglrc == NULL)
    return FALSE;

  src_hglrc = GDK_GLEXT_CONTEXT_HGLRC (src);
  if (src_hglrc == NULL)
    return FALSE;

  GDK_GL_NOTE_FUNC_IMPL ("wglCopyContext");

  return wglCopyContext (src_hglrc, dst_hglrc, mask);
}

/*< private >*/
void
_gdk_glext_context_set_gl_drawable (GdkGLExtContext  *glextcontext,
                                 GdkGLDrawable *gldrawable)
{
  GdkGLExtContextImplWin32 *impl = GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->gldrawable == gldrawable)
    return;

  if (impl->gldrawable != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (impl->gldrawable),
                                    (gpointer *) &(impl->gldrawable));
      impl->gldrawable = NULL;
    }

  if (gldrawable != NULL && GDK_IS_GL_DRAWABLE (gldrawable))
    {
      impl->gldrawable = gldrawable;
      g_object_add_weak_pointer (G_OBJECT (impl->gldrawable),
                                 (gpointer *) &(impl->gldrawable));
    }
}

/*< private >*/
/* currently unused. */
/*
void
_gdk_glext_context_set_gl_drawable_read (GdkGLExtContext  *glextcontext,
                                      GdkGLDrawable *gldrawable_read)
{
  GdkGLExtContextImplWin32 *impl = GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->gldrawable_read == gldrawable_read)
    return;

  if (impl->gldrawable_read != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (impl->gldrawable_read),
                                    (gpointer *) &(impl->gldrawable_read));
      impl->gldrawable_read = NULL;
    }

  if (gldrawable_read != NULL && GDK_IS_GL_DRAWABLE (gldrawable_read))
    {
      impl->gldrawable_read = gldrawable_read;
      g_object_add_weak_pointer (G_OBJECT (impl->gldrawable_read),
                                 (gpointer *) &(impl->gldrawable_read));
    }
}
*/

static GdkGLDrawable *
_gdk_win32_glext_context_impl_get_gl_drawable (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl)->gldrawable;
}

static GdkGLConfig *
_gdk_win32_glext_context_impl_get_gl_config (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl)->glconfig;
}

static GdkGLExtContext *
_gdk_win32_glext_context_impl_get_share_list (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl)->share_list;
}

static gboolean
_gdk_win32_glext_context_impl_is_direct (GdkGLExtContext *glextcontext)
{
  return FALSE;
}

static int
_gdk_win32_glext_context_impl_get_render_type (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext), 0);

  return GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl)->render_type;
}

static gboolean
_gdk_win32_glext_context_impl_make_current (GdkGLExtContext *glextcontext,
                                         GdkGLDrawable *draw,
                                         GdkGLDrawable *read)
{
  GdkGLWindowImplWin32 *impl;
  HDC hdc;
  HGLRC hglrc;

  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext), FALSE);
  g_return_val_if_fail (GDK_IS_WIN32_GL_WINDOW (draw), FALSE);

  if (GDK_GL_WINDOW_IS_DESTROYED (GDK_GL_WINDOW (draw)) ||
      GDK_GLEXT_CONTEXT_IS_DESTROYED (glextcontext))
    return FALSE;

  impl = GDK_GL_WINDOW_IMPL_WIN32 (GDK_GL_WINDOW (draw)->impl);

  /* Get DC. */
  hdc = GDK_GL_WINDOW_IMPL_WIN32_HDC_GET (impl);

  /* Get GLRC. */
  hglrc = GDK_GLEXT_CONTEXT_HGLRC (glextcontext);

  GDK_GL_NOTE_FUNC_IMPL ("wglMakeCurrent");

  if (!wglMakeCurrent (hdc, hglrc))
    {
      g_warning ("wglMakeCurrent() failed");
      _gdk_glext_context_set_gl_drawable (glextcontext, NULL);
      /* currently unused. */
      /* _gdk_glext_context_set_gl_drawable_read (glextcontext, NULL); */
      return FALSE;
    }

  _gdk_glext_context_set_gl_drawable (glextcontext, draw);
  /* currently unused. */
  /* _gdk_glext_context_set_gl_drawable_read (glextcontext, read); */

  if (_GDK_GL_CONFIG_AS_SINGLE_MODE (impl->glconfig))
    {
      /* We do this because we are treating a double-buffered frame
         buffer as a single-buffered frame buffer because the system
         does not appear to export any suitable single-buffered
         visuals (in which the following are necessary). */
      glDrawBuffer (GL_FRONT);
      glReadBuffer (GL_FRONT);
    }

  GDK_GL_NOTE (MISC, _gdk_gl_print_gl_info ());

  /*
   * Do *NOT* release DC.
   *
   * With some graphics card, DC owned by rendering thread will be needed.
   */

  return TRUE;
}

static void
_gdk_win32_glext_context_impl_make_uncurrent (GdkGLExtContext *glextcontext)
{
  GdkGLDrawable *gldrawable = _gdk_win32_glext_context_impl_get_gl_drawable (glextcontext);

  g_return_if_fail(gldrawable != NULL);

  GdkGLWindowImplWin32 *impl = GDK_GL_WINDOW_IMPL_WIN32 ( GDK_GL_WINDOW (gldrawable)->impl);

  /* Release DC. */
  GDK_GL_WINDOW_IMPL_WIN32_HDC_RELEASE (impl);
}

GdkGLExtContext *
_gdk_win32_glext_context_impl_get_current (void)
{
  static GdkGLExtContext *current = NULL;
  HGLRC hglrc;

  GDK_GL_NOTE_FUNC ();

  hglrc = wglGetCurrentContext ();

  if (hglrc == NULL)
    return NULL;

  if (current && GDK_GLEXT_CONTEXT_HGLRC (current) == hglrc)
    return current;

  current = gdk_glext_context_lookup (hglrc);

  return current;
}

static HGLRC
_gdk_win32_glext_context_impl_get_hglrc (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl)->hglrc;
}

/*
 * GdkGLExtContext hash table.
 */

static GHashTable *glext_context_ht = NULL;

static void
gdk_glext_context_insert (GdkGLExtContext *glextcontext)
{
  GdkGLExtContextImplWin32 *impl;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  g_return_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext));

  if (glext_context_ht == NULL)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Create GL context hash table."));

      /* We do not know the storage type of HGLRC. We assume that it is
         a pointer as NULL values are specified for this type. */
      glext_context_ht = g_hash_table_new (g_direct_hash,
                                        g_direct_equal);
    }

  impl = GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl);

  g_hash_table_insert (glext_context_ht, impl->hglrc, glextcontext);
}

static void
gdk_glext_context_remove (GdkGLExtContext *glextcontext)
{
  GdkGLExtContextImplWin32 *impl;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  g_return_if_fail (GDK_IS_WIN32_GLEXT_CONTEXT (glextcontext));

  if (glext_context_ht == NULL)
    return;

  impl = GDK_GLEXT_CONTEXT_IMPL_WIN32 (glextcontext->impl);

  g_hash_table_remove (glext_context_ht, impl->hglrc);

  if (g_hash_table_size (glext_context_ht) == 0)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Destroy GL context hash table."));
      g_hash_table_destroy (glext_context_ht);
      glext_context_ht = NULL;
    }
}

static GdkGLExtContext *
gdk_glext_context_lookup (HGLRC hglrc)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (glext_context_ht == NULL)
    return NULL;

  return g_hash_table_lookup (glext_context_ht, hglrc);
}
