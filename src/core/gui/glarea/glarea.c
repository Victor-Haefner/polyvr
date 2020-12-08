/* GTK - The GIMP Toolkit
 *
 * gtkglarea.c: A GL drawing area
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

//#include "config.h"

#define GTK_COMPILATION

#include "glarea.h"
//#include <gtk/gtkintl.h>
#include <gtk/gtkstylecontext.h>
//#include <gtk/gtkmarshalers.h>
//#include <gtk/gtkprivate.h>
#include <gtk/gtkrender.h>
#include <gobject/gtype.h>
#include <gobject/gvalue.h>
#include <gobject/gsignal.h>

#ifdef _WIN32
#include <windows.h>
#include <GL/GL.h>
#define APIENTRY __stdcall

typedef void(APIENTRY _glBindFramebuffer) (GLenum target, GLuint framebuffer);
typedef void(APIENTRY _glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
typedef void(APIENTRY _glGenFramebuffers) (GLsizei n, GLuint* ids);
typedef void(APIENTRY _glGenRenderbuffers) (GLsizei n, GLuint* ids);
typedef void(APIENTRY _glDeleteRenderbuffers) (GLsizei n, GLuint* renderbuffers);
typedef void(APIENTRY _glDeleteFramebuffers) (GLsizei n, GLuint* framebuffers);
typedef void(APIENTRY _glTexImage2DMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
typedef void(APIENTRY _glRenderbufferStorageMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void(APIENTRY _glRenderbufferStorage) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void(APIENTRY _glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void(APIENTRY _glFramebufferRenderbuffer) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef GLenum(APIENTRY _glCheckFramebufferStatus) (GLenum target);
_glBindFramebuffer* glBindFramebuffer = 0;
_glBindRenderbuffer* glBindRenderbuffer = 0;
_glGenFramebuffers* glGenFramebuffers = 0;
_glGenRenderbuffers* glGenRenderbuffers = 0;
_glDeleteRenderbuffers* glDeleteRenderbuffers = 0;
_glDeleteFramebuffers* glDeleteFramebuffers = 0;
_glTexImage2DMultisample* glTexImage2DMultisample = 0;
_glRenderbufferStorageMultisample* glRenderbufferStorageMultisample = 0;
_glRenderbufferStorage* glRenderbufferStorage = 0;
_glFramebufferTexture2D* glFramebufferTexture2D = 0;
_glFramebufferRenderbuffer* glFramebufferRenderbuffer = 0;
_glCheckFramebufferStatus* glCheckFramebufferStatus = 0;

void initGLFunctions() {
    glBindFramebuffer = (_glBindFramebuffer*)wglGetProcAddress("glBindFramebuffer");
    glBindRenderbuffer = (_glBindRenderbuffer*)wglGetProcAddress("glBindRenderbuffer");
    glGenFramebuffers = (_glGenFramebuffers*)wglGetProcAddress("glGenFramebuffers");
    glGenRenderbuffers = (_glGenRenderbuffers*)wglGetProcAddress("glGenRenderbuffers");
    glDeleteRenderbuffers = (_glDeleteRenderbuffers*)wglGetProcAddress("glDeleteRenderbuffers");
    glDeleteFramebuffers = (_glDeleteFramebuffers*)wglGetProcAddress("glDeleteFramebuffers");
    glTexImage2DMultisample = (_glTexImage2DMultisample*)wglGetProcAddress("glTexImage2DMultisample");
    glRenderbufferStorageMultisample = (_glRenderbufferStorageMultisample*)wglGetProcAddress("glRenderbufferStorageMultisample");
    glRenderbufferStorage = (_glRenderbufferStorage*)wglGetProcAddress("glRenderbufferStorage");
    glFramebufferTexture2D = (_glFramebufferTexture2D*)wglGetProcAddress("glFramebufferTexture2D");
    glFramebufferRenderbuffer = (_glFramebufferRenderbuffer*)wglGetProcAddress("glFramebufferRenderbuffer");
    glCheckFramebufferStatus = (_glCheckFramebufferStatus*)wglGetProcAddress("glCheckFramebufferStatus");
}
#else
#include <GL/gl.h>
#include <GL/glx.h>
//#define APIENTRY GLAPIENTRY
void initGLFunctions() {}

typedef struct {
    GObject parent_instance;

    GLXContext glx_context;
    GLXFBConfig glx_config;
    GLXDrawable drawable;

    guint is_attached : 1;
    guint is_direct : 1;
    guint do_frame_sync : 1;
    guint do_blit_swap : 1;
} _GdkX11GLContext;

typedef enum {
  GDK_RENDERING_MODE_SIMILAR = 0,
  GDK_RENDERING_MODE_IMAGE,
  GDK_RENDERING_MODE_RECORDING
} GdkRenderingMode;

typedef struct {
  GObject parent_instance;

  void* impl; /* window-system-specific delegate object */  // GdkWindowImpl*

  GdkWindow *parent;
  GdkWindow *transient_for;
  GdkVisual *visual;

  gpointer user_data;

  gint x;
  gint y;

  GdkEventMask event_mask;
  guint8 window_type;

  guint8 depth;
  guint8 resize_count;

  gint8 toplevel_window_type;

  GList *filters;
  GList *children;
  GList children_list_node;
  GList *native_children;


  cairo_pattern_t *background;

  struct {
    cairo_surface_t *surface;

    cairo_region_t *region;
    cairo_region_t *flushed_region;
    cairo_region_t *need_blend_region;

    gboolean surface_needs_composite;
    gboolean use_gl;
  } current_paint;
  GdkGLContext *gl_paint_context;

  cairo_region_t *update_area;
  guint update_freeze_count;
  /* This is the update_area that was in effect when the current expose
     started. It may be smaller than the expose area if we'e painting
     more than we have to, but it represents the "true" damage. */
  cairo_region_t *active_update_area;
  /* We store the old expose areas to support buffer-age optimizations */
  cairo_region_t *old_updated_area[2];

  GdkWindowState old_state;
  GdkWindowState state;

  guint8 alpha;
  guint8 fullscreen_mode;

  guint input_only : 1;
  guint pass_through : 1;
  guint modal_hint : 1;
  guint composited : 1;
  guint has_alpha_background : 1;

  guint destroyed : 2;

  guint accept_focus : 1;
  guint focus_on_map : 1;
  guint shaped : 1;
  guint support_multidevice : 1;
  guint synthesize_crossing_event_queued : 1;
  guint effective_visibility : 2;
  guint visibility : 2; /* The visibility wrt the toplevel (i.e. based on clip_region) */
  guint native_visibility : 2; /* the native visibility of a impl windows */
  guint viewable : 1; /* mapped and all parents mapped */
  guint applied_shape : 1;
  guint in_update : 1;
  guint geometry_dirty : 1;
  guint event_compression : 1;
  guint frame_clock_events_paused : 1;

  /* The GdkWindow that has the impl, ref:ed if another window.
   * This ref is required to keep the wrapper of the impl window alive
   * for as long as any GdkWindow references the impl. */
  void *impl_window; // GdkWindow*

  guint update_and_descendants_freeze_count;

  gint abs_x, abs_y; /* Absolute offset in impl */
  gint width, height;
  gint shadow_top;
  gint shadow_left;
  gint shadow_right;
  gint shadow_bottom;

  guint num_offscreen_children;

  /* The clip region is the part of the window, in window coordinates
     that is fully or partially (i.e. semi transparently) visible in
     the window hierarchy from the toplevel and down */
  cairo_region_t *clip_region;

  GdkCursor *cursor;
  GHashTable *device_cursor;

  cairo_region_t *shape;
  cairo_region_t *input_shape;

  GList *devices_inside;
  GHashTable *device_events;

  GHashTable *source_event_masks;
  gulong device_added_handler_id;
  gulong device_changed_handler_id;

  GdkFrameClock *frame_clock; /* NULL to use from parent or default */
  GdkWindowInvalidateHandlerFunc invalidate_handler;

  GdkDrawingContext *drawing_context;

  cairo_region_t *opaque_region;
} _GdkWindow;

typedef struct {
  GObject parent_instance;

  GList *queued_events;
  GList *queued_tail;

  /* Information for determining if the latest button click
   * is part of a double-click or triple-click
   */
  GHashTable *multiple_click_info;

  guint event_pause_count;       /* How many times events are blocked */

  guint closed             : 1;  /* Whether this display has been closed */

  GArray *touch_implicit_grabs;
  GHashTable *device_grabs;
  GHashTable *motion_hint_info;
  GdkDeviceManager *device_manager;
  GList *input_devices; /* Deprecated, only used to keep gdk_display_list_devices working */

  GHashTable *pointers_info;  /* GdkPointerWindowInfo for each device */
  guint32 last_event_time;    /* Last reported event time from server */

  guint double_click_time;  /* Maximum time between clicks in msecs */
  guint double_click_distance;   /* Maximum distance between clicks in pixels */

  guint has_gl_extension_texture_non_power_of_two : 1;
  guint has_gl_extension_texture_rectangle : 1;

  guint debug_updates     : 1;
  guint debug_updates_set : 1;

  GdkRenderingMode rendering_mode;

  GList *seats;
} m_GdkDisplay;

typedef struct {
  m_GdkDisplay parent_instance;
  Display *xdisplay;
  GdkScreen *screen;
  GList *screens;

  GSource *event_source;

  gint grab_count;

  /* Keyboard related information */
  gint xkb_event_type;
  gboolean use_xkb;

  /* Whether we were able to turn on detectable-autorepeat using
   * XkbSetDetectableAutorepeat. If FALSE, we'll fall back
   * to checking the next event with XPending().
   */
  gboolean have_xkb_autorepeat;

  GdkKeymap *keymap;
  guint      keymap_serial;

  gboolean have_xfixes;
  gint xfixes_event_base;

  gboolean have_xcomposite;
  gboolean have_xdamage;
  gint xdamage_event_base;

  gboolean have_randr12;
  gboolean have_randr13;
  gboolean have_randr15;
  gint xrandr_event_base;

  /* If the SECURITY extension is in place, whether this client holds
   * a trusted authorization and so is allowed to make various requests
   * (grabs, properties etc.) Otherwise always TRUE.
   */
  gboolean trusted_client;

  /* drag and drop information */
  GdkDragContext *current_dest_drag;

  /* Mapping to/from virtual atoms */
  GHashTable *atom_from_virtual;
  GHashTable *atom_to_virtual;

  /* Session Management leader window see ICCCM */
  Window leader_window;
  GdkWindow *leader_gdk_window;
  gboolean leader_window_title_set;

  /* List of functions to go from extension event => X window */
  GSList *event_types;

  /* X ID hashtable */
  GHashTable *xid_ht;

  /* translation queue */
  GQueue *translate_queue;

  /* input GdkWindow list */
  GList *input_windows;

  GPtrArray *monitors;
  int primary_monitor;

  /* Startup notification */
  gchar *startup_notification_id;

  /* Time of most recent user interaction. */
  gulong user_time;

  /* Sets of atoms for DND */
  guint base_dnd_atoms_precached : 1;
  guint xdnd_atoms_precached : 1;
  guint motif_atoms_precached : 1;
  guint use_sync : 1;

  guint have_shapes : 1;
  guint have_input_shapes : 1;
  gint shape_event_base;

  /* The offscreen window that has the pointer in it (if any) */
  GdkWindow *active_offscreen_window;

  GSList *error_traps;

  gint wm_moveresize_button;

  /* GLX information */
  gint glx_version;
  gint glx_error_base;
  gint glx_event_base;

  /* Translation between X server time and system-local monotonic time */
  gint64 server_time_query_time;
  gint64 server_time_offset;

  guint server_time_is_monotonic_time : 1;

  guint have_glx : 1;

  /* GLX extensions we check */
  guint has_glx_swap_interval : 1;
  guint has_glx_create_context : 1;
  guint has_glx_texture_from_pixmap : 1;
  guint has_glx_video_sync : 1;
  guint has_glx_buffer_age : 1;
  guint has_glx_sync_control : 1;
  guint has_glx_multisample : 1;
  guint has_glx_visual_rating : 1;
  guint has_glx_create_es2_context : 1;
} _GdkX11Display;

gboolean _gdk_gl_context_has_framebuffer_blit (GdkGLContext *context) {
    return FALSE; // TODO
  //GdkGLContextPrivate *priv = gdk_gl_context_get_instance_private (context);
  //return priv->has_gl_framebuffer_blit;
}

void _gdk_x11_window_invalidate_for_new_frame (_GdkWindow *window, cairo_region_t *update_area) {
  cairo_rectangle_int_t window_rect;
  GdkDisplay *display = gdk_window_get_display (window);
  _GdkX11Display *display_x11 = (_GdkX11Display*) (display);
  Display* dpy = display_x11->xdisplay;
  gboolean invalidate_all;

  /* Minimal update is ok if we're not drawing with gl */
  if (window->gl_paint_context == NULL) return;

  _GdkX11GLContext* context_x11 = (_GdkX11GLContext*) (window->gl_paint_context);

  unsigned int buffer_age = 0;

  context_x11->do_blit_swap = FALSE;

  if (display_x11->has_glx_buffer_age) {
      gdk_gl_context_make_current (window->gl_paint_context);
      glXQueryDrawable(dpy, context_x11->drawable, GLX_BACK_BUFFER_AGE_EXT, &buffer_age);
    }

  invalidate_all = FALSE;
  if (buffer_age >= 4) // fixed black widgets here and below
    {
      cairo_rectangle_int_t whole_window = { 0, 0, gdk_window_get_width (window), gdk_window_get_height (window) };

      if (_gdk_gl_context_has_framebuffer_blit (window->gl_paint_context) &&
          cairo_region_contains_rectangle (update_area, &whole_window) != CAIRO_REGION_OVERLAP_IN)
        {
          context_x11->do_blit_swap = TRUE;
        }
      else
        invalidate_all = TRUE;
    }
  else
    {
      if (buffer_age == 0) invalidate_all = TRUE; // fixed black widgets here and above
      if (buffer_age >= 2)
        {
          if (window->old_updated_area[0])
            cairo_region_union (update_area, window->old_updated_area[0]);
          else
            invalidate_all = TRUE;
        }
      if (buffer_age >= 3)
        {
          if (window->old_updated_area[1])
            cairo_region_union (update_area, window->old_updated_area[1]);
          else
            invalidate_all = TRUE;
        }
    }

  if (invalidate_all)
    {
      window_rect.x = 0;
      window_rect.y = 0;
      window_rect.width = gdk_window_get_width (window);
      window_rect.height = gdk_window_get_height (window);

      /* If nothing else is known, repaint everything so that the back
         buffer is fully up-to-date for the swapbuffer */
      cairo_region_union_rectangle (update_area, &window_rect);
    }

}

typedef struct {
  GObjectClass parent_class;

  cairo_surface_t *
               (* ref_cairo_surface)    (GdkWindow       *window);
  cairo_surface_t *
               (* create_similar_image_surface) (GdkWindow *     window,
                                                 cairo_format_t  format,
                                                 int             width,
                                                 int             height);

  void         (* show)                 (GdkWindow       *window,
					 gboolean         already_mapped);
  void         (* hide)                 (GdkWindow       *window);
  void         (* withdraw)             (GdkWindow       *window);
  void         (* raise)                (GdkWindow       *window);
  void         (* lower)                (GdkWindow       *window);
  void         (* restack_under)        (GdkWindow       *window,
					 GList           *native_siblings);
  void         (* restack_toplevel)     (GdkWindow       *window,
					 GdkWindow       *sibling,
					 gboolean        above);

  void         (* move_resize)          (GdkWindow       *window,
                                         gboolean         with_move,
                                         gint             x,
                                         gint             y,
                                         gint             width,
                                         gint             height);
  void         (* move_to_rect)         (GdkWindow       *window,
                                         const GdkRectangle *rect,
                                         GdkGravity       rect_anchor,
                                         GdkGravity       window_anchor,
                                         GdkAnchorHints   anchor_hints,
                                         gint             rect_anchor_dx,
                                         gint             rect_anchor_dy);
  void         (* set_background)       (GdkWindow       *window,
                                         cairo_pattern_t *pattern);

  GdkEventMask (* get_events)           (GdkWindow       *window);
  void         (* set_events)           (GdkWindow       *window,
                                         GdkEventMask     event_mask);

  gboolean     (* reparent)             (GdkWindow       *window,
                                         GdkWindow       *new_parent,
                                         gint             x,
                                         gint             y);

  void         (* set_device_cursor)    (GdkWindow       *window,
                                         GdkDevice       *device,
                                         GdkCursor       *cursor);

  void         (* get_geometry)         (GdkWindow       *window,
                                         gint            *x,
                                         gint            *y,
                                         gint            *width,
                                         gint            *height);
  void         (* get_root_coords)      (GdkWindow       *window,
					 gint             x,
					 gint             y,
                                         gint            *root_x,
                                         gint            *root_y);
  gboolean     (* get_device_state)     (GdkWindow       *window,
                                         GdkDevice       *device,
                                         gdouble         *x,
                                         gdouble         *y,
                                         GdkModifierType *mask);
  gboolean    (* begin_paint)           (GdkWindow       *window);
  void        (* end_paint)             (GdkWindow       *window);

  cairo_region_t * (* get_shape)        (GdkWindow       *window);
  cairo_region_t * (* get_input_shape)  (GdkWindow       *window);
  void         (* shape_combine_region) (GdkWindow       *window,
                                         const cairo_region_t *shape_region,
                                         gint             offset_x,
                                         gint             offset_y);
  void         (* input_shape_combine_region) (GdkWindow       *window,
					       const cairo_region_t *shape_region,
					       gint             offset_x,
					       gint             offset_y);

  /* Called before processing updates for a window. This gives the windowing
   * layer a chance to save the region for later use in avoiding duplicate
   * exposes.
   */
  void     (* queue_antiexpose)     (GdkWindow       *window,
                                     cairo_region_t  *update_area);

/* Called to do the windowing system specific part of gdk_window_destroy(),
 *
 * window: The window being destroyed
 * recursing: If TRUE, then this is being called because a parent
 *     was destroyed. This generally means that the call to the windowing
 *     system to destroy the window can be omitted, since it will be
 *     destroyed as a result of the parent being destroyed.
 *     Unless @foreign_destroy
 * foreign_destroy: If TRUE, the window or a parent was destroyed by some
 *     external agency. The window has already been destroyed and no
 *     windowing system calls should be made. (This may never happen
 *     for some windowing systems.)
 */
  void         (* destroy)              (GdkWindow       *window,
					 gboolean         recursing,
					 gboolean         foreign_destroy);


 /* Called when gdk_window_destroy() is called on a foreign window
  * or an ancestor of the foreign window. It should generally reparent
  * the window out of it's current heirarchy, hide it, and then
  * send a message to the owner requesting that the window be destroyed.
  */
  void         (*destroy_foreign)       (GdkWindow       *window);

  /* optional */
  gboolean     (* beep)                 (GdkWindow       *window);

  void         (* focus)                (GdkWindow       *window,
					 guint32          timestamp);
  void         (* set_type_hint)        (GdkWindow       *window,
					 GdkWindowTypeHint hint);
  GdkWindowTypeHint (* get_type_hint)   (GdkWindow       *window);
  void         (* set_modal_hint)       (GdkWindow *window,
					 gboolean   modal);
  void         (* set_skip_taskbar_hint) (GdkWindow *window,
					  gboolean   skips_taskbar);
  void         (* set_skip_pager_hint)  (GdkWindow *window,
					 gboolean   skips_pager);
  void         (* set_urgency_hint)     (GdkWindow *window,
					 gboolean   urgent);
  void         (* set_geometry_hints)   (GdkWindow         *window,
					 const GdkGeometry *geometry,
					 GdkWindowHints     geom_mask);
  void         (* set_title)            (GdkWindow   *window,
					 const gchar *title);
  void         (* set_role)             (GdkWindow   *window,
					 const gchar *role);
  void         (* set_startup_id)       (GdkWindow   *window,
					 const gchar *startup_id);
  void         (* set_transient_for)    (GdkWindow *window,
					 GdkWindow *parent);
  void         (* get_frame_extents)    (GdkWindow    *window,
					 GdkRectangle *rect);
  void         (* set_override_redirect) (GdkWindow *window,
					  gboolean override_redirect);
  void         (* set_accept_focus)     (GdkWindow *window,
					 gboolean accept_focus);
  void         (* set_focus_on_map)     (GdkWindow *window,
					 gboolean focus_on_map);
  void         (* set_icon_list)        (GdkWindow *window,
					 GList     *pixbufs);
  void         (* set_icon_name)        (GdkWindow   *window,
					 const gchar *name);
  void         (* iconify)              (GdkWindow *window);
  void         (* deiconify)            (GdkWindow *window);
  void         (* stick)                (GdkWindow *window);
  void         (* unstick)              (GdkWindow *window);
  void         (* maximize)             (GdkWindow *window);
  void         (* unmaximize)           (GdkWindow *window);
  void         (* fullscreen)           (GdkWindow *window);
  void         (* fullscreen_on_monitor) (GdkWindow *window, gint monitor);
  void         (* apply_fullscreen_mode) (GdkWindow *window);
  void         (* unfullscreen)         (GdkWindow *window);
  void         (* set_keep_above)       (GdkWindow *window,
					 gboolean   setting);
  void         (* set_keep_below)       (GdkWindow *window,
					 gboolean   setting);
  GdkWindow *  (* get_group)            (GdkWindow *window);
  void         (* set_group)            (GdkWindow *window,
					 GdkWindow *leader);
  void         (* set_decorations)      (GdkWindow      *window,
					 GdkWMDecoration decorations);
  gboolean     (* get_decorations)      (GdkWindow       *window,
					 GdkWMDecoration *decorations);
  void         (* set_functions)        (GdkWindow    *window,
					 GdkWMFunction functions);
  void         (* begin_resize_drag)    (GdkWindow     *window,
                                         GdkWindowEdge  edge,
                                         GdkDevice     *device,
                                         gint           button,
                                         gint           root_x,
                                         gint           root_y,
                                         guint32        timestamp);
  void         (* begin_move_drag)      (GdkWindow *window,
                                         GdkDevice     *device,
                                         gint       button,
                                         gint       root_x,
                                         gint       root_y,
                                         guint32    timestamp);
  void         (* enable_synchronized_configure) (GdkWindow *window);
  void         (* configure_finished)   (GdkWindow *window);
  void         (* set_opacity)          (GdkWindow *window,
					 gdouble    opacity);
  void         (* set_composited)       (GdkWindow *window,
                                         gboolean   composited);
  void         (* destroy_notify)       (GdkWindow *window);
  GdkDragProtocol (* get_drag_protocol) (GdkWindow *window,
                                         GdkWindow **target);
  void         (* register_dnd)         (GdkWindow *window);
  GdkDragContext * (*drag_begin)        (GdkWindow *window,
                                         GdkDevice *device,
                                         GList     *targets,
                                         gint       x_root,
                                         gint       y_root);

  void         (*process_updates_recurse) (GdkWindow      *window,
                                           cairo_region_t *region);

  void         (*sync_rendering)          (GdkWindow      *window);
  gboolean     (*simulate_key)            (GdkWindow      *window,
                                           gint            x,
                                           gint            y,
                                           guint           keyval,
                                           GdkModifierType modifiers,
                                           GdkEventType    event_type);
  gboolean     (*simulate_button)         (GdkWindow      *window,
                                           gint            x,
                                           gint            y,
                                           guint           button,
                                           GdkModifierType modifiers,
                                           GdkEventType    event_type);

  gboolean     (*get_property)            (GdkWindow      *window,
                                           GdkAtom         property,
                                           GdkAtom         type,
                                           gulong          offset,
                                           gulong          length,
                                           gint            pdelete,
                                           GdkAtom        *actual_type,
                                           gint           *actual_format,
                                           gint           *actual_length,
                                           guchar        **data);
  void         (*change_property)         (GdkWindow      *window,
                                           GdkAtom         property,
                                           GdkAtom         type,
                                           gint            format,
                                           GdkPropMode     mode,
                                           const guchar   *data,
                                           gint            n_elements);
  void         (*delete_property)         (GdkWindow      *window,
                                           GdkAtom         property);

  gint         (* get_scale_factor)       (GdkWindow      *window);
  void         (* get_unscaled_size)      (GdkWindow      *window,
                                           int            *unscaled_width,
                                           int            *unscaled_height);

  void         (* set_opaque_region)      (GdkWindow      *window,
                                           cairo_region_t *region);
  void         (* set_shadow_width)       (GdkWindow      *window,
                                           gint            left,
                                           gint            right,
                                           gint            top,
                                           gint            bottom);
  gboolean     (* show_window_menu)       (GdkWindow      *window,
                                           GdkEvent       *event);
  GdkGLContext *(*create_gl_context)      (GdkWindow      *window,
					   gboolean        attached,
                                           GdkGLContext   *share,
                                           GError        **error);
  gboolean     (* realize_gl_context)     (GdkWindow      *window,
                                           GdkGLContext   *context,
                                           GError        **error);
  void         (*invalidate_for_new_frame)(GdkWindow      *window,
                                           cairo_region_t *update_area);

  GdkDrawingContext *(* create_draw_context)  (GdkWindow            *window,
                                               const cairo_region_t *region);
  void               (* destroy_draw_context) (GdkWindow            *window,
                                               GdkDrawingContext    *context);
} _GdkWindowImplClass;

void override_x11_window_invalidate_for_new_frame(_GdkWindow* window) {
    GType* t = g_type_from_name("GdkWindowImplX11");
    _GdkWindowImplClass* impl_class = g_type_class_ref(t);
    impl_class->invalidate_for_new_frame = _gdk_x11_window_invalidate_for_new_frame;
    g_type_class_unref(impl_class);
}
#endif

#define GL_RENDERBUFFER                                  0x8D41
#define GL_TEXTURE_2D_MULTISAMPLE                    0x9100
#define GL_DEPTH24_STENCIL8                              0x88F0
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_FRAMEBUFFER_EXT                                  0x8D40
#define GL_COLOR_ATTACHMENT0_EXT                            0x8CE0
#define GL_FRAMEBUFFER_COMPLETE_EXT                         0x8CD5
#define GL_RENDERBUFFER_EXT                                 0x8D41
#define GL_DEPTH_ATTACHMENT_EXT                             0x8D00
#define GL_STENCIL_ATTACHMENT_EXT                           0x8D20
#define GL_BGRA                           0x80E1

typedef struct {
  GdkGLContext *context;
  GdkWindow *event_window;
  GError *error;

  gboolean have_buffers;

  int required_gl_version;

  guint frame_buffer;
  guint render_buffer;
  guint texture;
  guint depth_stencil_buffer;

  guint samples;
  gboolean has_alpha;
  gboolean has_depth_buffer;
  gboolean has_stencil_buffer;

  gboolean needs_resize;
  gboolean needs_render;
  gboolean auto_render;
  gboolean use_es;
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

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

enum {
  RENDER,
  RESIZE,
  CREATE_CONTEXT,

  LAST_SIGNAL
};

static void gl_area_allocate_buffers (GLArea *area);

static guint area_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (GLArea, gl_area, GTK_TYPE_WIDGET)

static void
gl_area_dispose (GObject *gobject)
{
  GLArea *area = GL_AREA (gobject);
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_clear_object (&priv->context);

  G_OBJECT_CLASS (gl_area_parent_class)->dispose (gobject);
}

static void
gl_area_set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GLArea *self = GL_AREA (gobject);

  switch (prop_id)
    {
    case PROP_AUTO_RENDER:
      gl_area_set_auto_render (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_ALPHA:
      gl_area_set_has_alpha (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_DEPTH_BUFFER:
      gl_area_set_has_depth_buffer (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_STENCIL_BUFFER:
      gl_area_set_has_stencil_buffer (self, g_value_get_boolean (value));
      break;

    case PROP_USE_ES:
      gl_area_set_use_es (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
gl_area_get_property (GObject    *gobject,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (GL_AREA (gobject));

  switch (prop_id)
    {
    case PROP_AUTO_RENDER:
      g_value_set_boolean (value, priv->auto_render);
      break;

    case PROP_HAS_ALPHA:
      g_value_set_boolean (value, priv->has_alpha);
      break;

    case PROP_HAS_DEPTH_BUFFER:
      g_value_set_boolean (value, priv->has_depth_buffer);
      break;

    case PROP_HAS_STENCIL_BUFFER:
      g_value_set_boolean (value, priv->has_stencil_buffer);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;

    case PROP_USE_ES:
      g_value_set_boolean (value, priv->use_es);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
gl_area_realize (GtkWidget *widget)
{
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

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  priv->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                       &attributes, attributes_mask);
  gtk_widget_register_window (widget, priv->event_window);

  g_clear_error (&priv->error);
  priv->context = NULL;
  g_signal_emit (area, area_signals[CREATE_CONTEXT], 0, &priv->context);


  /* In case the signal failed, but did not set an error */
  /*if (priv->context == NULL && priv->error == NULL)
    g_set_error_literal (&priv->error, GDK_GL_ERROR,
                         GDK_GL_ERROR_NOT_AVAILABLE,
                         _("OpenGL context creation failed"));*/

  priv->needs_resize = TRUE;
}

static void
gl_area_notify (GObject    *object,
                    GParamSpec *pspec)
{
  if (strcmp (pspec->name, "scale-factor") == 0)
    {
      GLArea *area = GL_AREA (object);
      GLAreaPrivate *priv = gl_area_get_instance_private (area);

      priv->needs_resize = TRUE;
    }

  if (G_OBJECT_CLASS (gl_area_parent_class)->notify)
    G_OBJECT_CLASS (gl_area_parent_class)->notify (object, pspec);
}

static GdkGLContext *
gl_area_real_create_context (GLArea *area)
{
    printf("gl_area_real_create_context\n");
  GLAreaPrivate *priv = gl_area_get_instance_private (area);
  GtkWidget *widget = GTK_WIDGET (area);
  GError *error = NULL;
  GdkGLContext *context;

#ifndef _WIN32
  override_x11_window_invalidate_for_new_frame( gtk_widget_get_window (widget) );
#endif

  context = gdk_window_create_gl_context (gtk_widget_get_window (widget), &error);
  if (error != NULL)
    {
      gl_area_set_error (area, error);
      g_clear_object (&context);
      g_clear_error (&error);
      return NULL;
    }

  gdk_gl_context_set_use_es (context, priv->use_es);
  gdk_gl_context_set_required_version (context,
                                       priv->required_gl_version / 10,
                                       priv->required_gl_version % 10);

  gdk_gl_context_realize (context, &error);
  if (error != NULL)
    {
      gl_area_set_error (area, error);
      g_clear_object (&context);
      g_clear_error (&error);
      return NULL;
    }

  gdk_gl_context_make_current(context);
  initGLFunctions();

  return context;
}

static void
gl_area_resize (GLArea *area, int width, int height)
{
  glViewport (0, 0, width, height);
}

/*
 * Creates all the buffer objects needed for rendering the scene
 */
static void
gl_area_ensure_buffers (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);
  GtkWidget *widget = GTK_WIDGET (area);

  gtk_widget_realize (widget);

  if (priv->context == NULL)
    return;

  if (priv->have_buffers)
    return;

  priv->have_buffers = TRUE;

  glGenFramebuffers (1, &priv->frame_buffer);

  if (priv->has_alpha)
    {
      /* For alpha we use textures as that is required for blending to work */
      if (priv->texture == 0)
        glGenTextures (1, &priv->texture);

      /* Delete old render buffer if any */
      if (priv->render_buffer != 0)
        {
          glDeleteRenderbuffers(1, &priv->render_buffer);
          priv->render_buffer = 0;
        }
    }
  else
    {
    /* For non-alpha we use render buffers so we can blit instead of texture the result */
      if (priv->render_buffer == 0)
        glGenRenderbuffers (1, &priv->render_buffer);

      /* Delete old texture if any */
      if (priv->texture != 0)
        {
          glDeleteTextures(1, &priv->texture);
          priv->texture = 0;
        }
    }

  if ((priv->has_depth_buffer || priv->has_stencil_buffer))
    {
      if (priv->depth_stencil_buffer == 0)
        glGenRenderbuffers (1, &priv->depth_stencil_buffer);
    }
  else if (priv->depth_stencil_buffer != 0)
    {
      /* Delete old depth/stencil buffer */
      glDeleteRenderbuffers (1, &priv->depth_stencil_buffer);
      priv->depth_stencil_buffer = 0;
    }

  gl_area_allocate_buffers (area);
}

/*
 * Allocates space of the right type and size for all the buffers
 */
static void
gl_area_allocate_buffers (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);
  GtkWidget *widget = GTK_WIDGET (area);
  int scale, width, height;

  if (priv->context == NULL)
    return;

  scale = gtk_widget_get_scale_factor (widget);
  width = gtk_widget_get_allocated_width (widget) * scale;
  height = gtk_widget_get_allocated_height (widget) * scale;

  if (priv->texture)
    {
      if (priv->samples > 0) {
          glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, priv->texture);
      } else {
          glBindTexture(GL_TEXTURE_2D, priv->texture);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }

      if (gdk_gl_context_get_use_es(priv->context)) {
          if (priv->samples == 0) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
          else glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, priv->samples, GL_RGBA8, width, height, GL_FALSE);
      } else {
          if (priv->samples == 0) glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
          else glTexImage2DMultisample (GL_TEXTURE_2D_MULTISAMPLE, priv->samples, GL_RGBA8, width, height, GL_FALSE);
      }
    }

  if (priv->render_buffer)
    {
      glBindRenderbuffer (GL_RENDERBUFFER, priv->render_buffer);
      if (priv->samples == 0) glRenderbufferStorage (GL_RENDERBUFFER, GL_RGB8, width, height);
      else glRenderbufferStorageMultisample (GL_RENDERBUFFER, priv->samples, GL_RGB8, width, height);
    }

  if (priv->has_depth_buffer || priv->has_stencil_buffer)
    {
      glBindRenderbuffer (GL_RENDERBUFFER, priv->depth_stencil_buffer);
      if (priv->has_stencil_buffer) {
        if (priv->samples == 0) glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        else glRenderbufferStorageMultisample(GL_RENDERBUFFER, priv->samples, GL_DEPTH24_STENCIL8, width, height);
      } else {
        if (priv->samples == 0) glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        else glRenderbufferStorageMultisample(GL_RENDERBUFFER, priv->samples, GL_DEPTH_COMPONENT24, width, height);
      }
    }

  priv->needs_render = TRUE;
}

/**
 * gl_area_attach_buffers:
 * @area: a #GLArea
 *
 * Ensures that the @area framebuffer object is made the current draw
 * and read target, and that all the required buffers for the @area
 * are created and bound to the frambuffer.
 *
 * This function is automatically called before emitting the
 * #GLArea::render signal, and doesn't normally need to be called
 * by application code.
 *
 * Since: 3.16
 */
void
gl_area_attach_buffers (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  if (priv->context == NULL)
    return;

  gl_area_make_current (area);

  if (!priv->have_buffers)
    gl_area_ensure_buffers (area);
  else if (priv->needs_resize)
    gl_area_allocate_buffers (area);

  glBindFramebuffer (GL_FRAMEBUFFER_EXT, priv->frame_buffer);

  if (priv->texture) {
      if (priv->samples == 0) glFramebufferTexture2D (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, priv->texture, 0);
      else glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D_MULTISAMPLE, priv->texture, 0);
  } else if (priv->render_buffer) {
    glFramebufferRenderbuffer (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, priv->render_buffer);
  }

  if (priv->depth_stencil_buffer)
    {
      if (priv->has_depth_buffer)
        glFramebufferRenderbuffer (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                      GL_RENDERBUFFER_EXT, priv->depth_stencil_buffer);
      if (priv->has_stencil_buffer)
        glFramebufferRenderbuffer (GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                                      GL_RENDERBUFFER_EXT, priv->depth_stencil_buffer);
    }
}

static void
gl_area_delete_buffers (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  if (priv->context == NULL)
    return;

  priv->have_buffers = FALSE;

  if (priv->render_buffer != 0)
    {
      glDeleteRenderbuffers (1, &priv->render_buffer);
      priv->render_buffer = 0;
    }

  if (priv->texture != 0)
    {
      glDeleteTextures(1, &priv->texture);
      priv->texture = 0;
    }

  if (priv->depth_stencil_buffer != 0)
    {
      glDeleteRenderbuffers (1, &priv->depth_stencil_buffer);
      priv->depth_stencil_buffer = 0;
    }

  if (priv->frame_buffer != 0)
    {
      glBindFramebuffer (GL_FRAMEBUFFER_EXT, 0);
      glDeleteFramebuffers (1, &priv->frame_buffer);
      priv->frame_buffer = 0;
    }
}

static void
gl_area_unrealize (GtkWidget *widget)
{
  GLArea *area = GL_AREA (widget);
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  if (priv->context != NULL)
    {
      if (priv->have_buffers)
        {
          gl_area_make_current (area);
          gl_area_delete_buffers (area);
        }

      /* Make sure to unset the context if current */
      if (priv->context == gdk_gl_context_get_current ())
        gdk_gl_context_clear_current ();
    }

  g_clear_object (&priv->context);
  g_clear_error (&priv->error);

  if (priv->event_window != NULL)
    {
      gtk_widget_unregister_window (widget, priv->event_window);
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  GTK_WIDGET_CLASS (gl_area_parent_class)->unrealize (widget);
}

static void
gl_area_map (GtkWidget *widget)
{
  GLArea *area = GL_AREA (widget);
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  if (priv->event_window != NULL)
    gdk_window_show (priv->event_window);

  GTK_WIDGET_CLASS (gl_area_parent_class)->map (widget);
}

static void
gl_area_unmap (GtkWidget *widget)
{
  GLArea *area = GL_AREA (widget);
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  if (priv->event_window != NULL)
    gdk_window_hide (priv->event_window);

  GTK_WIDGET_CLASS (gl_area_parent_class)->unmap (widget);
}

static void
gl_area_size_allocate (GtkWidget     *widget,
                           GtkAllocation *allocation)
{
  GLArea *area = GL_AREA (widget);
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  GTK_WIDGET_CLASS (gl_area_parent_class)->size_allocate (widget, allocation);

  if (gtk_widget_get_realized (widget))
    {
      if (priv->event_window != NULL)
        gdk_window_move_resize (priv->event_window,
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);

      priv->needs_resize = TRUE;
    }
}

static void
gl_area_draw_error_screen (GLArea *area,
                               cairo_t   *cr,
                               gint       width,
                               gint       height)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);
  PangoLayout *layout;
  int layout_height;

  layout = gtk_widget_create_pango_layout (GTK_WIDGET (area),
                                           priv->error->message);
  pango_layout_set_width (layout, width * PANGO_SCALE);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
  pango_layout_get_pixel_size (layout, NULL, &layout_height);
  gtk_render_layout (gtk_widget_get_style_context (GTK_WIDGET (area)),
                     cr,
                     0, (height - layout_height) / 2,
                     layout);

  g_object_unref (layout);
}

static gboolean
gl_area_draw (GtkWidget *widget,
                  cairo_t   *cr)
{
  GLArea *area = GL_AREA (widget);
  GLAreaPrivate *priv = gl_area_get_instance_private (area);
  gboolean unused;
  int w, h, scale;
  GLenum status;

  if (priv->error != NULL)
    {
      gl_area_draw_error_screen (area,
                                     cr,
                                     gtk_widget_get_allocated_width (widget),
                                     gtk_widget_get_allocated_height (widget));
      return FALSE;
    }

  if (priv->context == NULL)
    return FALSE;

  gl_area_make_current (area);

  gl_area_attach_buffers (area);

 if (priv->has_depth_buffer)
   glEnable (GL_DEPTH_TEST);
 else
   glDisable (GL_DEPTH_TEST);

  scale = gtk_widget_get_scale_factor (widget);
  w = gtk_widget_get_allocated_width (widget) * scale;
  h = gtk_widget_get_allocated_height (widget) * scale;

  status = glCheckFramebufferStatus (GL_FRAMEBUFFER_EXT);
  if (status == GL_FRAMEBUFFER_COMPLETE_EXT)
    {
      if (priv->needs_render || priv->auto_render)
        {
          if (priv->needs_resize)
            {
              g_signal_emit (area, area_signals[RESIZE], 0, w, h, NULL);
              priv->needs_resize = FALSE;
            }

          g_signal_emit (area, area_signals[RENDER], 0, priv->context, &unused);
        }

      priv->needs_render = FALSE;

      gdk_cairo_draw_from_gl (cr,
                              gtk_widget_get_window (widget),
                              priv->texture ? priv->texture : priv->render_buffer,
                              priv->texture ? GL_TEXTURE : GL_RENDERBUFFER,
                              scale, 0, 0, w, h);
      gl_area_make_current (area);
    }
  else
    {
      g_warning ("fb setup not supported");
    }

  return TRUE;
}

static gboolean
create_context_accumulator (GSignalInvocationHint *ihint,
                            GValue *return_accu,
                            const GValue *handler_return,
                            gpointer data)
{
  g_value_copy (handler_return, return_accu);

  /* stop after the first handler returning a valid object */
  return g_value_get_object (handler_return) == NULL;
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
        gint arg1,
        gint arg2,
        gpointer data2);
    GCClosure* cc = (GCClosure*)closure;
    gpointer data1, data2;
    GMarshalFunc_VOID__INT_INT callback;

    g_return_if_fail(n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA(closure))
    {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else
    {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__INT_INT)(marshal_data ? marshal_data : cc->callback);

    callback(data1,
        g_marshal_value_peek_int(param_values + 1),
        g_marshal_value_peek_int(param_values + 2),
        data2);
}


void
_marshal_OBJECT__VOID(GClosure* closure,
    GValue* return_value,
    guint         n_param_values,
    const GValue* param_values,
    gpointer      invocation_hint G_GNUC_UNUSED,
    gpointer      marshal_data)
{
    typedef GObject* (*GMarshalFunc_OBJECT__VOID) (gpointer data1,
        gpointer data2);
    GCClosure* cc = (GCClosure*)closure;
    gpointer data1, data2;
    GMarshalFunc_OBJECT__VOID callback;
    GObject* v_return;

    g_return_if_fail(return_value != NULL);
    g_return_if_fail(n_param_values == 1);

    if (G_CCLOSURE_SWAP_DATA(closure))
    {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    }
    else
    {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_OBJECT__VOID)(marshal_data ? marshal_data : cc->callback);

    v_return = callback(data1,
        data2);

    g_value_take_object(return_value, v_return);
}

#define P_(String) (String)
#define I_(string) g_intern_static_string(string)

static void
gl_area_class_init (GLAreaClass *klass)
{
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

  /**
   * GLArea:context:
   *
   * The #GdkGLContext used by the #GLArea widget.
   *
   * The #GLArea widget is responsible for creating the #GdkGLContext
   * instance. If you need to render with other kinds of buffers (stencil,
   * depth, etc), use render buffers.
   *
   * Since: 3.16
   */
  obj_props[PROP_CONTEXT] =
    g_param_spec_object ("context",
                         P_("Context"),
                         P_("The GL context"),
                         GDK_TYPE_GL_CONTEXT,
                         G_PARAM_READABLE |
                         G_PARAM_STATIC_STRINGS);

  /**
   * GLArea:auto-render:
   *
   * If set to %TRUE the #GLArea::render signal will be emitted every time
   * the widget draws. This is the default and is useful if drawing the widget
   * is faster.
   *
   * If set to %FALSE the data from previous rendering is kept around and will
   * be used for drawing the widget the next time, unless the window is resized.
   * In order to force a rendering gl_area_queue_render() must be called.
   * This mode is useful when the scene changes seldomly, but takes a long time
   * to redraw.
   *
   * Since: 3.16
   */

#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

  obj_props[PROP_AUTO_RENDER] =
    g_param_spec_boolean ("auto-render",
                          P_("Auto render"),
                          P_("Whether the GLArea renders on each redraw"),
                          TRUE,
                          GTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GLArea:has-alpha:
   *
   * If set to %TRUE the buffer allocated by the widget will have an alpha channel
   * component, and when rendering to the window the result will be composited over
   * whatever is below the widget.
   *
   * If set to %FALSE there will be no alpha channel, and the buffer will fully
   * replace anything below the widget.
   *
   * Since: 3.16
   */
  obj_props[PROP_HAS_ALPHA] =
    g_param_spec_boolean ("has-alpha",
                          P_("Has alpha"),
                          P_("Whether the color buffer has an alpha component"),
                          FALSE,
                          GTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GLArea:has-depth-buffer:
   *
   * If set to %TRUE the widget will allocate and enable a depth buffer for the
   * target framebuffer.
   *
   * Since: 3.16
   */
  obj_props[PROP_HAS_DEPTH_BUFFER] =
    g_param_spec_boolean ("has-depth-buffer",
                          P_("Has depth buffer"),
                          P_("Whether a depth buffer is allocated"),
                          FALSE,
                          GTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GLArea:has-stencil-buffer:
   *
   * If set to %TRUE the widget will allocate and enable a stencil buffer for the
   * target framebuffer.
   *
   * Since: 3.16
   */
  obj_props[PROP_HAS_STENCIL_BUFFER] =
    g_param_spec_boolean ("has-stencil-buffer",
                          P_("Has stencil buffer"),
                          P_("Whether a stencil buffer is allocated"),
                          FALSE,
                          GTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GLArea:use-es:
   *
   * If set to %TRUE the widget will try to create a #GdkGLContext using
   * OpenGL ES instead of OpenGL.
   *
   * See also: gdk_gl_context_set_use_es()
   *
   * Since: 3.22
   */
  obj_props[PROP_USE_ES] =
    g_param_spec_boolean ("use-es",
                          P_("Use OpenGL ES"),
                          P_("Whether the context uses OpenGL or OpenGL ES"),
                          FALSE,
                          GTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  gobject_class->set_property = gl_area_set_property;
  gobject_class->get_property = gl_area_get_property;
  gobject_class->dispose = gl_area_dispose;
  gobject_class->notify = gl_area_notify;

  g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);

  /**
   * GLArea::render:
   * @area: the #GLArea that emitted the signal
   * @context: the #GdkGLContext used by @area
   *
   * The ::render signal is emitted every time the contents
   * of the #GLArea should be redrawn.
   *
   * The @context is bound to the @area prior to emitting this function,
   * and the buffers are painted to the window once the emission terminates.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   *
   * Since: 3.16
   */

  area_signals[RENDER] =
    g_signal_new (I_("render"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GLAreaClass, render),
                  _boolean_handled_accumulator, NULL,
                  NULL,
                  G_TYPE_BOOLEAN, 1,
                  GDK_TYPE_GL_CONTEXT);

  /**
   * GLArea::resize:
   * @area: the #GLArea that emitted the signal
   * @width: the width of the viewport
   * @height: the height of the viewport
   *
   * The ::resize signal is emitted once when the widget is realized, and
   * then each time the widget is changed while realized. This is useful
   * in order to keep GL state up to date with the widget size, like for
   * instance camera properties which may depend on the width/height ratio.
   *
   * The GL context for the area is guaranteed to be current when this signal
   * is emitted.
   *
   * The default handler sets up the GL viewport.
   *
   * Since: 3.16
   */
  area_signals[RESIZE] =
    g_signal_new (I_("resize"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GLAreaClass, resize),
                  NULL, NULL,
                  _marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

  /**
   * GLArea::create-context:
   * @area: the #GLArea that emitted the signal
   * @error: (allow-none): location to store error information on failure
   *
   * The ::create-context signal is emitted when the widget is being
   * realized, and allows you to override how the GL context is
   * created. This is useful when you want to reuse an existing GL
   * context, or if you want to try creating different kinds of GL
   * options.
   *
   * If context creation fails then the signal handler can use
   * gl_area_set_error() to register a more detailed error
   * of how the construction failed.
   *
   * Returns: (transfer full): a newly created #GdkGLContext;
   *     the #GLArea widget will take ownership of the returned value.
   *
   * Since: 3.16
   */
  area_signals[CREATE_CONTEXT] =
    g_signal_new (I_("create-context"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GLAreaClass, create_context),
                  create_context_accumulator, NULL,
                  _marshal_OBJECT__VOID,
                  GDK_TYPE_GL_CONTEXT, 0);
}

static void
gl_area_init (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  gtk_widget_set_has_window (GTK_WIDGET (area), FALSE);
  gtk_widget_set_app_paintable (GTK_WIDGET (area), TRUE);

  priv->auto_render = TRUE;
  priv->needs_render = TRUE;
  priv->required_gl_version = 0;
}

/**
 * gl_area_new:
 *
 * Creates a new #GLArea widget.
 *
 * Returns: (transfer full): the newly created #GLArea
 *
 * Since: 3.16
 */
GtkWidget*
gl_area_new (void) {
    GtkWidget* obj = g_object_new (TYPE_GL_AREA, NULL);
    return obj;
}

/**
 * gl_area_set_error:
 * @area: a #GLArea
 * @error: (allow-none): a new #GError, or %NULL to unset the error
 *
 * Sets an error on the area which will be shown instead of the
 * GL rendering. This is useful in the #GLArea::create-context
 * signal if GL context creation fails.
 *
 * Since: 3.16
 */
void
gl_area_set_error (GLArea    *area,
                       const GError *error)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  g_clear_error (&priv->error);
  if (error)
    priv->error = g_error_copy (error);
}

/**
 * gl_area_get_error:
 * @area: a #GLArea
 *
 * Gets the current error set on the @area.
 *
 * Returns: (nullable) (transfer none): the #GError or %NULL
 *
 * Since: 3.16
 */
GError *
gl_area_get_error (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_val_if_fail (IS_GL_AREA (area), NULL);

  return priv->error;
}

/**
 * gl_area_set_use_es:
 * @area: a #GLArea
 * @use_es: whether to use OpenGL or OpenGL ES
 *
 * Sets whether the @area should create an OpenGL or an OpenGL ES context.
 *
 * You should check the capabilities of the #GdkGLContext before drawing
 * with either API.
 *
 * Since: 3.22
 */
void
gl_area_set_use_es (GLArea *area,
                        gboolean   use_es)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));
  g_return_if_fail (!gtk_widget_get_realized (GTK_WIDGET (area)));

  use_es = !!use_es;

  if (priv->use_es != use_es)
    {
      priv->use_es = use_es;

      g_object_notify_by_pspec (G_OBJECT (area), obj_props[PROP_USE_ES]);
    }
}

/**
 * gl_area_get_use_es:
 * @area: a #GLArea
 *
 * Retrieves the value set by gl_area_set_use_es().
 *
 * Returns: %TRUE if the #GLArea should create an OpenGL ES context
 *   and %FALSE otherwise
 *
 * Since: 3.22
 */
gboolean
gl_area_get_use_es (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_val_if_fail (IS_GL_AREA (area), FALSE);

  return priv->use_es;
}

/**
 * gl_area_set_required_version:
 * @area: a #GLArea
 * @major: the major version
 * @minor: the minor version
 *
 * Sets the required version of OpenGL to be used when creating the context
 * for the widget.
 *
 * This function must be called before the area has been realized.
 *
 * Since: 3.16
 */
void
gl_area_set_required_version (GLArea *area,
                                  gint       major,
                                  gint       minor)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));
  g_return_if_fail (!gtk_widget_get_realized (GTK_WIDGET (area)));

  priv->required_gl_version = major * 10 + minor;
}

/**
 * gl_area_get_required_version:
 * @area: a #GLArea
 * @major: (out): return location for the required major version
 * @minor: (out): return location for the required minor version
 *
 * Retrieves the required version of OpenGL set
 * using gl_area_set_required_version().
 *
 * Since: 3.16
 */
void
gl_area_get_required_version (GLArea *area,
                                  gint      *major,
                                  gint      *minor)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  if (major != NULL)
    *major = priv->required_gl_version / 10;
  if (minor != NULL)
    *minor = priv->required_gl_version % 10;
}

/**
 * gl_area_get_has_alpha:
 * @area: a #GLArea
 *
 * Returns whether the area has an alpha component.
 *
 * Returns: %TRUE if the @area has an alpha component, %FALSE otherwise
 *
 * Since: 3.16
 */
gboolean
gl_area_get_has_alpha (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_val_if_fail (IS_GL_AREA (area), FALSE);

  return priv->has_alpha;
}

/**
 * gl_area_set_has_alpha:
 * @area: a #GLArea
 * @has_alpha: %TRUE to add an alpha component
 *
 * If @has_alpha is %TRUE the buffer allocated by the widget will have
 * an alpha channel component, and when rendering to the window the
 * result will be composited over whatever is below the widget.
 *
 * If @has_alpha is %FALSE there will be no alpha channel, and the
 * buffer will fully replace anything below the widget.
 *
 * Since: 3.16
 */
void
gl_area_set_has_alpha (GLArea *area,
                           gboolean   has_alpha)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  has_alpha = !!has_alpha;

  if (priv->has_alpha != has_alpha)
    {
      priv->has_alpha = has_alpha;

      g_object_notify (G_OBJECT (area), "has-alpha");

      gl_area_delete_buffers (area);
    }
}

/**
 * gl_area_get_has_depth_buffer:
 * @area: a #GLArea
 *
 * Returns whether the area has a depth buffer.
 *
 * Returns: %TRUE if the @area has a depth buffer, %FALSE otherwise
 *
 * Since: 3.16
 */
gboolean
gl_area_get_has_depth_buffer (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_val_if_fail (IS_GL_AREA (area), FALSE);

  return priv->has_depth_buffer;
}

/**
 * gl_area_set_has_depth_buffer:
 * @area: a #GLArea
 * @has_depth_buffer: %TRUE to add a depth buffer
 *
 * If @has_depth_buffer is %TRUE the widget will allocate and
 * enable a depth buffer for the target framebuffer. Otherwise
 * there will be none.
 *
 * Since: 3.16
 */
void
gl_area_set_has_depth_buffer (GLArea *area,
                                  gboolean   has_depth_buffer)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  has_depth_buffer = !!has_depth_buffer;

  if (priv->has_depth_buffer != has_depth_buffer)
    {
      priv->has_depth_buffer = has_depth_buffer;

      g_object_notify (G_OBJECT (area), "has-depth-buffer");

      priv->have_buffers = FALSE;
    }
}

/**
 * gl_area_get_has_stencil_buffer:
 * @area: a #GLArea
 *
 * Returns whether the area has a stencil buffer.
 *
 * Returns: %TRUE if the @area has a stencil buffer, %FALSE otherwise
 *
 * Since: 3.16
 */
gboolean
gl_area_get_has_stencil_buffer (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_val_if_fail (IS_GL_AREA (area), FALSE);

  return priv->has_stencil_buffer;
}

guint
gl_area_get_samples (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_val_if_fail (IS_GL_AREA (area), FALSE);

  return priv->samples;
}

/**
 * gl_area_set_has_stencil_buffer:
 * @area: a #GLArea
 * @has_stencil_buffer: %TRUE to add a stencil buffer
 *
 * If @has_stencil_buffer is %TRUE the widget will allocate and
 * enable a stencil buffer for the target framebuffer. Otherwise
 * there will be none.
 *
 * Since: 3.16
 */
void
gl_area_set_has_stencil_buffer (GLArea *area,
                                    gboolean   has_stencil_buffer)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  has_stencil_buffer = !!has_stencil_buffer;

  if (priv->has_stencil_buffer != has_stencil_buffer)
    {
      priv->has_stencil_buffer = has_stencil_buffer;

      g_object_notify (G_OBJECT (area), "has-stencil-buffer");

      priv->have_buffers = FALSE;
    }
}

void
gl_area_set_samples (GLArea *area,
                                    guint   samples)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  if (priv->samples != samples)
    {
      priv->samples = samples;

      priv->have_buffers = FALSE;
    }
}

/**
 * gl_area_queue_render:
 * @area: a #GLArea
 *
 * Marks the currently rendered data (if any) as invalid, and queues
 * a redraw of the widget, ensuring that the #GLArea::render signal
 * is emitted during the draw.
 *
 * This is only needed when the gl_area_set_auto_render() has
 * been called with a %FALSE value. The default behaviour is to
 * emit #GLArea::render on each draw.
 *
 * Since: 3.16
 */
void
gl_area_queue_render (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  priv->needs_render = TRUE;

  gtk_widget_queue_draw (GTK_WIDGET (area));
}


/**
 * gl_area_get_auto_render:
 * @area: a #GLArea
 *
 * Returns whether the area is in auto render mode or not.
 *
 * Returns: %TRUE if the @area is auto rendering, %FALSE otherwise
 *
 * Since: 3.16
 */
gboolean
gl_area_get_auto_render (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_val_if_fail (IS_GL_AREA (area), FALSE);

  return priv->auto_render;
}

/**
 * gl_area_set_auto_render:
 * @area: a #GLArea
 * @auto_render: a boolean
 *
 * If @auto_render is %TRUE the #GLArea::render signal will be
 * emitted every time the widget draws. This is the default and is
 * useful if drawing the widget is faster.
 *
 * If @auto_render is %FALSE the data from previous rendering is kept
 * around and will be used for drawing the widget the next time,
 * unless the window is resized. In order to force a rendering
 * gl_area_queue_render() must be called. This mode is useful when
 * the scene changes seldomly, but takes a long time to redraw.
 *
 * Since: 3.16
 */
void
gl_area_set_auto_render (GLArea *area,
                             gboolean   auto_render)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_if_fail (IS_GL_AREA (area));

  auto_render = !!auto_render;

  if (priv->auto_render != auto_render)
    {
      priv->auto_render = auto_render;

      g_object_notify (G_OBJECT (area), "auto-render");

      if (auto_render)
        gtk_widget_queue_draw (GTK_WIDGET (area));
    }
}

/**
 * gl_area_get_context:
 * @area: a #GLArea
 *
 * Retrieves the #GdkGLContext used by @area.
 *
 * Returns: (transfer none): the #GdkGLContext
 *
 * Since: 3.16
 */
GdkGLContext *
gl_area_get_context (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);

  g_return_val_if_fail (IS_GL_AREA (area), NULL);

  return priv->context;
}

/**
 * gl_area_make_current:
 * @area: a #GLArea
 *
 * Ensures that the #GdkGLContext used by @area is associated with
 * the #GLArea.
 *
 * This function is automatically called before emitting the
 * #GLArea::render signal, and doesn't normally need to be called
 * by application code.
 *
 * Since: 3.16
 */
void
gl_area_make_current (GLArea *area)
{
  GLAreaPrivate *priv = gl_area_get_instance_private (area);
  GtkWidget *widget;

  g_return_if_fail (IS_GL_AREA (area));

  widget = GTK_WIDGET (area);

  g_return_if_fail (gtk_widget_get_realized (widget));

  if (priv->context != NULL)
    gdk_gl_context_make_current (priv->context);
}
