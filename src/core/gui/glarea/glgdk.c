
#define GDK_COMPILATION

#include <gdk/x11/gdkx11display.h>
#include <gdk/x11/gdkx11screen.h>
#include <gdk/x11/gdkx11visual.h>

#include "glgdk.h"

#include <X11/Xlibint.h>
#include <X11/Xutil.h>

#include <GL/gl.h>
#include <GL/glx.h>

typedef enum {
  GDK_RENDERING_MODE_SIMILAR = 0,
  GDK_RENDERING_MODE_IMAGE,
  GDK_RENDERING_MODE_RECORDING
} GdkRenderingMode;

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
} _GdkDisplay;

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

struct glvisualinfo {
  int supports_gl;
  int double_buffer;
  int stereo;
  int alpha_size;
  int depth_size;
  int stencil_size;
  int num_multisample;
  int visual_caveat;
};

static gboolean visual_compatible (const _GdkVisual *a, const _GdkVisual *b) {
  return a->type == b->type &&
    a->depth == b->depth &&
    a->red_mask == b->red_mask &&
    a->green_mask == b->green_mask &&
    a->blue_mask == b->blue_mask &&
    a->colormap_size == b->colormap_size &&
    a->bits_per_rgb == b->bits_per_rgb;
}

static _GdkVisual* pick_better_visual_for_gl(_GdkX11Screen* x11_screen, struct glvisualinfo* gl_info, _GdkVisual* compatible) {
    printf(" try to pick_better_visual_for_gl\n");
    _GdkVisual *visual;
    int i;

    // wish for:
    /*static int attrsF1[] = {
          GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,     k
          GLX_RENDER_TYPE     , GLX_RGBA_BIT,       k
          GLX_DOUBLEBUFFER    , GL_TRUE,            k
          GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,     k
          GLX_X_RENDERABLE    , GL_TRUE,            k
          GLX_RED_SIZE        , 8,
          GLX_GREEN_SIZE      , 8,
          GLX_BLUE_SIZE       , 8,
          GLX_ALPHA_SIZE      , GLX_DONT_CARE,      k
          GLX_DEPTH_SIZE      , 24,                 k
          GLX_STENCIL_SIZE    , 8,                  k
          GLX_SAMPLE_BUFFERS  , 1,                  k
          GLX_SAMPLES         , 4,                  k
          None
        };*/

    for (i = 0; i < x11_screen->nvisuals; i++) {
        visual = (_GdkVisual*)x11_screen->visuals[i];
        if (gl_info[i].stereo || !gl_info[i].supports_gl) continue;
        if (!gl_info[i].double_buffer) continue;
        if (gl_info[i].depth_size < 24) continue;
        if (gl_info[i].stencil_size < 8) continue;
        if (!visual_compatible (visual, compatible)) continue;
        //printf(" %i %i %i %i\n", i, visual_compatible(visual, compatible), gl_info[i].double_buffer, gl_info[i].num_multisample);
        if (gl_info[i].num_multisample >= 4) {
            printf(" pick better visual with multisampling %i\n", i);
            return visual;
        }
    }

    printf("  warning! retry without multisampling..\n");

    // retry without multisampling
    for (i = 0; i < x11_screen->nvisuals; i++) {
        visual = (_GdkVisual*)x11_screen->visuals[i];
        if (gl_info[i].stereo || !gl_info[i].supports_gl) continue;
        if (!gl_info[i].double_buffer) continue;
        if (gl_info[i].depth_size < 24) continue;
        if (gl_info[i].stencil_size < 8) continue;
        if (!visual_compatible (visual, compatible)) continue;
        printf(" pick better visual without multisampling %i\n", i);
        return visual;
    }

    printf("  failed! return compatible visual..\n");
    return compatible;
}

gboolean gdk_x11_screen_init_gl(_GdkScreen*);

void glgdk_x11_screen_update_visuals_for_gl (_GdkScreen *screen) {
  printf("glgdk_x11_screen_update_visuals_for_gl\n");
  struct glvisualinfo* gl_info;

  _GdkX11Screen* x11_screen = (_GdkX11Screen*)GDK_X11_SCREEN (screen);
  GdkDisplay* display = x11_screen->display;
  _GdkX11Display* display_x11 = (_GdkX11Display*)GDK_X11_DISPLAY(display);
  Display* dpy = gdk_x11_display_get_xdisplay (display);

  if (!gdk_x11_screen_init_gl (screen)) {
      printf(" gdk_x11_screen_init_gl failed!\n");
      return;
  }

  gl_info = g_new0 (struct glvisualinfo, x11_screen->nvisuals);

  for (int i = 0; i < x11_screen->nvisuals; i++) {
      XVisualInfo *visual_list;
      XVisualInfo visual_template;
      int nxvisuals;

      visual_template.screen = x11_screen->screen_num;
      visual_template.visualid = gdk_x11_visual_get_xvisual (x11_screen->visuals[i])->visualid;
      visual_list = XGetVisualInfo (x11_screen->xdisplay, VisualIDMask| VisualScreenMask, &visual_template, &nxvisuals);

      if (visual_list == NULL)
        continue;

      glXGetConfig (dpy, &visual_list[0], GLX_USE_GL, &gl_info[i].supports_gl);
      glXGetConfig (dpy, &visual_list[0], GLX_DOUBLEBUFFER, &gl_info[i].double_buffer);
      glXGetConfig (dpy, &visual_list[0], GLX_STEREO, &gl_info[i].stereo);
      glXGetConfig (dpy, &visual_list[0], GLX_ALPHA_SIZE, &gl_info[i].alpha_size);
      glXGetConfig (dpy, &visual_list[0], GLX_DEPTH_SIZE, &gl_info[i].depth_size);
      glXGetConfig (dpy, &visual_list[0], GLX_STENCIL_SIZE, &gl_info[i].stencil_size);

      if (display_x11->has_glx_multisample)
        glXGetConfig(dpy, &visual_list[0], GLX_SAMPLES, &gl_info[i].num_multisample);

      if (display_x11->has_glx_visual_rating)
        glXGetConfig(dpy, &visual_list[0], GLX_VISUAL_CAVEAT_EXT, &gl_info[i].visual_caveat);
      else
        gl_info[i].visual_caveat = GLX_NONE_EXT;

      XFree (visual_list);
    }

  x11_screen->system_visual = (GdkVisual*)pick_better_visual_for_gl (x11_screen, gl_info, (_GdkVisual*)x11_screen->system_visual);
  if (x11_screen->rgba_visual)
    x11_screen->rgba_visual = (GdkVisual*)pick_better_visual_for_gl (x11_screen, gl_info, (_GdkVisual*)x11_screen->rgba_visual);

  g_free (gl_info);
}

void replace_gl_visuals() {
    GdkDisplay* display = gdk_display_get_default();
    GdkScreen* screen = gdk_display_get_default_screen(display);
    glgdk_x11_screen_update_visuals_for_gl((_GdkScreen*)screen);
}
