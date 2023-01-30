#ifndef GLAREALINUX_H_INCLUDED
#define GLAREALINUX_H_INCLUDED

#include <gdk/x11/gdkx11display.h>
#include <gdk/x11/gdkx11screen.h>
#include <gdk/x11/gdkx11visual.h>
#include <gdk/x11/gdkx11glcontext.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>

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

    if (invalidate_all || *getGlobalInvalidate()) {
        window_rect.x = 0;
        window_rect.y = 0;
        window_rect.width = gdk_window_get_width ((GdkWindow*)window);
        window_rect.height = gdk_window_get_height ((GdkWindow*)window);

        /* If nothing else is known, repaint everything so that the back
         buffer is fully up-to-date for the swapbuffer */
        cairo_region_union_rectangle (update_area, &window_rect);
        *getGlobalInvalidate() = FALSE;
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

gint epoxy_glx_version(Display*, int);
guint epoxy_has_glx_extension(Display*, int, const char*);

gboolean gdk_x11_screen_init_gl (_GdkScreen *screen) {
    GdkDisplay *display = gdk_screen_get_display (screen);
    _GdkX11Display *display_x11 = (_GdkX11Display*)display;
    int error_base, event_base;

    if (display_x11->have_glx) return TRUE;
    Display* dpy = display_x11->xdisplay;
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

static gboolean find_fbconfig_for_visual (GdkDisplay* display, _GdkVisual* visual, GLXFBConfig* fb_config_out, gboolean full, GError** error) {
    _GdkX11Display *display_x11 = (_GdkX11Display*)display;
    Display* dpy = display_x11->xdisplay;
    GLXFBConfig *configs;
    int n_configs, i;
    //gboolean use_rgba;
    gboolean retval = FALSE;

    _GdkX11Visual *visual_x11 = (_GdkX11Visual*)visual;
    VisualID xvisual_id = XVisualIDFromVisual(visual_x11->xvisual);

    static int attrs[] = { // those do not matter as we only want to find the config with ID xvisual_id
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_DOUBLEBUFFER    , GL_TRUE,
      GLX_RED_SIZE        , 1,
      GLX_GREEN_SIZE      , 1,
      GLX_BLUE_SIZE       , 1,
      GLX_ALPHA_SIZE      , GLX_DONT_CARE,
      None
    };
    configs = glXChooseFBConfig (dpy, DefaultScreen (dpy), attrs, &n_configs);
    printf(" creating context with minimum specs, found %i configs\n", n_configs);

    if (configs == NULL || n_configs == 0) {
      printf("AAAAAAA, glXChooseFBConfig failed!\n");
      g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_UNSUPPORTED_FORMAT, "No available configurations for the given pixel format");
      return FALSE;
    }

    for (i = 0; i < n_configs; i++)  {
      XVisualInfo *visinfo;

      visinfo = glXGetVisualFromFBConfig (dpy, configs[i]);
      if (visinfo == NULL) continue;


      //printf(" search visualid %i =? %i\n", visinfo->visualid, xvisual_id);
      if (visinfo->visualid != xvisual_id) {
          XFree (visinfo);
          continue;
      }

      if (fb_config_out != NULL) *fb_config_out = configs[i];

      XFree (visinfo);
      retval = TRUE;
      goto out;
    }

    printf("Error, find_fbconfig_for_visual failed? tried to find %lu\n", xvisual_id);
    //g_set_error (error, GDK_GL_ERROR, GDK_GL_ERROR_UNSUPPORTED_FORMAT, "No available configurations for the given RGBA pixel format");

out:
    XFree (configs);
    return retval;
}

GdkGLContext* x11_window_create_gl_context(GdkWindow* window, gboolean attached, GdkGLContext *share, gboolean full, GError**error) {
  GLXFBConfig config;

  GdkDisplay* display = gdk_window_get_display (window);

  if (!gdk_x11_screen_init_gl (gdk_window_get_screen (window))) {
      g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "No GL implementation is available");
      return NULL;
  }

  _GdkVisual* visual = (_GdkVisual*)gdk_window_get_visual ((GdkWindow*)window);
  if (!find_fbconfig_for_visual (display, visual, &config, full, error)) return NULL;
  printf("visual depth %i\n", visual->depth);

  _GdkX11GLContext* context = g_object_new (GDK_TYPE_X11_GL_CONTEXT,
                          "display", display,
                          "window", window,
                          "shared-context", share,
                          NULL);

  context->glx_config = config;
  context->is_attached = attached;

  return GDK_GL_CONTEXT (context);
}



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

gboolean gdk_x11_screen_init_gl(_GdkScreen*);



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


void glgdk_x11_screen_update_visuals_for_gl (_GdkScreen *screen) {
  printf("glgdk_x11_screen_update_visuals_for_gl\n");
  struct glvisualinfo* gl_info;

  if (!GDK_IS_X11_SCREEN(screen)) {
  	printf(" ..not a X11 screen, ignore..\n");
	return;
  }

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

#endif // GLAREALINUX_H_INCLUDED
