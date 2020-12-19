
#define GDK_COMPILATION

#include <gdk/x11/gdkx11display.h>
#include <gdk/x11/gdkx11screen.h>
#include <gdk/x11/gdkx11visual.h>
#include <gdk/x11/gdkx11monitor.h>

#include "glgdk.h"

#include <X11/Xatom.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>

#include <GL/gl.h>
#include <GL/glx.h>

typedef enum {
  GDK_RENDERING_MODE_SIMILAR = 0,
  GDK_RENDERING_MODE_IMAGE,
  GDK_RENDERING_MODE_RECORDING
} GdkRenderingMode;

enum {
  OPENED,
  CLOSED,
  SEAT_ADDED,
  SEAT_REMOVED,
  MONITOR_ADDED,
  MONITOR_REMOVED,
  LAST_SIGNAL
};

enum {
  INVALIDATE,
  MLAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
static guint msignals[MLAST_SIGNAL] = { 0 };

typedef struct {
  GObject parent;

  GdkDisplay *display;
  char *manufacturer;
  char *model;
  GdkRectangle geometry;
  int width_mm;
  int height_mm;
  int scale_factor;
  int refresh_rate;
  GdkSubpixelLayout subpixel_layout;
} _GdkMonitor;

typedef struct {
  _GdkMonitor parent;

  XID output;
  guint add     : 1;
  guint remove  : 1;
} _GdkX11Monitor;

typedef struct {
  gint	         fd;
  GSource	*source;
  Display	*display;
} _GdkInternalConnection;

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

typedef struct {
  _GdkVisual visual;

  Visual *xvisual;
  Colormap colormap;
} _GdkX11Visual;

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

static const char *const precache_atoms[] = {
  "UTF8_STRING",
  "WM_CLIENT_LEADER",
  "WM_DELETE_WINDOW",
  "WM_ICON_NAME",
  "WM_LOCALE_NAME",
  "WM_NAME",
  "WM_PROTOCOLS",
  "WM_TAKE_FOCUS",
  "WM_WINDOW_ROLE",
  "_NET_ACTIVE_WINDOW",
  "_NET_CURRENT_DESKTOP",
  "_NET_FRAME_EXTENTS",
  "_NET_STARTUP_ID",
  "_NET_WM_CM_S0",
  "_NET_WM_DESKTOP",
  "_NET_WM_ICON",
  "_NET_WM_ICON_NAME",
  "_NET_WM_NAME",
  "_NET_WM_PID",
  "_NET_WM_PING",
  "_NET_WM_STATE",
  "_NET_WM_STATE_ABOVE",
  "_NET_WM_STATE_BELOW",
  "_NET_WM_STATE_FULLSCREEN",
  "_NET_WM_STATE_HIDDEN",
  "_NET_WM_STATE_MODAL",
  "_NET_WM_STATE_MAXIMIZED_VERT",
  "_NET_WM_STATE_MAXIMIZED_HORZ",
  "_NET_WM_STATE_SKIP_TASKBAR",
  "_NET_WM_STATE_SKIP_PAGER",
  "_NET_WM_STATE_STICKY",
  "_NET_WM_SYNC_REQUEST",
  "_NET_WM_SYNC_REQUEST_COUNTER",
  "_NET_WM_WINDOW_TYPE",
  "_NET_WM_WINDOW_TYPE_COMBO",
  "_NET_WM_WINDOW_TYPE_DIALOG",
  "_NET_WM_WINDOW_TYPE_DND",
  "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
  "_NET_WM_WINDOW_TYPE_MENU",
  "_NET_WM_WINDOW_TYPE_NORMAL",
  "_NET_WM_WINDOW_TYPE_POPUP_MENU",
  "_NET_WM_WINDOW_TYPE_TOOLTIP",
  "_NET_WM_WINDOW_TYPE_UTILITY",
  "_NET_WM_USER_TIME",
  "_NET_WM_USER_TIME_WINDOW",
  "_NET_VIRTUAL_ROOTS",
  "GDK_SELECTION",
  "_NET_WM_STATE_FOCUSED",
  "GDK_VISUALS"
};

static gboolean get_cached_gl_visuals (GdkDisplay *display, int *system, int *rgba) {
return FALSE;
  gboolean found;
  Atom type_return;
  gint format_return;
  gulong nitems_return;
  gulong bytes_after_return;
  guchar *data = NULL;
  Display *dpy;

  dpy = gdk_x11_display_get_xdisplay (display);

  found = FALSE;

  gdk_x11_display_error_trap_push (display);
  if (XGetWindowProperty (dpy, DefaultRootWindow (dpy),
                          gdk_x11_get_xatom_by_name_for_display (display, "GDK_VISUALS"),
                          0, 2, False, XA_INTEGER, &type_return,
                          &format_return, &nitems_return,
                          &bytes_after_return, &data) == Success)
    {
      if (type_return == XA_INTEGER && format_return == 32 && nitems_return == 2 && data != NULL) {
          long *visuals = (long *) data;

          *system = (int)visuals[0];
          *rgba = (int)visuals[1];
          printf(" - - - get_cached_gl_visuals %i %i\n", *system, *rgba);
          found = TRUE;
        }
    }
  gdk_x11_display_error_trap_pop_ignored (display);

  if (data) XFree (data);

  return found;
}

static gboolean
visual_compatible (const _GdkVisual *a, const _GdkVisual *b)
{
  return a->type == b->type &&
    a->depth == b->depth &&
    a->red_mask == b->red_mask &&
    a->green_mask == b->green_mask &&
    a->blue_mask == b->blue_mask &&
    a->colormap_size == b->colormap_size &&
    a->bits_per_rgb == b->bits_per_rgb;
}

static gboolean
visual_is_rgba (const _GdkVisual *visual)
{
  return
    visual->depth == 32 &&
    visual->red_mask   == 0xff0000 &&
    visual->green_mask == 0x00ff00 &&
    visual->blue_mask  == 0x0000ff;
}

static GdkVisual *
pick_better_visual_for_gl (_GdkX11Screen *x11_screen, struct glvisualinfo *gl_info, _GdkVisual *compatible) {
    printf(" try to pick_better_visual_for_gl\n");
  _GdkVisual *visual;
  int i;
  gboolean want_alpha = visual_is_rgba (compatible);

  /* First look for "perfect match", i.e:
   * supports gl
   * double buffer
   * alpha iff visual is an rgba visual
   * no unnecessary stuff
   */
#ifdef NOPE
  for (i = 0; i < x11_screen->nvisuals; i++) {
      visual = x11_screen->visuals[i];
      if (visual_compatible (visual, compatible) &&
          gl_info[i].supports_gl &&
          gl_info[i].double_buffer &&
          !gl_info[i].stereo &&
          (want_alpha ? (gl_info[i].alpha_size > 0) : (gl_info[i].alpha_size == 0)) &&
          (gl_info[i].depth_size == 0) &&
          (gl_info[i].stencil_size == 0) &&
          (gl_info[i].num_multisample == 0) &&
          (gl_info[i].visual_caveat == GLX_NONE_EXT)) {
              printf(" pick_better_visual_for_gl 1\n");
            return visual;
        }
    }

  if (!want_alpha) {
      /* Next, allow alpha even if we don't want it: */
      for (i = 0; i < x11_screen->nvisuals; i++)
        {
          visual = x11_screen->visuals[i];
          if (visual_compatible (visual, compatible) &&
              gl_info[i].supports_gl &&
              gl_info[i].double_buffer &&
              !gl_info[i].stereo &&
              (gl_info[i].depth_size == 0) &&
              (gl_info[i].stencil_size == 0) &&
              (gl_info[i].num_multisample == 0) &&
              (gl_info[i].visual_caveat == GLX_NONE_EXT)) {
              printf(" pick_better_visual_for_gl 2\n");
                return visual;
            }
        }
    }

  /* Next, allow depth and stencil buffers: */
  for (i = 0; i < x11_screen->nvisuals; i++)
    {
      visual = x11_screen->visuals[i];
      if (visual_compatible (visual, compatible) &&
          gl_info[i].supports_gl &&
          gl_info[i].double_buffer &&
          !gl_info[i].stereo &&
          (gl_info[i].num_multisample == 0) &&
          (gl_info[i].visual_caveat == GLX_NONE_EXT)) {
              printf(" pick_better_visual_for_gl 3\n");
            return visual;
        }
    }
#endif

  /* Next, allow multisample: */
  for (i = 0; i < x11_screen->nvisuals; i++) {
      visual = x11_screen->visuals[i];
      if (gl_info[i].stereo || !gl_info[i].supports_gl) continue;
      printf(" %i %i %i %i\n", i, visual_compatible(visual, compatible), gl_info[i].double_buffer, gl_info[i].num_multisample);
      if (visual_compatible (visual, compatible) &&
          gl_info[i].double_buffer &&
          (gl_info[i].num_multisample >= 4)
          // && (gl_info[i].visual_caveat == GLX_NONE_EXT))
          ) {
              printf(" pick_better_visual_for_gl 4\n");
            return visual;
        }
    }

  printf("  failed! return compatible visual..\n");
  return compatible;
}

static void
save_cached_gl_visuals (GdkDisplay *display, int system, int rgba)
{
  long visualdata[2];
  Display *dpy;

  dpy = gdk_x11_display_get_xdisplay (display);

  visualdata[0] = system;
  visualdata[1] = rgba;

  gdk_x11_display_error_trap_push (display);
  XChangeProperty (dpy, DefaultRootWindow (dpy),
                   gdk_x11_get_xatom_by_name_for_display (display, "GDK_VISUALS"),
                   XA_INTEGER, 32, PropModeReplace,
                   (unsigned char *)visualdata, 2);
  gdk_x11_display_error_trap_pop_ignored (display);
}

void glgdk_x11_screen_update_visuals_for_gl (_GdkScreen *screen) {
  printf("glgdk_x11_screen_update_visuals_for_gl\n");
  _GdkX11Screen *x11_screen;
  _GdkDisplay *display;
  _GdkX11Display *display_x11;
  Display *dpy;
  struct glvisualinfo *gl_info;
  int i;
  int system_visual_id, rgba_visual_id;

  x11_screen = GDK_X11_SCREEN (screen);
  display = x11_screen->display;
  display_x11 = GDK_X11_DISPLAY (display);
  dpy = gdk_x11_display_get_xdisplay (display);

  /* We save the default visuals as a property on the root window to avoid
     having to initialize GL each time, as it may not be used later. */
  if (get_cached_gl_visuals (display, &system_visual_id, &rgba_visual_id)) {
      printf(" got cached gl visuals IDs %i %i\n", system_visual_id, rgba_visual_id);
      for (i = 0; i < x11_screen->nvisuals; i++)
        {
          GdkVisual *visual = x11_screen->visuals[i];
          int visual_id = gdk_x11_visual_get_xvisual (visual)->visualid;

          if (visual_id == system_visual_id)
            x11_screen->system_visual = visual;
          if (visual_id == rgba_visual_id)
            x11_screen->rgba_visual = visual;
        }

      return;
    }

  if (!gdk_x11_screen_init_gl (screen)) {
      printf(" gdk_x11_screen_init_gl failed!\n");
      return;
  }

  gl_info = g_new0 (struct glvisualinfo, x11_screen->nvisuals);

  for (i = 0; i < x11_screen->nvisuals; i++)
    {
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

  x11_screen->system_visual = pick_better_visual_for_gl (x11_screen, gl_info, x11_screen->system_visual);
  if (x11_screen->rgba_visual)
    x11_screen->rgba_visual = pick_better_visual_for_gl (x11_screen, gl_info, x11_screen->rgba_visual);

  g_free (gl_info);

  save_cached_gl_visuals (display,
                          gdk_x11_visual_get_xvisual (x11_screen->system_visual)->visualid,
                          x11_screen->rgba_visual ? gdk_x11_visual_get_xvisual (x11_screen->rgba_visual)->visualid : 0);
}



void replace_gl_visuals() {
    GdkDisplay* display = gdk_display_get_default();
    GdkScreen* screen = gdk_display_get_default_screen(display);
    glgdk_x11_screen_update_visuals_for_gl(screen);
}

static guint
gdk_visual_hash (Visual *key)
{
  return key->visualid;
}

static gboolean
gdk_visual_equal (Visual *a,
                  Visual *b)
{
  return (a->visualid == b->visualid);
}

static void gdk_visual_add (_GdkVisual *visual) {
  _GdkX11Screen *x11_screen = GDK_X11_SCREEN (visual->screen);

  if (!x11_screen->visual_hash)
    x11_screen->visual_hash = g_hash_table_new ((GHashFunc) gdk_visual_hash,
                                                (GEqualFunc) gdk_visual_equal);
  _GdkX11Visual* xv = GDK_X11_VISUAL (visual);
  g_hash_table_insert (x11_screen->visual_hash, xv->xvisual, visual);
}

void glgdk_x11_screen_init_visuals (_GdkScreen *screen) {
  printf("glgdk_x11_screen_init_visuals\n");
  static const gint possible_depths[8] = { 32, 30, 24, 16, 15, 8, 4, 1 };
  static const GdkVisualType possible_types[6] =
    {
      GDK_VISUAL_DIRECT_COLOR,
      GDK_VISUAL_TRUE_COLOR,
      GDK_VISUAL_PSEUDO_COLOR,
      GDK_VISUAL_STATIC_COLOR,
      GDK_VISUAL_GRAYSCALE,
      GDK_VISUAL_STATIC_GRAY
    };

  _GdkX11Screen *x11_screen;
  XVisualInfo *visual_list;
  XVisualInfo visual_template;
  _GdkVisual *temp_visual;
  Visual *default_xvisual;
  _GdkVisual **visuals;
  int nxvisuals;
  int nvisuals;
  int i, j;

  g_return_if_fail (GDK_IS_SCREEN (screen));
  x11_screen = GDK_X11_SCREEN (screen);

  nxvisuals = 0;
  visual_template.screen = x11_screen->screen_num;
  visual_list = XGetVisualInfo (x11_screen->xdisplay, VisualScreenMask, &visual_template, &nxvisuals);

  visuals = g_new (GdkVisual *, nxvisuals);
  for (i = 0; i < nxvisuals; i++)
    visuals[i] = g_object_new (GDK_TYPE_X11_VISUAL, NULL);

  default_xvisual = DefaultVisual (x11_screen->xdisplay, x11_screen->screen_num);

  nvisuals = 0;
  for (i = 0; i < nxvisuals; i++)
    {
      visuals[nvisuals]->screen = screen;

      if (visual_list[i].depth >= 1)
	{
#ifdef __cplusplus
	  switch (visual_list[i].c_class)
#else /* __cplusplus */
	  switch (visual_list[i].class)
#endif /* __cplusplus */
	    {
	    case StaticGray:
	      visuals[nvisuals]->type = GDK_VISUAL_STATIC_GRAY;
	      break;
	    case GrayScale:
	      visuals[nvisuals]->type = GDK_VISUAL_GRAYSCALE;
	      break;
	    case StaticColor:
	      visuals[nvisuals]->type = GDK_VISUAL_STATIC_COLOR;
	      break;
	    case PseudoColor:
	      visuals[nvisuals]->type = GDK_VISUAL_PSEUDO_COLOR;
	      break;
	    case TrueColor:
	      visuals[nvisuals]->type = GDK_VISUAL_TRUE_COLOR;
	      break;
	    case DirectColor:
	      visuals[nvisuals]->type = GDK_VISUAL_DIRECT_COLOR;
	      break;
	    }

	  visuals[nvisuals]->depth = visual_list[i].depth;
	  visuals[nvisuals]->byte_order =
	    (ImageByteOrder(x11_screen->xdisplay) == LSBFirst) ?
	    GDK_LSB_FIRST : GDK_MSB_FIRST;
	  visuals[nvisuals]->red_mask = visual_list[i].red_mask;
	  visuals[nvisuals]->green_mask = visual_list[i].green_mask;
	  visuals[nvisuals]->blue_mask = visual_list[i].blue_mask;
	  visuals[nvisuals]->colormap_size = visual_list[i].colormap_size;
	  visuals[nvisuals]->bits_per_rgb = visual_list[i].bits_per_rgb;
	  _GdkX11Visual* v = (_GdkX11Visual*)visuals[nvisuals];
	  v->xvisual = visual_list[i].visual;

	  if ((visuals[nvisuals]->type != GDK_VISUAL_TRUE_COLOR) &&
	      (visuals[nvisuals]->type != GDK_VISUAL_DIRECT_COLOR))
	    {
	      visuals[nvisuals]->red_mask = 0;
	      visuals[nvisuals]->green_mask = 0;
	      visuals[nvisuals]->blue_mask = 0;
	    }

	  nvisuals += 1;
	}
    }

  if (visual_list)
    XFree (visual_list);

  for (i = 0; i < nvisuals; i++)
    {
      for (j = i+1; j < nvisuals; j++)
	{
	  if (visuals[j]->depth >= visuals[i]->depth)
	    {
	      if ((visuals[j]->depth == 8) && (visuals[i]->depth == 8))
		{
		  if (visuals[j]->type == GDK_VISUAL_PSEUDO_COLOR)
		    {
		      temp_visual = visuals[j];
		      visuals[j] = visuals[i];
		      visuals[i] = temp_visual;
		    }
		  else if ((visuals[i]->type != GDK_VISUAL_PSEUDO_COLOR) &&
			   visuals[j]->type > visuals[i]->type)
		    {
		      temp_visual = visuals[j];
		      visuals[j] = visuals[i];
		      visuals[i] = temp_visual;
		    }
		}
	      else if ((visuals[j]->depth > visuals[i]->depth) ||
		       ((visuals[j]->depth == visuals[i]->depth) &&
			(visuals[j]->type > visuals[i]->type)))
		{
		  temp_visual = visuals[j];
		  visuals[j] = visuals[i];
		  visuals[i] = temp_visual;
		}
	    }
	}
    }

  for (i = 0; i < nvisuals; i++)
    {
	  _GdkX11Visual* v = (_GdkX11Visual*)visuals[i];
      if (default_xvisual->visualid == v->xvisual->visualid)
         {
           x11_screen->system_visual = visuals[i];
           v->colormap = DefaultColormap (x11_screen->xdisplay, x11_screen->screen_num);
         }

      /* For now, we only support 8888 ARGB for the "rgba visual".
       * Additional formats (like ABGR) could be added later if they
       * turn up.
       */
      if (x11_screen->rgba_visual == NULL &&
          visuals[i]->depth == 32 &&
	  (visuals[i]->red_mask   == 0xff0000 &&
	   visuals[i]->green_mask == 0x00ff00 &&
	   visuals[i]->blue_mask  == 0x0000ff))
	{
	  x11_screen->rgba_visual = visuals[i];
        }
    }

#ifdef G_ENABLE_DEBUG
  if (GDK_DEBUG_CHECK (MISC))
    {
      static const gchar *const visual_names[] =
      {
        "static gray",
        "grayscale",
        "static color",
        "pseudo color",
        "true color",
        "direct color",
      };

      for (i = 0; i < nvisuals; i++)
        g_message ("visual: %s: %d", visual_names[visuals[i]->type], visuals[i]->depth);
    }
#endif /* G_ENABLE_DEBUG */

  x11_screen->navailable_depths = 0;
  for (i = 0; i < G_N_ELEMENTS (possible_depths); i++)
    {
      for (j = 0; j < nvisuals; j++)
	{
	  if (visuals[j]->depth == possible_depths[i])
	    {
	      x11_screen->available_depths[x11_screen->navailable_depths++] = visuals[j]->depth;
	      break;
	    }
	}
    }

  if (x11_screen->navailable_depths == 0)
    g_error ("unable to find a usable depth");

  x11_screen->navailable_types = 0;
  for (i = 0; i < G_N_ELEMENTS (possible_types); i++)
    {
      for (j = 0; j < nvisuals; j++)
	{
	  if (visuals[j]->type == possible_types[i])
	    {
	      x11_screen->available_types[x11_screen->navailable_types++] = visuals[j]->type;
	      break;
	    }
	}
    }

  for (i = 0; i < nvisuals; i++)
    gdk_visual_add (visuals[i]);

  if (x11_screen->navailable_types == 0)
    g_error ("unable to find a usable visual type");

  x11_screen->visuals = visuals;
  x11_screen->nvisuals = nvisuals;

  /* If GL is available we want to pick better default/rgba visuals,
     as we care about glx details such as alpha/depth/stencil depth,
     stereo and double buffering */
  glgdk_x11_screen_update_visuals_for_gl (screen);
}

static void
init_randr_support (_GdkScreen *screen)
{
  _GdkX11Screen *x11_screen = GDK_X11_SCREEN (screen);

  /* NB: This is also needed for XSettings, so don't remove. */
  XSelectInput (GDK_SCREEN_XDISPLAY (screen),
                x11_screen->xroot_window,
                StructureNotifyMask);

#ifdef HAVE_RANDR
  if (!GDK_X11_DISPLAY (gdk_screen_get_display (screen))->have_randr12)
    return;

  XRRSelectInput (GDK_SCREEN_XDISPLAY (screen),
                  x11_screen->xroot_window,
                  RRScreenChangeNotifyMask
                  | RRCrtcChangeNotifyMask
                  | RROutputPropertyNotifyMask);
#endif
}

static gboolean
init_randr15 (GdkScreen *screen, gboolean *changed)
{
#ifdef HAVE_RANDR15
  GdkDisplay *display = gdk_screen_get_display (screen);
  GdkX11Display *x11_display = GDK_X11_DISPLAY (display);
  GdkX11Screen *x11_screen = GDK_X11_SCREEN (screen);
  XRRScreenResources *resources;
  RROutput primary_output = None;
  RROutput first_output = None;
  int i;
  XRRMonitorInfo *rr_monitors;
  int num_rr_monitors;
  int old_primary;

  if (!x11_display->have_randr15)
    return FALSE;

  resources = XRRGetScreenResourcesCurrent (x11_screen->xdisplay,
                                            x11_screen->xroot_window);
  if (!resources)
    return FALSE;

  rr_monitors = XRRGetMonitors (x11_screen->xdisplay,
                                x11_screen->xroot_window,
                                True,
                                &num_rr_monitors);
  if (!rr_monitors)
    return FALSE;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      monitor->add = FALSE;
      monitor->remove = TRUE;
    }

  for (i = 0; i < num_rr_monitors; i++)
    {
      RROutput output = rr_monitors[i].outputs[0];
      XRROutputInfo *output_info;
      GdkX11Monitor *monitor;
      GdkRectangle geometry;
      GdkRectangle newgeo;
      char *name;
      int refresh_rate = 0;

      gdk_x11_display_error_trap_push (display);
      output_info = XRRGetOutputInfo (x11_screen->xdisplay, resources, output);
      if (gdk_x11_display_error_trap_pop (display))
        continue;

      if (output_info == NULL)
        continue;

      if (output_info->connection == RR_Disconnected)
        {
          XRRFreeOutputInfo (output_info);
          continue;
        }

      if (first_output == None)
        first_output = output;

      if (output_info->crtc)
        {
          XRRCrtcInfo *crtc = XRRGetCrtcInfo (x11_screen->xdisplay, resources, output_info->crtc);
          int j;

          for (j = 0; j < resources->nmode; j++)
            {
              XRRModeInfo *xmode = &resources->modes[j];
              if (xmode->id == crtc->mode)
                {
                  if (xmode->hTotal != 0 && xmode->vTotal != 0)
                    refresh_rate = (1000 * xmode->dotClock) / (xmode->hTotal * xmode->vTotal);
                  break;
                }
            }

          XRRFreeCrtcInfo (crtc);
        }

      monitor = find_monitor_by_output (x11_display, output);
      if (monitor)
        monitor->remove = FALSE;
      else
        {
          monitor = g_object_new (GDK_TYPE_X11_MONITOR,
                                  "display", display,
                                  NULL);
          monitor->output = output;
          monitor->add = TRUE;
          g_ptr_array_add (x11_display->monitors, monitor);
        }

      gdk_monitor_get_geometry (GDK_MONITOR (monitor), &geometry);
      name = g_strndup (output_info->name, output_info->nameLen);

      newgeo.x = rr_monitors[i].x / x11_screen->window_scale;
      newgeo.y = rr_monitors[i].y / x11_screen->window_scale;
      newgeo.width = rr_monitors[i].width / x11_screen->window_scale;
      newgeo.height = rr_monitors[i].height / x11_screen->window_scale;
      if (newgeo.x != geometry.x ||
          newgeo.y != geometry.y ||
          newgeo.width != geometry.width ||
          newgeo.height != geometry.height ||
          rr_monitors[i].mwidth != gdk_monitor_get_width_mm (GDK_MONITOR (monitor)) ||
          rr_monitors[i].mheight != gdk_monitor_get_height_mm (GDK_MONITOR (monitor)) ||
          g_strcmp0 (name, gdk_monitor_get_model (GDK_MONITOR (monitor))))
        *changed = TRUE;

      gdk_monitor_set_position (GDK_MONITOR (monitor), newgeo.x, newgeo.y);
      gdk_monitor_set_size (GDK_MONITOR (monitor), newgeo.width, newgeo.height);
      g_object_notify (G_OBJECT (monitor), "workarea");
      gdk_monitor_set_physical_size (GDK_MONITOR (monitor),
                                     rr_monitors[i].mwidth,
                                     rr_monitors[i].mheight);
      gdk_monitor_set_subpixel_layout (GDK_MONITOR (monitor),
                                       translate_subpixel_order (output_info->subpixel_order));
      gdk_monitor_set_refresh_rate (GDK_MONITOR (monitor), refresh_rate);
      gdk_monitor_set_scale_factor (GDK_MONITOR (monitor), x11_screen->window_scale);
      gdk_monitor_set_model (GDK_MONITOR (monitor), name);
      g_free (name);

      if (rr_monitors[i].primary)
        primary_output = monitor->output;

      XRRFreeOutputInfo (output_info);
    }

  XRRFreeMonitors (rr_monitors);
  XRRFreeScreenResources (resources);

  for (i = x11_display->monitors->len - 1; i >= 0; i--)
    {
      GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->add)
        {
          gdk_display_monitor_added (display, GDK_MONITOR (monitor));
          *changed = TRUE;
        }
      else if (monitor->remove)
        {
          g_object_ref (monitor);
          g_ptr_array_remove (x11_display->monitors, monitor);
          gdk_display_monitor_removed (display, GDK_MONITOR (monitor));
          g_object_unref (monitor);
          *changed = TRUE;
        }
    }

  old_primary = x11_display->primary_monitor;
  x11_display->primary_monitor = 0;
  for (i = 0; i < x11_display->monitors->len; ++i)
    {
      GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->output == primary_output)
        {
          x11_display->primary_monitor = i;
          break;
        }

      /* No RandR1.3+ available or no primary set, fall back to prefer LVDS as primary if present */
      if (primary_output == None &&
          g_ascii_strncasecmp (gdk_monitor_get_model (GDK_MONITOR (monitor)), "LVDS", 4) == 0)
        {
          x11_display->primary_monitor = i;
          break;
        }

      /* No primary specified and no LVDS found */
      if (monitor->output == first_output)
        x11_display->primary_monitor = i;
    }

  if (x11_display->primary_monitor != old_primary)
    *changed = TRUE;

  return x11_display->monitors->len > 0;
#endif

  return FALSE;
}

static gboolean
init_randr13 (GdkScreen *screen, gboolean *changed)
{
#ifdef HAVE_RANDR
  GdkDisplay *display = gdk_screen_get_display (screen);
  GdkX11Display *x11_display = GDK_X11_DISPLAY (display);
  GdkX11Screen *x11_screen = GDK_X11_SCREEN (screen);
  XRRScreenResources *resources;
  RROutput primary_output = None;
  RROutput first_output = None;
  int i;
  int old_primary;

  if (!x11_display->have_randr13)
      return FALSE;

  resources = XRRGetScreenResourcesCurrent (x11_screen->xdisplay,
                                            x11_screen->xroot_window);
  if (!resources)
    return FALSE;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      monitor->add = FALSE;
      monitor->remove = TRUE;
    }

  for (i = 0; i < resources->noutput; ++i)
    {
      RROutput output = resources->outputs[i];
      XRROutputInfo *output_info =
        XRRGetOutputInfo (x11_screen->xdisplay, resources, output);

      if (output_info->connection == RR_Disconnected)
        {
          XRRFreeOutputInfo (output_info);
          continue;
        }

      if (output_info->crtc)
	{
	  GdkX11Monitor *monitor;
	  XRRCrtcInfo *crtc = XRRGetCrtcInfo (x11_screen->xdisplay, resources, output_info->crtc);
          char *name;
          GdkRectangle geometry;
          GdkRectangle newgeo;
          int j;
          int refresh_rate = 0;

          for (j = 0; j < resources->nmode; j++)
            {
              XRRModeInfo *xmode = &resources->modes[j];
              if (xmode->id == crtc->mode)
                {
                  refresh_rate = (1000 * xmode->dotClock) / (xmode->hTotal *xmode->vTotal);
                  break;
                }
            }

          monitor = find_monitor_by_output (x11_display, output);
          if (monitor)
            monitor->remove = FALSE;
          else
            {
              monitor = g_object_new (gdk_x11_monitor_get_type (),
                                      "display", display,
                                      NULL);
              monitor->output = output;
              monitor->add = TRUE;
              g_ptr_array_add (x11_display->monitors, monitor);
            }

          gdk_monitor_get_geometry (GDK_MONITOR (monitor), &geometry);
          name = g_strndup (output_info->name, output_info->nameLen);

          newgeo.x = crtc->x / x11_screen->window_scale;
          newgeo.y = crtc->y / x11_screen->window_scale;
          newgeo.width = crtc->width / x11_screen->window_scale;
          newgeo.height = crtc->height / x11_screen->window_scale;
          if (newgeo.x != geometry.x ||
              newgeo.y != geometry.y ||
              newgeo.width != geometry.width ||
              newgeo.height != geometry.height ||
              output_info->mm_width != gdk_monitor_get_width_mm (GDK_MONITOR (monitor)) ||
              output_info->mm_height != gdk_monitor_get_height_mm (GDK_MONITOR (monitor)) ||
              g_strcmp0 (name, gdk_monitor_get_model (GDK_MONITOR (monitor))) != 0)
            *changed = TRUE;

          gdk_monitor_set_position (GDK_MONITOR (monitor), newgeo.x, newgeo.y);
          gdk_monitor_set_size (GDK_MONITOR (monitor), newgeo.width, newgeo.height);
          g_object_notify (G_OBJECT (monitor), "workarea");
          gdk_monitor_set_physical_size (GDK_MONITOR (monitor),
                                         output_info->mm_width,
                                         output_info->mm_height);
          gdk_monitor_set_subpixel_layout (GDK_MONITOR (monitor),
                                           translate_subpixel_order (output_info->subpixel_order));
          gdk_monitor_set_refresh_rate (GDK_MONITOR (monitor), refresh_rate);
          gdk_monitor_set_scale_factor (GDK_MONITOR (monitor), x11_screen->window_scale);
          gdk_monitor_set_model (GDK_MONITOR (monitor), name);

          g_free (name);

          XRRFreeCrtcInfo (crtc);
	}

      XRRFreeOutputInfo (output_info);
    }

  if (resources->noutput > 0)
    first_output = resources->outputs[0];

  XRRFreeScreenResources (resources);

  /* Which usable multihead data is not returned in non RandR 1.2+ X driver? */

  for (i = x11_display->monitors->len - 1; i >= 0; i--)
    {
      GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->add)
        {
          gdk_display_monitor_added (display, GDK_MONITOR (monitor));
          *changed = TRUE;
        }
      else if (monitor->remove)
        {
          g_object_ref (monitor);
          g_ptr_array_remove (x11_display->monitors, monitor);
          gdk_display_monitor_removed (display, GDK_MONITOR (monitor));
          g_object_unref (monitor);
          *changed = TRUE;
        }
    }

  old_primary = x11_display->primary_monitor;
  x11_display->primary_monitor = 0;
  primary_output = XRRGetOutputPrimary (x11_screen->xdisplay,
                                        x11_screen->xroot_window);

  for (i = 0; i < x11_display->monitors->len; ++i)
    {
      GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->output == primary_output)
        {
          x11_display->primary_monitor = i;
          break;
        }

      /* No RandR1.3+ available or no primary set, fall back to prefer LVDS as primary if present */
      if (primary_output == None &&
          g_ascii_strncasecmp (gdk_monitor_get_model (GDK_MONITOR (monitor)), "LVDS", 4) == 0)
        {
          x11_display->primary_monitor = i;
          break;
        }

      /* No primary specified and no LVDS found */
      if (monitor->output == first_output)
        x11_display->primary_monitor = i;
    }

  if (x11_display->primary_monitor != old_primary)
    *changed = TRUE;

  return x11_display->monitors->len > 0;
#endif

  return FALSE;
}

static _GdkX11Monitor *
find_monitor_by_output (_GdkX11Display *x11_display, XID output)
{
  int i;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      _GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->output == output)
        return monitor;
    }

  return NULL;
}


void
gdk_monitor_set_position (_GdkMonitor *monitor,
                          int         x,
                          int         y)
{
  g_object_freeze_notify (G_OBJECT (monitor));

  if (monitor->geometry.x != x)
    {
      monitor->geometry.x = x;
      g_object_notify (G_OBJECT (monitor), "geometry");
    }

  if (monitor->geometry.y != y)
    {
      monitor->geometry.y = y;
      g_object_notify (G_OBJECT (monitor), "geometry");
    }

  g_object_thaw_notify (G_OBJECT (monitor));
}

void
gdk_monitor_set_size (_GdkMonitor *monitor,
                      int         width,
                      int         height)
{
  g_object_freeze_notify (G_OBJECT (monitor));

  if (monitor->geometry.width != width)
    {
      monitor->geometry.width = width;
      g_object_notify (G_OBJECT (monitor), "geometry");
    }

  if (monitor->geometry.height != height)
    {
      monitor->geometry.height = height;
      g_object_notify (G_OBJECT (monitor), "geometry");
    }

  g_object_thaw_notify (G_OBJECT (monitor));
}

static gint
gdk_x11_screen_get_width_mm (_GdkScreen *screen)
{
  _GdkX11Screen* xs = GDK_X11_SCREEN (screen);
  return WidthMMOfScreen (xs->xscreen);
}

static gint
gdk_x11_screen_get_height_mm (_GdkScreen *screen)
{
  _GdkX11Screen* xs = GDK_X11_SCREEN (screen);
  return HeightMMOfScreen (xs->xscreen);
}

void
gdk_monitor_set_physical_size (_GdkMonitor *monitor,
                               int         width_mm,
                               int         height_mm)
{
  g_object_freeze_notify (G_OBJECT (monitor));

  if (monitor->width_mm != width_mm)
    {
      monitor->width_mm = width_mm;
      g_object_notify (G_OBJECT (monitor), "width-mm");
    }

  if (monitor->height_mm != height_mm)
    {
      monitor->height_mm = height_mm;
      g_object_notify (G_OBJECT (monitor), "height-mm");
    }

  g_object_thaw_notify (G_OBJECT (monitor));
}

void
gdk_monitor_set_scale_factor (_GdkMonitor *monitor,
                              int         scale_factor)
{
  if (monitor->scale_factor == scale_factor)
    return;

  monitor->scale_factor = scale_factor;

  g_object_notify (G_OBJECT (monitor), "scale-factor");
}

void
gdk_display_monitor_added (_GdkDisplay *display,
                           _GdkMonitor *monitor)
{
  g_signal_emit (display, signals[MONITOR_ADDED], 0, monitor);
}

void
gdk_monitor_invalidate (_GdkMonitor *monitor)
{
  g_signal_emit (monitor, msignals[INVALIDATE], 0);
}

void
gdk_display_monitor_removed (_GdkDisplay *display,
                             _GdkMonitor *monitor)
{
  g_signal_emit (display, signals[MONITOR_REMOVED], 0, monitor);
  gdk_monitor_invalidate (monitor);
}

static void
init_no_multihead (_GdkScreen *screen, gboolean *changed)
{
  _GdkDisplay *display = gdk_screen_get_display (screen);
  _GdkX11Display *x11_display = GDK_X11_DISPLAY (display);
  _GdkX11Screen *x11_screen = GDK_X11_SCREEN (screen);
  _GdkX11Monitor *monitor;
  GdkRectangle geometry;
  GdkRectangle newgeo;
  int i;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      _GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      monitor->add = FALSE;
      monitor->remove = TRUE;
    }

  monitor = find_monitor_by_output (x11_display, 0);
  if (monitor)
    monitor->remove = FALSE;
  else
    {
      monitor = g_object_new (gdk_x11_monitor_get_type (),
                              "display", x11_display,
                              NULL);
      monitor->output = 0;
      monitor->add = TRUE;
      g_ptr_array_add (x11_display->monitors, monitor);
    }

  gdk_monitor_get_geometry (GDK_MONITOR (monitor), &geometry);

  newgeo.x = 0;
  newgeo.y = 0;
  newgeo.width = DisplayWidth (x11_display->xdisplay, x11_screen->screen_num) /
                               x11_screen->window_scale;
  newgeo.height = DisplayHeight (x11_display->xdisplay, x11_screen->screen_num) /
                                 x11_screen->window_scale;

  if (newgeo.x != geometry.x ||
      newgeo.y != geometry.y ||
      newgeo.width != geometry.width ||
      newgeo.height != geometry.height ||
      gdk_x11_screen_get_width_mm (screen) != gdk_monitor_get_width_mm (GDK_MONITOR (monitor)) ||
      gdk_x11_screen_get_height_mm (screen) != gdk_monitor_get_height_mm (GDK_MONITOR (monitor)))
    *changed = TRUE;

  gdk_monitor_set_position (GDK_MONITOR (monitor), newgeo.x, newgeo.y);
  gdk_monitor_set_size (GDK_MONITOR (monitor), newgeo.width, newgeo.height);

  g_object_notify (G_OBJECT (monitor), "workarea");
  gdk_monitor_set_physical_size (GDK_MONITOR (monitor),
                                 gdk_x11_screen_get_width_mm (screen),
                                 gdk_x11_screen_get_height_mm (screen));
  gdk_monitor_set_scale_factor (GDK_MONITOR (monitor), x11_screen->window_scale);

  if (x11_display->primary_monitor != 0)
    *changed = TRUE;
  x11_display->primary_monitor = 0;

  for (i = x11_display->monitors->len - 1; i >= 0; i--)
    {
      _GdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->add)
        {
          gdk_display_monitor_added (GDK_DISPLAY (x11_display), GDK_MONITOR (monitor));
          *changed = TRUE;
        }
      else if (monitor->remove)
        {
          g_object_ref (monitor);
          g_ptr_array_remove (x11_display->monitors, monitor);
          gdk_display_monitor_removed (GDK_DISPLAY (x11_display), GDK_MONITOR (monitor));
          g_object_unref (monitor);
          *changed = TRUE;
        }
    }
}

static gboolean
init_multihead (GdkScreen *screen)
{
  gboolean any_changed = FALSE;

  if (!init_randr15 (screen, &any_changed) &&
      !init_randr13 (screen, &any_changed))
    init_no_multihead (screen, &any_changed);

  return any_changed;
}

GdkScreen* glgdk_x11_screen_new (_GdkDisplay *display, gint screen_number)  {
  _GdkScreen *screen;
  _GdkX11Screen *x11_screen;
  _GdkX11Display *display_x11 = GDK_X11_DISPLAY (display);
  const char *scale_str;

  screen = g_object_new (GDK_TYPE_X11_SCREEN, NULL);

  x11_screen = GDK_X11_SCREEN (screen);
  x11_screen->display = display;
  x11_screen->xdisplay = display_x11->xdisplay;
  x11_screen->xscreen = ScreenOfDisplay (display_x11->xdisplay, screen_number);
  x11_screen->screen_num = screen_number;
  x11_screen->xroot_window = RootWindow (display_x11->xdisplay,screen_number);
  x11_screen->wmspec_check_window = None;
  /* we want this to be always non-null */
  x11_screen->window_manager_name = g_strdup ("unknown");

  scale_str = g_getenv ("GDK_SCALE");
  if (scale_str) {
      x11_screen->fixed_window_scale = TRUE;
      x11_screen->window_scale = atol (scale_str);
      if (x11_screen->window_scale == 0)
        x11_screen->window_scale = 1;
    }
  else
    x11_screen->window_scale = 1;

  init_randr_support (screen);
  init_multihead (screen);

  glgdk_x11_screen_init_visuals (screen);
#ifdef NOPE
  _gdk_x11_screen_init_root_window (screen);
  update_bounding_box (screen);
#endif

  return screen;
}

static void glgdk_internal_connection_watch (Display  *display, XPointer  arg,
					   gint      fd, gboolean  opening, XPointer *watch_data);

#define N_PREDEFINED_ATOMS 69
#define ATOM_TO_INDEX(atom) (GPOINTER_TO_UINT(atom))
#define INDEX_TO_ATOM(atom) ((GdkAtom)GUINT_TO_POINTER(atom))

static Atom
lookup_cached_xatom (_GdkDisplay *display,
		     GdkAtom     atom)
{
  _GdkX11Display *display_x11 = GDK_X11_DISPLAY (display);

  if (ATOM_TO_INDEX (atom) < N_PREDEFINED_ATOMS)
    return ATOM_TO_INDEX (atom);

  if (display_x11->atom_from_virtual)
    return GPOINTER_TO_UINT (g_hash_table_lookup (display_x11->atom_from_virtual,
						  GDK_ATOM_TO_POINTER (atom)));

  return None;
}

static void
insert_atom_pair (_GdkDisplay *display,
		  GdkAtom     virtual_atom,
		  Atom        xatom)
{
  _GdkX11Display *display_x11 = GDK_X11_DISPLAY (display);

  if (!display_x11->atom_from_virtual)
    {
      display_x11->atom_from_virtual = g_hash_table_new (g_direct_hash, NULL);
      display_x11->atom_to_virtual = g_hash_table_new (g_direct_hash, NULL);
    }

  g_hash_table_insert (display_x11->atom_from_virtual,
		       GDK_ATOM_TO_POINTER (virtual_atom),
		       GUINT_TO_POINTER (xatom));
  g_hash_table_insert (display_x11->atom_to_virtual,
		       GUINT_TO_POINTER (xatom),
		       GDK_ATOM_TO_POINTER (virtual_atom));
}

void
_gdk_x11_precache_atoms (GdkDisplay          *display,
			 const gchar * const *atom_names,
			 gint                 n_atoms)
{
  Atom *xatoms;
  GdkAtom *atoms;
  const gchar **xatom_names;
  gint n_xatoms;
  gint i;

  xatoms = g_new (Atom, n_atoms);
  xatom_names = g_new (const gchar *, n_atoms);
  atoms = g_new (GdkAtom, n_atoms);

  n_xatoms = 0;
  for (i = 0; i < n_atoms; i++)
    {
      GdkAtom atom = gdk_atom_intern_static_string (atom_names[i]);
      if (lookup_cached_xatom (display, atom) == None)
	{
	  atoms[n_xatoms] = atom;
	  xatom_names[n_xatoms] = atom_names[i];
	  n_xatoms++;
	  printf("_gdk_x11_precache_atoms %i %s\n", i, atom_names[i]);
	}
    }

  if (n_xatoms)
    XInternAtoms (GDK_DISPLAY_XDISPLAY (display),
                  (char **)xatom_names, n_xatoms, False, xatoms);

  for (i = 0; i < n_xatoms; i++)
    insert_atom_pair (display, atoms[i], xatoms[i]);

  g_free (xatoms);
  g_free (xatom_names);
  g_free (atoms);
}

GdkDisplay* glgdk_x11_display_open (const gchar *display_name) {
  Display *xdisplay;
  _GdkDisplay *display;
  _GdkX11Display *display_x11;
  GdkWindowAttr attr;
  gint argc;
  gchar *argv[1];

  XClassHint *class_hint;
  gulong pid;
  gint ignore;
  gint maj, min;

  xdisplay = XOpenDisplay (display_name);
  if (!xdisplay) return NULL;

  display = g_object_new (GDK_TYPE_X11_DISPLAY, NULL);
  display_x11 = GDK_X11_DISPLAY (display);

  display_x11->xdisplay = xdisplay;

  /* Set up handlers for Xlib internal connections */
  XAddConnectionWatch (xdisplay, glgdk_internal_connection_watch, NULL);

  _gdk_x11_precache_atoms (display, precache_atoms, G_N_ELEMENTS (precache_atoms));

  /* RandR must be initialized before we initialize the screens */
  display_x11->have_randr12 = FALSE;
  display_x11->have_randr13 = FALSE;
  display_x11->have_randr15 = FALSE;
#ifdef HAVE_RANDR
  if (XRRQueryExtension (display_x11->xdisplay,
			 &display_x11->xrandr_event_base, &ignore))
  {
      int major, minor;

      XRRQueryVersion (display_x11->xdisplay, &major, &minor);

      if ((major == 1 && minor >= 2) || major > 1) {
	  display_x11->have_randr12 = TRUE;
	  if (minor >= 3 || major > 1)
	      display_x11->have_randr13 = TRUE;
#ifdef HAVE_RANDR15
	  if (minor >= 5 || major > 1)
	      display_x11->have_randr15 = TRUE;
#endif
      }

       gdk_x11_register_standard_event_type (display, display_x11->xrandr_event_base, RRNumberEvents);
  }
#endif

  /* initialize the display's screens */
  display_x11->screen = glgdk_x11_screen_new (display, DefaultScreen (display_x11->xdisplay));

#ifdef NOPE

  /* We need to initialize events after we have the screen
   * structures in places
   */
  _gdk_x11_xsettings_init (GDK_X11_SCREEN (display_x11->screen));

  display->device_manager = _gdk_x11_device_manager_new (display);

  gdk_event_init (display);

  attr.window_type = GDK_WINDOW_TOPLEVEL;
  attr.wclass = GDK_INPUT_ONLY;
  attr.x = 10;
  attr.y = 10;
  attr.width = 10;
  attr.height = 10;
  attr.event_mask = 0;

  display_x11->leader_gdk_window = gdk_window_new (GDK_X11_SCREEN (display_x11->screen)->root_window,
						   &attr, GDK_WA_X | GDK_WA_Y);
  (_gdk_x11_window_get_toplevel (display_x11->leader_gdk_window))->is_leader = TRUE;

  display_x11->leader_window = GDK_WINDOW_XID (display_x11->leader_gdk_window);

  display_x11->leader_window_title_set = FALSE;

#ifdef HAVE_XFIXES
  if (XFixesQueryExtension (display_x11->xdisplay,
			    &display_x11->xfixes_event_base,
			    &ignore))
    {
      display_x11->have_xfixes = TRUE;

      gdk_x11_register_standard_event_type (display,
					    display_x11->xfixes_event_base,
					    XFixesNumberEvents);
    }
  else
#endif
    display_x11->have_xfixes = FALSE;

#ifdef HAVE_XCOMPOSITE
  if (XCompositeQueryExtension (display_x11->xdisplay,
				&ignore, &ignore))
    {
      int major, minor;

      XCompositeQueryVersion (display_x11->xdisplay, &major, &minor);

      /* Prior to Composite version 0.4, composited windows clipped their
       * parents, so you had to use IncludeInferiors to draw to the parent
       * This isn't useful for our purposes, so require 0.4
       */
      display_x11->have_xcomposite = major > 0 || (major == 0 && minor >= 4);
    }
  else
#endif
    display_x11->have_xcomposite = FALSE;

#ifdef HAVE_XDAMAGE
  if (XDamageQueryExtension (display_x11->xdisplay,
			     &display_x11->xdamage_event_base,
			     &ignore))
    {
      display_x11->have_xdamage = TRUE;

      gdk_x11_register_standard_event_type (display,
					    display_x11->xdamage_event_base,
					    XDamageNumberEvents);
    }
  else
#endif
    display_x11->have_xdamage = FALSE;

  display_x11->have_shapes = FALSE;
  display_x11->have_input_shapes = FALSE;

  if (XShapeQueryExtension (GDK_DISPLAY_XDISPLAY (display), &display_x11->shape_event_base, &ignore))
    {
      display_x11->have_shapes = TRUE;
#ifdef ShapeInput
      if (XShapeQueryVersion (GDK_DISPLAY_XDISPLAY (display), &maj, &min))
	display_x11->have_input_shapes = (maj == 1 && min >= 1);
#endif
    }

  display_x11->trusted_client = TRUE;
  {
    Window root, child;
    int rootx, rooty, winx, winy;
    unsigned int xmask;

    gdk_x11_display_error_trap_push (display);
    XQueryPointer (display_x11->xdisplay,
		   GDK_X11_SCREEN (display_x11->screen)->xroot_window,
		   &root, &child, &rootx, &rooty, &winx, &winy, &xmask);
    if (G_UNLIKELY (gdk_x11_display_error_trap_pop (display) == BadWindow))
      {
	g_warning ("Connection to display %s appears to be untrusted. Pointer and keyboard grabs and inter-client communication may not work as expected.", gdk_display_get_name (display));
	display_x11->trusted_client = FALSE;
      }
  }

  if (g_getenv ("GDK_SYNCHRONIZE"))
    XSynchronize (display_x11->xdisplay, True);

  class_hint = XAllocClassHint();
  class_hint->res_name = (char *) g_get_prgname ();
  class_hint->res_class = (char *)gdk_get_program_class ();

  /* XmbSetWMProperties sets the RESOURCE_NAME environment variable
   * from argv[0], so we just synthesize an argument array here.
   */
  argc = 1;
  argv[0] = (char *) g_get_prgname ();

  XmbSetWMProperties (display_x11->xdisplay,
		      display_x11->leader_window,
		      NULL, NULL, argv, argc, NULL, NULL,
		      class_hint);
  XFree (class_hint);

  if (gdk_sm_client_id)
    set_sm_client_id (display, gdk_sm_client_id);

  pid = getpid ();
  XChangeProperty (display_x11->xdisplay,
		   display_x11->leader_window,
		   gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_PID"),
		   XA_CARDINAL, 32, PropModeReplace, (guchar *) & pid, 1);

  /* We don't yet know a valid time. */
  display_x11->user_time = 0;

#ifdef HAVE_XKB
  {
    gint xkb_major = XkbMajorVersion;
    gint xkb_minor = XkbMinorVersion;
    if (XkbLibraryVersion (&xkb_major, &xkb_minor))
      {
        xkb_major = XkbMajorVersion;
        xkb_minor = XkbMinorVersion;

        if (XkbQueryExtension (display_x11->xdisplay,
			       NULL, &display_x11->xkb_event_type, NULL,
                               &xkb_major, &xkb_minor))
          {
	    Bool detectable_autorepeat_supported;

	    display_x11->use_xkb = TRUE;

            XkbSelectEvents (display_x11->xdisplay,
                             XkbUseCoreKbd,
                             XkbNewKeyboardNotifyMask | XkbMapNotifyMask | XkbStateNotifyMask,
                             XkbNewKeyboardNotifyMask | XkbMapNotifyMask | XkbStateNotifyMask);

	    /* keep this in sync with _gdk_x11_keymap_state_changed() */
	    XkbSelectEventDetails (display_x11->xdisplay,
				   XkbUseCoreKbd, XkbStateNotify,
				   XkbAllStateComponentsMask,
                                   XkbModifierStateMask|XkbGroupStateMask);

	    XkbSetDetectableAutoRepeat (display_x11->xdisplay,
					True,
					&detectable_autorepeat_supported);

	    GDK_NOTE (MISC, g_message ("Detectable autorepeat %s.",
				       detectable_autorepeat_supported ?
				       "supported" : "not supported"));

	    display_x11->have_xkb_autorepeat = detectable_autorepeat_supported;
          }
      }
  }
#endif

  display_x11->use_sync = FALSE;
#ifdef HAVE_XSYNC
  {
    int major, minor;
    int error_base, event_base;

    if (XSyncQueryExtension (display_x11->xdisplay,
			     &event_base, &error_base) &&
        XSyncInitialize (display_x11->xdisplay,
                         &major, &minor))
      display_x11->use_sync = TRUE;
  }
#endif

  _gdk_x11_screen_setup (display_x11->screen);

  g_signal_emit_by_name (display, "opened");
#endif
  return display;
}

gboolean glgtk_init_check (int* argc, char*** argv) {
    gboolean ret;
    if (!gtk_parse_args (argc, argv)) return FALSE;

    const char* dname = gdk_get_display_arg_name();
    printf("glgtk_init_check, display name: %s\n", dname);
    ret = (glgdk_x11_display_open(dname) != NULL);
    return ret;
}

static gboolean
process_internal_connection (GIOChannel  *gioc,
			     GIOCondition cond,
			     gpointer     data)
{
  _GdkInternalConnection *connection = (_GdkInternalConnection *)data;

  gdk_threads_enter ();

  XProcessInternalConnection ((Display*)connection->display, connection->fd);

  gdk_threads_leave ();

  return TRUE;
}

static _GdkInternalConnection* gdk_add_connection_handler (Display *display, guint    fd) {
  GIOChannel *io_channel;
  _GdkInternalConnection *connection;

  connection = g_new (_GdkInternalConnection, 1);

  connection->fd = fd;
  connection->display = display;

  io_channel = g_io_channel_unix_new (fd);

  connection->source = g_io_create_watch (io_channel, G_IO_IN);
  g_source_set_callback (connection->source, (GSourceFunc)process_internal_connection, connection, NULL);
  g_source_attach (connection->source, NULL);

  g_io_channel_unref (io_channel);

  return connection;
}

static void gdk_remove_connection_handler (_GdkInternalConnection *connection) {
  g_source_destroy (connection->source);
  g_free (connection);
}

static void
glgdk_internal_connection_watch (Display  *display,
			       XPointer  arg,
			       gint      fd,
			       gboolean  opening,
			       XPointer *watch_data)
{
  if (opening)
    *watch_data = (XPointer)gdk_add_connection_handler (display, fd);
  else
    gdk_remove_connection_handler ((_GdkInternalConnection *)*watch_data);
}
