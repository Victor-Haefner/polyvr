#define GTK_COMPILATION
#define GDK_COMPILATION

#include <gdk/x11/gdkx11glcontext.h>

#include "glarea.h"
#include <gtk/gtkstylecontext.h>
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

typedef struct {
  GObject parent_instance;

  cairo_font_options_t *font_options;
  gdouble resolution; /* pixels/points scale factor for fonts */
  guint resolution_set : 1; /* resolution set through public API */
  guint closed : 1;
} _GdkScreen;

typedef struct {
  _GdkScreen parent_instance;

  GdkDisplay *display;
  Display *xdisplay;
  Screen *xscreen;
  Window xroot_window;
  GdkWindow *root_window;
  gint screen_num;

  gint width;
  gint height;

  gint window_scale;
  gboolean fixed_window_scale;

  /* Xft resources for the display, used for default values for
   * the Xft/ XSETTINGS
   */
  gint xft_hintstyle;
  gint xft_rgba;
  gint xft_dpi;

  /* Window manager */
  long last_wmspec_check_time;
  Window wmspec_check_window;
  char *window_manager_name;

  /* X Settings */
  GdkWindow *xsettings_manager_window;
  Atom xsettings_selection_atom;
  GHashTable *xsettings; /* string of GDK settings name => GValue */

  /* TRUE if wmspec_check_window has changed since last
   * fetch of _NET_SUPPORTED
   */
  guint need_refetch_net_supported : 1;
  /* TRUE if wmspec_check_window has changed since last
   * fetch of window manager name
   */
  guint need_refetch_wm_name : 1;
  guint is_composited : 1;
  guint xft_init : 1; /* Whether we've intialized these values yet */
  guint xft_antialias : 1;
  guint xft_hinting : 1;

  /* Visual Part */
  gint nvisuals;
  GdkVisual **visuals;
  GdkVisual *system_visual;
  gint available_depths[7];
  GdkVisualType available_types[6];
  gint16 navailable_depths;
  gint16 navailable_types;
  GHashTable *visual_hash;
  GdkVisual *rgba_visual;

  /* cache for window->translate vfunc */
  GC subwindow_gcs[32];
} _GdkX11Screen;

typedef struct {
  guint program;
  guint position_location;
  guint uv_location;
  guint map_location;
  guint flip_location;
} GdkGLContextProgram;

typedef struct {
  guint vertex_array_object;
  guint tmp_framebuffer;
  guint tmp_vertex_buffer;

  GdkGLContextProgram texture_2d_quad_program;
  GdkGLContextProgram texture_rect_quad_program;

  GdkGLContextProgram *current_program;

  guint is_legacy : 1;
  guint use_es : 1;
} GdkGLContextPaintData;

typedef struct {
  GdkDisplay *display;
  GdkWindow *window;
  GdkGLContext *shared_context;

  int major;
  int minor;
  int gl_version;

  guint realized : 1;
  guint use_texture_rectangle : 1;
  guint has_gl_framebuffer_blit : 1;
  guint has_frame_terminator : 1;
  guint has_unpack_subimage : 1;
  guint extensions_checked : 1;
  guint debug_enabled : 1;
  guint forward_compatible : 1;
  guint is_legacy : 1;

  int use_es;

  GdkGLContextPaintData *paint_data;
} GdkGLContextPrivate;

typedef struct {
  GObject parent_instance;

  /*< private >*/
  GdkVisualType type;
  gint depth;
  GdkByteOrder byte_order;
  gint colormap_size;
  gint bits_per_rgb;

  guint32 red_mask;
  guint32 green_mask;
  guint32 blue_mask;

  GdkScreen *screen;
} _GdkVisual;

typedef struct {
  _GdkVisual visual;

  Visual *xvisual;
  Colormap colormap;
} _GdkX11Visual;

_GdkWindowImplClass* getGdkWindowImplClass() {
    GType* t = g_type_from_name("GdkWindowImplX11");
    return g_type_class_ref(t);
}

void override_x11_window_invalidate_for_new_frame(_GdkWindow* window) {
    _GdkWindowImplClass* impl_class = getGdkWindowImplClass();
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
    guint blitID;
    guint blitType;

    gboolean needs_resize;
    gboolean needs_render;
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
  LAST_SIGNAL
};

static void gl_area_allocate_buffers (GLArea *area);

static guint area_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (GLArea, gl_area, GTK_TYPE_WIDGET)

static void gl_area_get_property(GObject* gobject, guint prop_id, GValue* value, GParamSpec* pspec) {}
static void gl_area_set_property (GObject* gobject, guint prop_id, const GValue* value, GParamSpec* pspec) {}

static void gl_area_dispose(GObject *gobject) {
    GLArea *area = GL_AREA (gobject);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_clear_object (&priv->context);
    G_OBJECT_CLASS (gl_area_parent_class)->dispose (gobject);
}

gboolean gdk_x11_screen_init_gl (GdkScreen *screen) {
    GdkDisplay *display = gdk_screen_get_display (screen);
    _GdkX11Display *display_x11 = (_GdkX11Display*)display;
    Display *dpy;
    int error_base, event_base;

    if (display_x11->have_glx) return TRUE;
    dpy = gdk_x11_display_get_xdisplay (display);
    if (!glXQueryExtension (dpy, &error_base, &event_base)) return FALSE;
    int screen_num = ((_GdkX11Screen*)screen)->screen_num;

    display_x11->have_glx = TRUE;
    display_x11->glx_version = epoxy_glx_version (dpy, screen_num);
    display_x11->glx_error_base = error_base;
    display_x11->glx_event_base = event_base;

    display_x11->has_glx_create_context = epoxy_has_glx_extension (dpy, screen_num, "GLX_ARB_create_context_profile");
    display_x11->has_glx_create_es2_context = epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_create_context_es2_profile");
    display_x11->has_glx_swap_interval = epoxy_has_glx_extension (dpy, screen_num, "GLX_SGI_swap_control");
    display_x11->has_glx_texture_from_pixmap = epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_texture_from_pixmap");
    display_x11->has_glx_video_sync = epoxy_has_glx_extension (dpy, screen_num, "GLX_SGI_video_sync");
    display_x11->has_glx_buffer_age = epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_buffer_age");
    display_x11->has_glx_sync_control = epoxy_has_glx_extension (dpy, screen_num, "GLX_OML_sync_control");
    display_x11->has_glx_multisample = epoxy_has_glx_extension (dpy, screen_num, "GLX_ARB_multisample");
    display_x11->has_glx_visual_rating = epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_visual_rating");

    return TRUE;
}

#define MAX_GLX_ATTRS   30

static gboolean
find_fbconfig_for_visual (GdkDisplay   *display,
			  GdkVisual    *visual,
                          GLXFBConfig  *fb_config_out,
                          GError      **error)
{
  static int attrs[MAX_GLX_ATTRS];
  _GdkX11Display *display_x11 = (_GdkX11Display*)display;
  Display* dpy = display_x11->xdisplay;
  GLXFBConfig *configs;
  int n_configs, i;
  gboolean use_rgba;
  gboolean retval = FALSE;

  _GdkX11Visual *visual_x11 = (_GdkX11Visual*)visual;
  VisualID xvisual_id = XVisualIDFromVisual(visual_x11->xvisual);

  i = 0;
  attrs[i++] = GLX_DRAWABLE_TYPE;
  attrs[i++] = GLX_WINDOW_BIT;

  attrs[i++] = GLX_RENDER_TYPE;
  attrs[i++] = GLX_RGBA_BIT;

  attrs[i++] = GLX_DOUBLEBUFFER;
  attrs[i++] = GL_TRUE;

  attrs[i++] = GLX_RED_SIZE;
  attrs[i++] = 1;
  attrs[i++] = GLX_GREEN_SIZE;
  attrs[i++] = 1;
  attrs[i++] = GLX_BLUE_SIZE;
  attrs[i++] = 1;

  use_rgba = (visual == gdk_screen_get_rgba_visual (gdk_display_get_default_screen (display)));
  if (use_rgba)
    {
      attrs[i++] = GLX_ALPHA_SIZE;
      attrs[i++] = 1;
    }
  else
    {
      attrs[i++] = GLX_ALPHA_SIZE;
      attrs[i++] = GLX_DONT_CARE;
    }

  attrs[i++] = None;

  g_assert (i < MAX_GLX_ATTRS);

  configs = glXChooseFBConfig (dpy, DefaultScreen (dpy), attrs, &n_configs);
  if (configs == NULL || n_configs == 0)
    {
      g_set_error_literal (error, GDK_GL_ERROR,
                           GDK_GL_ERROR_UNSUPPORTED_FORMAT, "No available configurations for the given pixel format");
      return FALSE;
    }

  for (i = 0; i < n_configs; i++)
    {
      XVisualInfo *visinfo;

      visinfo = glXGetVisualFromFBConfig (dpy, configs[i]);
      if (visinfo == NULL)
        continue;

      if (visinfo->visualid != xvisual_id)
        {
          XFree (visinfo);
          continue;
        }

      if (fb_config_out != NULL)
        *fb_config_out = configs[i];

      XFree (visinfo);
      retval = TRUE;
      goto out;
    }

  g_set_error (error, GDK_GL_ERROR,
               GDK_GL_ERROR_UNSUPPORTED_FORMAT, "No available configurations for the given RGBA pixel format");

out:
  XFree (configs);

  return retval;
}

GdkGLContext* x11_window_create_gl_context(GdkWindow* window, gboolean attached, GdkGLContext *share, GError**error) {
    /*GLXFBConfig config;

    GdkDisplay* display = gdk_window_get_display (window);

    if (!gdk_x11_screen_init_gl (gdk_window_get_screen (window))) {
        g_set_error_literal(error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "No GL implementation is available");
        return NULL;
    }

    GdkVisual* visual = gdk_window_get_visual (window);
    if (!find_fbconfig_for_visual (display, visual, &config, error)) return NULL;

    _GdkX11GLContext* context = g_object_new(GDK_TYPE_X11_GL_CONTEXT,
                          "display", display, "window", window,
                          "shared-context", share, NULL);

    context->glx_config = config;
    context->is_attached = attached;

    return GDK_GL_CONTEXT (context);*/

  GdkDisplay *display;
  _GdkX11GLContext *context;
  GdkVisual *visual;
  GLXFBConfig config;

  display = gdk_window_get_display (window);

  if (!gdk_x11_screen_init_gl (gdk_window_get_screen (window)))
    {
      g_set_error_literal (error, GDK_GL_ERROR,
                           GDK_GL_ERROR_NOT_AVAILABLE,
                           "No GL implementation is available");
      return NULL;
    }

  visual = gdk_window_get_visual (window);
  if (!find_fbconfig_for_visual (display, visual, &config, error))
    return NULL;

  context = g_object_new (GDK_TYPE_X11_GL_CONTEXT,
                          "display", display,
                          "window", window,
                          "shared-context", share,
                          NULL);

  context->glx_config = config;
  context->is_attached = attached;

  return GDK_GL_CONTEXT (context);
}

typedef struct {
  GdkDisplay *display;

  GLXDrawable glx_drawable;

  Window dummy_xwin;
  GLXWindow dummy_glx;

  guint32 last_frame_counter;
} DrawableInfo;

GdkGLContextPrivate* gdk_gl_context_get_instance_private(GdkGLContext* context) {
    return ( (GdkGLContextPrivate *) ((char *) context
              + g_type_class_get_instance_private_offset(
                          ((GTypeInstance *)context)->g_class )));
}

void
gdk_gl_context_set_is_legacy (GdkGLContext *context,
                              gboolean      is_legacy)
{
  GdkGLContextPrivate *priv = gdk_gl_context_get_instance_private (context);

  priv->is_legacy = !!is_legacy;
}

static GLXContext
create_gl3_context (GdkDisplay   *display,
                    GLXFBConfig   config,
                    GdkGLContext *share,
                    int           profile,
                    int           flags,
                    int           major,
                    int           minor)
{
  int attrib_list[] = {
    GLX_CONTEXT_PROFILE_MASK_ARB, profile,
    GLX_CONTEXT_MAJOR_VERSION_ARB, major,
    GLX_CONTEXT_MINOR_VERSION_ARB, minor,
    GLX_CONTEXT_FLAGS_ARB, flags,
    None,
  };
  GLXContext res;

  _GdkX11GLContext *share_x11 = NULL;

  if (share != NULL)
    share_x11 = (_GdkX11GLContext*) (share);

  gdk_x11_display_error_trap_push (display);

  res = glXCreateContextAttribsARB (gdk_x11_display_get_xdisplay (display),
                                    config,
                                    share_x11 != NULL ? share_x11->glx_context : NULL,
                                    True,
                                    attrib_list);

  if (gdk_x11_display_error_trap_pop (display))
    return NULL;

  return res;
}

static XVisualInfo *
find_xvisinfo_for_fbconfig (GdkDisplay  *display,
                            GLXFBConfig  config)
{
  Display *dpy = gdk_x11_display_get_xdisplay (display);

  return glXGetVisualFromFBConfig (dpy, config);
}

static void
drawable_info_free (gpointer data_)
{
  DrawableInfo *data = data_;
  Display *dpy;

  gdk_x11_display_error_trap_push (data->display);

  dpy = gdk_x11_display_get_xdisplay (data->display);

  if (data->glx_drawable)
    glXDestroyWindow (dpy, data->glx_drawable);

  if (data->dummy_glx)
    glXDestroyWindow (dpy, data->dummy_glx);

  if (data->dummy_xwin)
    XDestroyWindow (dpy, data->dummy_xwin);

  gdk_x11_display_error_trap_pop_ignored (data->display);

  g_slice_free (DrawableInfo, data);
}

static void
set_glx_drawable_info (GdkWindow    *window,
                       DrawableInfo *info)
{
  g_object_set_data_full (G_OBJECT (window), "-gdk-x11-window-glx-info",
                          info,
                          drawable_info_free);
}

static DrawableInfo *
get_glx_drawable_info (GdkWindow *window)
{
  return g_object_get_data (G_OBJECT (window), "-gdk-x11-window-glx-info");
}

static GLXContext
create_legacy_context (GdkDisplay   *display,
                       GLXFBConfig   config,
                       GdkGLContext *share)
{
  _GdkX11GLContext *share_x11 = NULL;
  GLXContext res;

  if (share != NULL)
    share_x11 = GDK_X11_GL_CONTEXT (share);

  gdk_x11_display_error_trap_push (display);

  res = glXCreateNewContext (gdk_x11_display_get_xdisplay (display),
                             config,
                             GLX_RGBA_TYPE,
                             share_x11 != NULL ? share_x11->glx_context : NULL,
                             TRUE);

  if (gdk_x11_display_error_trap_pop (display))
    return NULL;

  return res;
}

static gboolean
gdk_x11_gl_context_realize (GdkGLContext  *context,
                            GError       **error)
{
  _GdkX11Display *display_x11;
  GdkDisplay *display;
  _GdkX11GLContext *context_x11;
  GLXWindow drawable;
  XVisualInfo *xvisinfo;
  Display *dpy;
  DrawableInfo *info;
  GdkGLContext *share;
  _GdkWindow *window;
  gboolean debug_bit, compat_bit, legacy_bit, es_bit;
  int major, minor, flags;

  window = gdk_gl_context_get_window (context);
  display = gdk_window_get_display (window);
  dpy = gdk_x11_display_get_xdisplay (display);
  context_x11 = (_GdkX11GLContext*) (context);
  display_x11 = (_GdkX11Display*) (display);
  share = gdk_gl_context_get_shared_context (context);

  gdk_gl_context_get_required_version (context, &major, &minor);
  debug_bit = gdk_gl_context_get_debug_enabled (context);
  compat_bit = gdk_gl_context_get_forward_compatible (context);

  // If there is no glXCreateContextAttribsARB() then we default to legacy
  legacy_bit = TRUE;
  es_bit = FALSE;

  // We cannot share legacy contexts with core profile ones, so the
  // shared context is the one that decides if we're going to create
  // a legacy context or not.
  if (share != NULL && gdk_gl_context_is_legacy (share))
    legacy_bit = TRUE;

  flags = 0;
  if (debug_bit)
    flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
  if (compat_bit)
    flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

  // If we have access to GLX_ARB_create_context_profile then we can ask for
  // a compatibility profile; if we don't, then we have to fall back to the
  // old GLX 1.3 API.

   _GdkX11Display* displayX11 = (_GdkX11Display*)display;
  if (legacy_bit && !displayX11->has_glx_create_context)
    {
      context_x11->glx_context = create_legacy_context (display, context_x11->glx_config, share);
    }
  else
    {
      int profile;

      if (es_bit)
        profile = GLX_CONTEXT_ES2_PROFILE_BIT_EXT;
      else
        profile = legacy_bit ? GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
                             : GLX_CONTEXT_CORE_PROFILE_BIT_ARB;

      // We need to tweak the version, otherwise we may end up requesting
      // a compatibility context with a minimum version of 3.2, which is
      // an error
      if (legacy_bit)
        {
          major = 3;
          minor = 0;
        }

      context_x11->glx_context = create_gl3_context (display,
                                                     context_x11->glx_config,
                                                     share,
                                                     profile, flags, major, minor);

      // Fall back to legacy in case the GL3 context creation failed
      if (context_x11->glx_context == NULL)
        {
          context_x11->glx_context = create_legacy_context (display, context_x11->glx_config, share);
          legacy_bit = TRUE;
          es_bit = FALSE;
        }
    }

  if (context_x11->glx_context == NULL)
    {
      g_set_error_literal (error, GDK_GL_ERROR,
                           GDK_GL_ERROR_NOT_AVAILABLE, "Unable to create a GL context");
      return FALSE;
    }

  // Ensure that any other context is created with a legacy bit set
  gdk_gl_context_set_is_legacy (context, legacy_bit);

  // Ensure that any other context is created with a ES bit set
  gdk_gl_context_set_use_es (context, es_bit);

  xvisinfo = find_xvisinfo_for_fbconfig (display, context_x11->glx_config);

  info = get_glx_drawable_info (window->impl_window);
  if (info == NULL)
    {
      XSetWindowAttributes attrs;
      unsigned long mask;

      gdk_x11_display_error_trap_push (display);

      info = g_slice_new0 (DrawableInfo);
      info->display = display;
      info->last_frame_counter = 0;

      attrs.override_redirect = True;
      attrs.colormap = XCreateColormap (dpy, DefaultRootWindow (dpy), xvisinfo->visual, AllocNone);
      attrs.border_pixel = 0;
      mask = CWOverrideRedirect | CWColormap | CWBorderPixel;
      info->dummy_xwin = XCreateWindow (dpy, DefaultRootWindow (dpy),
                                        -100, -100, 1, 1,
                                        0,
                                        xvisinfo->depth,
                                        CopyFromParent,
                                        xvisinfo->visual,
                                        mask,
                                        &attrs);
      XMapWindow(dpy, info->dummy_xwin);

      if (displayX11->glx_version >= 13)
        {
          info->glx_drawable = glXCreateWindow (dpy, context_x11->glx_config,
                                                gdk_x11_window_get_xid (window->impl_window),
                                                NULL);
          info->dummy_glx = glXCreateWindow (dpy, context_x11->glx_config, info->dummy_xwin, NULL);
        }

      if (gdk_x11_display_error_trap_pop (display))
        {
          g_set_error_literal (error, GDK_GL_ERROR,
                               GDK_GL_ERROR_NOT_AVAILABLE, "Unable to create a GL context");

          XFree (xvisinfo);
          drawable_info_free (info);
          glXDestroyContext (dpy, context_x11->glx_context);
          context_x11->glx_context = NULL;

          return FALSE;
        }

      set_glx_drawable_info (window->impl_window, info);
    }

  XFree (xvisinfo);

  if (context_x11->is_attached)
    drawable = info->glx_drawable ? info->glx_drawable : gdk_x11_window_get_xid (window->impl_window);
  else
    drawable = info->dummy_glx ? info->dummy_glx : info->dummy_xwin;

  context_x11->is_direct = glXIsDirect (dpy, context_x11->glx_context);
  context_x11->drawable = drawable;

  return TRUE;
}

/*gboolean gdk_gl_context_realize (GdkGLContext  *context, GError       **error)
{
  GdkGLContextPrivate *priv = gdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), FALSE);

  if (priv->realized) return TRUE;

  priv->realized = gdk_x11_gl_context_realize (context, error);

  return priv->realized;
}*/

GdkGLContext *
gdk_window_get_paint_gl_context (_GdkWindow  *window,
                                 GError    **error)
{
  GError *internal_error = NULL;

  _GdkWindow* iwindow = (_GdkWindow*)window->impl_window;
  if (iwindow->gl_paint_context == NULL)
    {
      _GdkWindowImplClass *impl_class = getGdkWindowImplClass();

      if (impl_class->create_gl_context == NULL)
        {
          g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "The current backend does not support OpenGL");
          printf(" gdk_window_get_paint_gl_context failed, no impl_class->create_gl_context!\n");
          return NULL;
        }

      iwindow->gl_paint_context =
        impl_class->create_gl_context (iwindow,
                                       TRUE,
                                       NULL,
                                       &internal_error);
    }

  if (internal_error != NULL)
    {
      g_propagate_error (error, internal_error);
      g_clear_object (&(iwindow->gl_paint_context));
          printf(" gdk_window_get_paint_gl_context failed, internal_error set\n");
      return NULL;
    }

  gdk_gl_context_realize (iwindow->gl_paint_context, &internal_error);
  if (internal_error != NULL)
    {
      g_propagate_error (error, internal_error);
      g_clear_object (&(iwindow->gl_paint_context));
          printf(" gdk_window_get_paint_gl_context failed, gdk_gl_context_realize failed\n");
      return NULL;
    }

  return iwindow->gl_paint_context;
}

GdkGLContext *
_gdk_window_create_gl_context (_GdkWindow    *window,
                              GError      **error)
{
  GdkGLContext *paint_context;

  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  paint_context = gdk_window_get_paint_gl_context (window, error);
    printf("_gdk_window_create_gl_context paint_context: %p\n", paint_context);
  if (paint_context == NULL) return NULL;

  return x11_window_create_gl_context (window->impl_window, FALSE, paint_context, error);
}

static GdkGLContext* gl_area_real_create_context(GLArea *area) {
    printf("gl_area_real_create_context\n");
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    GtkWidget *widget = GTK_WIDGET (area);
    GError *error = NULL;
    GdkGLContext *context;

#ifndef _WIN32
    override_x11_window_invalidate_for_new_frame( gtk_widget_get_window(widget) );
#endif

    //context = gdk_window_create_gl_context (gtk_widget_get_window (widget), &error);
    context = _gdk_window_create_gl_context (gtk_widget_get_window (widget), &error);
    //_GdkWindow* win = gtk_widget_get_window(widget);
    //context = x11_window_create_gl_context(win->impl_window, TRUE, 0, &error);
    if (error != NULL) {
        gl_area_set_error (area, error);
        g_clear_object (&context);
        g_clear_error (&error);
        return NULL;
    }

    gdk_gl_context_set_use_es (context, 0);
    gdk_gl_context_set_required_version (context, 3, 2);

    gdk_gl_context_realize (context, &error);
    if (error != NULL) {
        gl_area_set_error (area, error);
        g_clear_object (&context);
        g_clear_error (&error);
        return NULL;
    }

    gdk_gl_context_make_current(context);
    initGLFunctions();

    return context;
}

static void gl_area_realize (GtkWidget *widget) {
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

    priv->event_window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
    gtk_widget_register_window (widget, priv->event_window);

    g_clear_error (&priv->error);
    priv->context = gl_area_real_create_context(area);
    priv->needs_resize = TRUE;
}

static void gl_area_notify(GObject* object, GParamSpec* pspec) {
    if (strcmp (pspec->name, "scale-factor") == 0) {
        GLArea *area = GL_AREA (object);
        GLAreaPrivate *priv = gl_area_get_instance_private (area);
        priv->needs_resize = TRUE;
    }

    if (G_OBJECT_CLASS (gl_area_parent_class)->notify)
        G_OBJECT_CLASS (gl_area_parent_class)->notify (object, pspec);
}


static void gl_area_resize (GLArea *area, int width, int height) {
    glViewport (0, 0, width, height);
}

static void gl_area_ensure_buffers (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    GtkWidget *widget = GTK_WIDGET (area);

    gtk_widget_realize (widget);

    if (priv->context == NULL) return;
    if (priv->have_buffers) return;

    priv->have_buffers = TRUE;

    glGenFramebuffers (1, &priv->frame_buffer);

    if (priv->render_buffer == 0) glGenRenderbuffers (1, &priv->render_buffer);
    if (priv->depth_stencil_buffer == 0) glGenRenderbuffers (1, &priv->depth_stencil_buffer);

    gl_area_allocate_buffers (area);
}

static void gl_area_allocate_buffers (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    GtkWidget *widget = GTK_WIDGET (area);
    int scale, width, height;

    if (priv->context == NULL) return;

    scale = gtk_widget_get_scale_factor (widget);
    width = gtk_widget_get_allocated_width (widget) * scale;
    height = gtk_widget_get_allocated_height (widget) * scale;

    if (priv->render_buffer) {
        glBindRenderbuffer (GL_RENDERBUFFER, priv->render_buffer);
        if (priv->samples == 0) glRenderbufferStorage (GL_RENDERBUFFER, GL_RGB8, width, height);
        else glRenderbufferStorageMultisample (GL_RENDERBUFFER, priv->samples, GL_RGB8, width, height);
    }

    glBindRenderbuffer (GL_RENDERBUFFER, priv->depth_stencil_buffer);
    if (priv->samples == 0) glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    else glRenderbufferStorageMultisample(GL_RENDERBUFFER, priv->samples, GL_DEPTH24_STENCIL8, width, height);

    priv->needs_render = TRUE;
}

void gl_area_attach_buffers (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);

    g_return_if_fail (IS_GL_AREA (area));

    if (priv->context == NULL) return;

    gl_area_make_current (area);

    if (!priv->have_buffers) gl_area_ensure_buffers (area);
    else if (priv->needs_resize) gl_area_allocate_buffers (area);

    glBindFramebuffer (GL_FRAMEBUFFER_EXT, priv->frame_buffer);

    glFramebufferRenderbuffer (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,  GL_RENDERBUFFER_EXT, priv->render_buffer);
    glFramebufferRenderbuffer (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,   GL_RENDERBUFFER_EXT, priv->depth_stencil_buffer);
    glFramebufferRenderbuffer (GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, priv->depth_stencil_buffer);
}

static void gl_area_delete_buffers (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);

    if (priv->context == NULL) return;

    priv->have_buffers = FALSE;

    if (priv->render_buffer != 0) {
        glDeleteRenderbuffers (1, &priv->render_buffer);
        priv->render_buffer = 0;
    }

    if (priv->depth_stencil_buffer != 0) {
        glDeleteRenderbuffers (1, &priv->depth_stencil_buffer);
        priv->depth_stencil_buffer = 0;
    }

    if (priv->frame_buffer != 0) {
        glBindFramebuffer (GL_FRAMEBUFFER_EXT, 0);
        glDeleteFramebuffers (1, &priv->frame_buffer);
        priv->frame_buffer = 0;
    }
}

static void gl_area_unrealize (GtkWidget *widget) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);

    if (priv->context != NULL) {
        if (priv->have_buffers) {
            gl_area_make_current (area);
            gl_area_delete_buffers (area);
        }

        /* Make sure to unset the context if current */
        if (priv->context == gdk_gl_context_get_current()) gdk_gl_context_clear_current ();
    }

    g_clear_object (&priv->context);
    g_clear_error (&priv->error);

    if (priv->event_window != NULL) {
        gtk_widget_unregister_window (widget, priv->event_window);
        gdk_window_destroy (priv->event_window);
        priv->event_window = NULL;
    }

    GTK_WIDGET_CLASS (gl_area_parent_class)->unrealize (widget);
}

static void gl_area_map (GtkWidget *widget) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    if (priv->event_window != NULL) gdk_window_show (priv->event_window);
    GTK_WIDGET_CLASS (gl_area_parent_class)->map (widget);
}

static void gl_area_unmap (GtkWidget *widget) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    if (priv->event_window != NULL) gdk_window_hide (priv->event_window);
    GTK_WIDGET_CLASS (gl_area_parent_class)->unmap (widget);
}

static void gl_area_size_allocate(GtkWidget* widget, GtkAllocation *allocation) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);

    GTK_WIDGET_CLASS (gl_area_parent_class)->size_allocate (widget, allocation);

    if (gtk_widget_get_realized (widget)) {
        if (priv->event_window != NULL) gdk_window_move_resize(priv->event_window,
                                allocation->x, allocation->y, allocation->width, allocation->height);

        priv->needs_resize = TRUE;
    }
}

static gboolean gl_area_draw(GtkWidget* widget, cairo_t* cr) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    gboolean unused;
    int w, h, scale;
    GLenum status;

    if (priv->error != NULL) return FALSE;
    if (priv->context == NULL) return FALSE;

    gl_area_make_current (area);
    gl_area_attach_buffers (area);

    glEnable (GL_DEPTH_TEST);

    scale = gtk_widget_get_scale_factor (widget);
    w = gtk_widget_get_allocated_width (widget) * scale;
    h = gtk_widget_get_allocated_height (widget) * scale;

    status = glCheckFramebufferStatus (GL_FRAMEBUFFER_EXT);
    if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
        if (priv->needs_render) {
            if (priv->needs_resize) {
                g_signal_emit (area, area_signals[RESIZE], 0, w, h, NULL);
                priv->needs_resize = FALSE;
            }
            g_signal_emit (area, area_signals[RENDER], 0, priv->context, &unused);
        }

        priv->needs_render = FALSE;

        //gdk_cairo_draw_from_gl (cr, gtk_widget_get_window (widget), priv->render_buffer, GL_RENDERBUFFER, scale, 0, 0, w, h);
        if (priv->blitID == 0) priv->blitID = priv->render_buffer;
        gdk_cairo_draw_from_gl (cr, gtk_widget_get_window (widget), priv->blitID, priv->blitType, scale, 0, 0, w, h);
        gl_area_make_current (area);
    } else g_warning ("fb setup not supported");

    return TRUE;
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
        gint arg1, gint arg2, gpointer data2);
    GCClosure* cc = (GCClosure*)closure;
    gpointer data1, data2;
    GMarshalFunc_VOID__INT_INT callback;

    g_return_if_fail(n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    } else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__INT_INT)(marshal_data ? marshal_data : cc->callback);

    callback(data1,
        g_marshal_value_peek_int(param_values + 1),
        g_marshal_value_peek_int(param_values + 2),
        data2);
}

#define I_(string) g_intern_static_string(string)

static void gl_area_class_init (GLAreaClass *klass) {
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

    gobject_class->set_property = gl_area_set_property;
    gobject_class->get_property = gl_area_get_property;
    gobject_class->dispose = gl_area_dispose;
    gobject_class->notify = gl_area_notify;

    g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);

    area_signals[RENDER] = g_signal_new (I_("render"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GLAreaClass, render),
                  _boolean_handled_accumulator, NULL,
                  NULL,
                  G_TYPE_BOOLEAN, 1,
                  GDK_TYPE_GL_CONTEXT);

    area_signals[RESIZE] = g_signal_new (I_("resize"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GLAreaClass, resize),
                  NULL, NULL,
                  _marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
}

static void gl_area_init (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    gtk_widget_set_has_window (GTK_WIDGET (area), FALSE);
    gtk_widget_set_app_paintable (GTK_WIDGET (area), TRUE);
    priv->needs_render = TRUE;
    priv->blitID = 0;
    priv->blitType = GL_RENDERBUFFER;
    priv->samples = 0;
}

GtkWidget* gl_area_new (void) {
    GtkWidget* obj = g_object_new (TYPE_GL_AREA, NULL);
    return obj;
}

void gl_area_set_error (GLArea    *area, const GError *error) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_if_fail (IS_GL_AREA (area));
    g_clear_error (&priv->error);
    if (error) priv->error = g_error_copy (error);
}

GError* gl_area_get_error (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_val_if_fail (IS_GL_AREA (area), NULL);
    return priv->error;
}

guint gl_area_get_samples (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_val_if_fail (IS_GL_AREA (area), FALSE);
    return priv->samples;
}

void gl_area_set_samples (GLArea *area, guint samples) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_if_fail (IS_GL_AREA (area));

    if (priv->samples != samples) {
        priv->samples = samples;
        priv->have_buffers = FALSE;
    }
}

void gl_area_set_blit_id (GLArea *area, guint ID, guint bType) {
    g_return_if_fail (IS_GL_AREA (area));
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    if (priv->blitID == ID && priv->blitType == bType) return;
    printf("gl_area_set_blit_id %i\n", ID);

    priv->blitID = ID;
    priv->blitType = bType;
}

void gl_area_queue_render (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_if_fail (IS_GL_AREA (area));
    priv->needs_render = TRUE;
    gtk_widget_queue_draw (GTK_WIDGET (area));
}

GdkGLContext* gl_area_get_context (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_val_if_fail (IS_GL_AREA (area), NULL);
    return priv->context;
}

void gl_area_make_current (GLArea *area) {
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    g_return_if_fail (IS_GL_AREA (area));
    GtkWidget* widget = GTK_WIDGET (area);
    g_return_if_fail (gtk_widget_get_realized (widget));
    if (priv->context != NULL) gdk_gl_context_make_current (priv->context);
}
