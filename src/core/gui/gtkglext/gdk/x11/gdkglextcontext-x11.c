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

#include "../gdkglextcontext.h"
#include "gdkglextcontext-x11.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdk.h>            /* for gdk_error_trap_(push|pop) () */

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglconfig-x11.h"
#include "gdkglwindow-x11.h"

typedef struct _GdkGLConfig   GdkGLConfig;

static void          gdk_glext_context_insert (GdkGLExtContext *glextcontext);
static void          gdk_glext_context_remove (GdkGLExtContext *glextcontext);
static GdkGLExtContext *gdk_glext_context_lookup (GLXContext    glxcontext);

static gboolean       _gdk_x11_glext_context_impl_copy             (GdkGLExtContext  *glextcontext,
                                                                 GdkGLExtContext  *src,
                                                                 unsigned long  mask);
static GdkGLDrawable* _gdk_x11_glext_context_impl_get_gl_drawable  (GdkGLExtContext *glextcontext);
static GdkGLConfig*   _gdk_x11_glext_context_impl_get_gl_config    (GdkGLExtContext *glextcontext);
static GdkGLExtContext*  _gdk_x11_glext_context_impl_get_share_list   (GdkGLExtContext *glextcontext);
static gboolean       _gdk_x11_glext_context_impl_is_direct        (GdkGLExtContext *glextcontext);
static int            _gdk_x11_glext_context_impl_get_render_type  (GdkGLExtContext *glextcontext);
static gboolean       _gdk_x11_glext_context_impl_make_current     (GdkGLExtContext  *glextcontext,
                                                                 GdkGLDrawable *draw,
                                                                 GdkGLDrawable *read);
static GLXContext     _gdk_x11_glext_context_impl_get_glxcontext   (GdkGLExtContext *glextcontext);

G_DEFINE_TYPE (GdkGLExtContextImplX11,             \
               gdk_glext_context_impl_x11,         \
               GDK_TYPE_GLEXT_CONTEXT_IMPL)

static void
gdk_glext_context_impl_x11_init (GdkGLExtContextImplX11 *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  self->glxcontext = NULL;
  self->share_list = NULL;
  self->is_direct = FALSE;
  self->render_type = 0;
  self->glconfig = NULL;
  self->gldrawable = NULL;
  self->gldrawable_read = NULL;
  self->is_destroyed = 0;
  self->is_foreign = 0;
}

void
_gdk_glext_context_destroy (GdkGLExtContext *glextcontext)
{
  GdkGLExtContextImplX11 *impl = GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl);
  Display *xdisplay;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->is_destroyed)
    return;

  gdk_glext_context_remove (glextcontext);

  xdisplay = GDK_GL_CONFIG_XDISPLAY (impl->glconfig);

  if (impl->glxcontext == glXGetCurrentContext ())
    {
      glXWaitGL ();

      GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");
      glXMakeCurrent (xdisplay, None, NULL);
    }

  if (!impl->is_foreign)
    {
      GDK_GL_NOTE_FUNC_IMPL ("glXDestroyContext");
      glXDestroyContext (xdisplay, impl->glxcontext);
      impl->glxcontext = NULL;
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
gdk_glext_context_impl_x11_finalize (GObject *object)
{
  GdkGLExtContextImplX11 *impl = GDK_GLEXT_CONTEXT_IMPL_X11 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  _gdk_glext_context_destroy (GDK_GLEXT_CONTEXT (object));

  g_object_unref (G_OBJECT (impl->glconfig));

  if (impl->share_list != NULL)
    g_object_unref (G_OBJECT (impl->share_list));

  G_OBJECT_CLASS (gdk_glext_context_impl_x11_parent_class)->finalize (object);
}

static void
gdk_glext_context_impl_x11_class_init (GdkGLExtContextImplX11Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  klass->get_glxcontext = _gdk_x11_glext_context_impl_get_glxcontext;

  klass->parent_class.copy_glext_context_impl = _gdk_x11_glext_context_impl_copy;
  klass->parent_class.get_gl_drawable = _gdk_x11_glext_context_impl_get_gl_drawable;
  klass->parent_class.get_gl_config   = _gdk_x11_glext_context_impl_get_gl_config;
  klass->parent_class.get_share_list  = _gdk_x11_glext_context_impl_get_share_list;
  klass->parent_class.is_direct       = _gdk_x11_glext_context_impl_is_direct;
  klass->parent_class.get_render_type = _gdk_x11_glext_context_impl_get_render_type;
  klass->parent_class.make_current    = _gdk_x11_glext_context_impl_make_current;
  klass->parent_class.make_uncurrent  = NULL;

  object_class->finalize = gdk_glext_context_impl_x11_finalize;
}

static GdkGLExtContextImpl *
gdk_x11_glext_context_impl_new_common (GdkGLExtContext  *glextcontext,
                                    GdkGLConfig   *glconfig,
                                    GdkGLExtContext  *share_list,
                                    int            render_type,
                                    GLXContext     glxcontext,
                                    gboolean       is_foreign)
{
  GdkGLExtContextImpl    *impl;
  GdkGLExtContextImplX11 *x11_impl;

  Display *xdisplay;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Instantiate the GdkGLExtContextImplX11 object.
   */

  impl = g_object_new (GDK_TYPE_GLEXT_CONTEXT_IMPL_X11, NULL);
  x11_impl = GDK_GLEXT_CONTEXT_IMPL_X11 (impl);

  x11_impl->glxcontext = glxcontext;

  if (share_list != NULL && GDK_IS_GLEXT_CONTEXT (share_list))
    {
      x11_impl->share_list = share_list;
      g_object_ref (G_OBJECT (x11_impl->share_list));
    }
  else
    {
      x11_impl->share_list = NULL;
    }

  xdisplay = GDK_GL_CONFIG_XDISPLAY (glconfig);
  x11_impl->is_direct = glXIsDirect (xdisplay, glxcontext) ? TRUE : FALSE;

  x11_impl->render_type = render_type;

  x11_impl->glconfig = glconfig;
  g_object_ref (G_OBJECT (x11_impl->glconfig));

  x11_impl->gldrawable = NULL;
  x11_impl->gldrawable_read = NULL;

  x11_impl->is_foreign = is_foreign;

  x11_impl->is_destroyed = FALSE;

  glextcontext->impl = impl;

  /*
   * Insert into the GL context hash table.
   */

  gdk_glext_context_insert (glextcontext);

  return impl;
}

/*< private >*/
GdkGLExtContextImpl *
_gdk_x11_glext_context_impl_new (GdkGLExtContext  *glextcontext,
                              GdkGLDrawable *gldrawable,
                              GdkGLExtContext  *share_list,
                              gboolean       direct,
                              int            render_type)
{
  GdkGLConfig *glconfig;
  GdkGLExtContextImplX11 *share_impl = NULL;
  GLXContext share_glxcontext = NULL;

  Display *xdisplay;
  XVisualInfo *xvinfo;
  GLXContext glxcontext;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Create an OpenGL rendering context.
   */

  glconfig = gdk_gl_drawable_get_gl_config (gldrawable);

  xdisplay = GDK_GL_CONFIG_XDISPLAY (glconfig); // segfault
  xvinfo = GDK_GL_CONFIG_XVINFO (glconfig);

  if (share_list != NULL && GDK_IS_GLEXT_CONTEXT (share_list))
    {
      share_impl = GDK_GLEXT_CONTEXT_IMPL_X11 (share_list->impl);
      share_glxcontext = share_impl->glxcontext;
    }

  GDK_GL_NOTE_FUNC_IMPL ("glXCreateContext");

  if (_gdk_glext_context_force_indirect)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Force indirect"));

      glxcontext = glXCreateContext (xdisplay,
                                     xvinfo,
                                     share_glxcontext,
                                     False);
    }
  else
    {
      glxcontext = glXCreateContext (xdisplay,
                                     xvinfo,
                                     share_glxcontext,
                                     (direct == TRUE) ? True : False);
    }
  if (glxcontext == NULL)
    return NULL;

  GDK_GL_NOTE (MISC,
    g_message (" -- Context: screen number = %d", xvinfo->screen));
  GDK_GL_NOTE (MISC,
    g_message (" -- Context: visual id = 0x%lx", xvinfo->visualid));

  /*
   * Instantiate the GdkGLExtContextImplX11 object.
   */

  return gdk_x11_glext_context_impl_new_common (glextcontext,
                                             glconfig,
                                             share_list,
                                             render_type,
                                             glxcontext,
                                             FALSE);
}

GdkGLExtContextImpl *
_gdk_x11_glext_context_impl_new_from_glxcontext (GdkGLExtContext *glextcontext,
                                              GdkGLConfig  *glconfig,
                                              GdkGLExtContext *share_list,
                                              GLXContext    glxcontext)
{
  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);
  g_return_val_if_fail (glxcontext != NULL, NULL);

  /*
   * Instantiate the GdkGLExtContextImplX11 object.
   */

  return gdk_x11_glext_context_impl_new_common (glextcontext,
                                             glconfig,
                                             share_list,
                                             GDK_GL_RGBA_TYPE,
                                             glxcontext,
                                             TRUE);
}

static gboolean
_gdk_x11_glext_context_impl_copy (GdkGLExtContext  *glextcontext,
                               GdkGLExtContext  *src,
                               unsigned long  mask)
{
  GLXContext dst_glxcontext, src_glxcontext;
  GdkGLConfig *glconfig;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (glextcontext), FALSE);
  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (src), FALSE);

  dst_glxcontext = GDK_GLEXT_CONTEXT_GLXCONTEXT (glextcontext);
  if (dst_glxcontext == NULL)
    return FALSE;

  src_glxcontext = GDK_GLEXT_CONTEXT_GLXCONTEXT (src);
  if (src_glxcontext == NULL)
    return FALSE;

  glconfig = GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl)->glconfig;

  gdk_error_trap_push ();

  glXCopyContext (GDK_GL_CONFIG_XDISPLAY (glconfig),
                  src_glxcontext, dst_glxcontext,
                  mask);

  return gdk_error_trap_pop () == Success;
}

/*< private >*/
void
_gdk_x11_glext_context_impl_set_gl_drawable (GdkGLExtContext  *glextcontext,
                                          GdkGLDrawable *gldrawable)
{
  GdkGLExtContextImplX11 *impl = GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl);

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
  GdkGLExtContextImplX11 *impl = GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext);

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
_gdk_x11_glext_context_impl_get_gl_drawable (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl)->gldrawable;
}

static GdkGLConfig *
_gdk_x11_glext_context_impl_get_gl_config (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl)->glconfig;
}

static GdkGLExtContext *
_gdk_x11_glext_context_impl_get_share_list (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl)->share_list;
}

static gboolean
_gdk_x11_glext_context_impl_is_direct (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (glextcontext), FALSE);

  return GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl)->is_direct;
}

static int
_gdk_x11_glext_context_impl_get_render_type (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (glextcontext), 0);

  return GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl)->render_type;
}

static gboolean
_gdk_x11_glext_context_impl_make_current (GdkGLExtContext  *glextcontext,
                                       GdkGLDrawable *draw,
                                       GdkGLDrawable *read)
{
  GdkGLWindow *glwindow;
  GdkGLWindowImplX11 *x11_impl;
  GdkGLConfig *glconfig;
  GdkWindow *window;
  Window glxwindow;
  GLXContext glxcontext;

  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (glextcontext), FALSE);
  g_return_val_if_fail (GDK_IS_X11_GL_WINDOW (draw), FALSE);

  glwindow = GDK_GL_WINDOW(draw);
  x11_impl = GDK_GL_WINDOW_IMPL_X11 (glwindow->impl);
  glconfig = x11_impl->glconfig;
  window = gdk_gl_window_get_window(glwindow);
  glxwindow = x11_impl->glxwindow;
  glxcontext = GDK_GLEXT_CONTEXT_GLXCONTEXT (glextcontext);

  if (glxwindow == None || glxcontext == NULL)
    return FALSE;

  GDK_GL_NOTE (MISC,
    g_message (" -- Window: screen number = %d",
      GDK_SCREEN_XNUMBER (gdk_window_get_screen (window))));
  GDK_GL_NOTE (MISC,
    g_message (" -- Window: visual id = 0x%lx",
      GDK_VISUAL_XVISUAL (gdk_window_get_visual (window))->visualid));

  GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");

  if (!glXMakeCurrent (GDK_GL_CONFIG_XDISPLAY (glconfig), glxwindow, glxcontext))
    {
      g_warning ("glXMakeCurrent() failed");
      _gdk_x11_glext_context_impl_set_gl_drawable (glextcontext, NULL);
      /* currently unused. */
      /* _gdk_glext_context_set_gl_drawable_read (glextcontext, NULL); */
      return FALSE;
    }

  _gdk_x11_glext_context_impl_set_gl_drawable (glextcontext, draw);
  /* currently unused. */
  /* _gdk_glext_context_set_gl_drawable_read (glextcontext, read); */

  if (_GDK_GL_CONFIG_AS_SINGLE_MODE (glconfig))
    {
      /* We do this because we are treating a double-buffered frame
         buffer as a single-buffered frame buffer because the system
         does not appear to export any suitable single-buffered
         visuals (in which the following are necessary). */
      glDrawBuffer (GL_FRONT);
      glReadBuffer (GL_FRONT);
    }

  GDK_GL_NOTE (MISC, _gdk_gl_print_gl_info ());

  return TRUE;
}

GdkGLExtContext *
_gdk_x11_glext_context_impl_get_current (void)
{
  static GdkGLExtContext *current = NULL;
  GLXContext glxcontext;

  GDK_GL_NOTE_FUNC ();

  glxcontext = glXGetCurrentContext ();

  if (glxcontext == NULL)
    return NULL;

  if (current && GDK_GLEXT_CONTEXT_GLXCONTEXT (current) == glxcontext)
    return current;

  current = gdk_glext_context_lookup (glxcontext);

  return current;
}

GLXContext
_gdk_x11_glext_context_impl_get_glxcontext (GdkGLExtContext *glextcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GLEXT_CONTEXT (glextcontext), NULL);

  return GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl)->glxcontext;
}

/*
 * GdkGLExtContext hash table.
 */

static GHashTable *glext_context_ht = NULL;

static void
gdk_glext_context_insert (GdkGLExtContext *glextcontext)
{
  GdkGLExtContextImplX11 *impl;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (glext_context_ht == NULL)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Create GL context hash table."));

      /* We do not know the storage type of GLXContext from the GLX
         specification. We assume that it is a pointer as NULL values
         are specified for this type---this is consistent with the SGI
         and Mesa GLX implementations. */
      glext_context_ht = g_hash_table_new (g_direct_hash,
                                        g_direct_equal);
    }

  impl = GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl);

  g_hash_table_insert (glext_context_ht, impl->glxcontext, glextcontext);
}

static void
gdk_glext_context_remove (GdkGLExtContext *glextcontext)
{
  GdkGLExtContextImplX11 *impl;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (glext_context_ht == NULL)
    return;

  impl = GDK_GLEXT_CONTEXT_IMPL_X11 (glextcontext->impl);

  g_hash_table_remove (glext_context_ht, impl->glxcontext);

  if (g_hash_table_size (glext_context_ht) == 0)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Destroy GL context hash table."));
      g_hash_table_destroy (glext_context_ht);
      glext_context_ht = NULL;
    }
}

static GdkGLExtContext *
gdk_glext_context_lookup (GLXContext glxcontext)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (glext_context_ht == NULL)
    return NULL;

  return g_hash_table_lookup (glext_context_ht, glxcontext);
}
