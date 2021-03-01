#ifndef GLAREALINUX_H_INCLUDED
#define GLAREALINUX_H_INCLUDED


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

typedef struct {
  _GdkDisplay parent_instance;
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

void _gdk_x11_window_invalidate_for_new_frame (_GdkWindow *window, cairo_region_t *update_area) {
    cairo_rectangle_int_t window_rect;
    GdkDisplay* display = gdk_window_get_display((GdkWindow*)window);
    _GdkX11Display *display_x11 = (_GdkX11Display*) (display);
    Display* dpy = display_x11->xdisplay;
    gboolean invalidate_all;

    //SET_DEBUG_BREAK_POINT(A,1)

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
    if (buffer_age >= 4) invalidate_all = TRUE;
    else {
        if (buffer_age == 0) invalidate_all = TRUE; // fixed black widgets here and above
        if (buffer_age >= 1) {
            if (window->old_updated_area[0]) cairo_region_union (update_area, window->old_updated_area[0]);
            else invalidate_all = TRUE;
        }
        if (buffer_age >= 2) {
            if (window->old_updated_area[1]) cairo_region_union (update_area, window->old_updated_area[1]);
            else invalidate_all = TRUE;
        }
    }

    if (invalidate_all || global_invalidate) {
        window_rect.x = 0;
        window_rect.y = 0;
        window_rect.width = gdk_window_get_width ((GdkWindow*)window);
        window_rect.height = gdk_window_get_height ((GdkWindow*)window);

        /* If nothing else is known, repaint everything so that the back
         buffer is fully up-to-date for the swapbuffer */
        cairo_region_union_rectangle (update_area, &window_rect);
        global_invalidate = FALSE;
    }
}

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
  _GdkVisual visual;

  Visual *xvisual;
  Colormap colormap;
} _GdkX11Visual;


#endif // GLAREALINUX_H_INCLUDED
