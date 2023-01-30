#ifndef GLAREALINUX_H_INCLUDED
#define GLAREALINUX_H_INCLUDED

#define GDK_COMPILATION

#include <gdk/wayland/gdkwaylandwindow.h>
#include <gdk/wayland/gdkwaylandglcontext.h>


#include <epoxy/egl.h>
#include <wayland-egl.h>

void initGLFunctions();

//#define GDK_IS_WAYLAND_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WAYLAND_WINDOW))

#define GDK_WAYLAND_MAX_THEME_SCALE 2
#define GDK_WAYLAND_THEME_SCALES_COUNT GDK_WAYLAND_MAX_THEME_SCALE

typedef struct {
  GObject parent_instance;
  EGLContext egl_context;
  EGLConfig egl_config;
  gboolean is_attached;
} _GdkWaylandGLContext;

typedef struct{
  GDestroyNotify destroy_notify;
  gpointer offer_data;
  GList *targets; /* List of GdkAtom */
} _DataOfferData;

typedef struct {
  _DataOfferData *offer;
  GHashTable *buffers; /* Hashtable of target_atom->SelectionBuffer */
} _SelectionData;

enum {
  ATOM_PRIMARY,
  ATOM_CLIPBOARD,
  ATOM_DND,
  N_ATOMS
};

typedef struct {
  GdkWindow *source;
  GCancellable *cancellable;
  guchar *data;
  gsize data_len;
  GdkAtom type;
  gint fd;
} _StoredSelection;

typedef struct {
  /* Destination-side data */
  _SelectionData selections[N_ATOMS];
  GHashTable *offers; /* Currently alive offers, Hashtable of wl_data_offer->DataOfferData */

  /* Source-side data */
  _StoredSelection stored_selection;
  GArray *source_targets;
  GdkAtom requested_target;

  struct gtk_primary_selection_source *primary_source;
  GdkWindow *primary_owner;

  struct wl_data_source *clipboard_source;
  GdkWindow *clipboard_owner;

  struct wl_data_source *dnd_source; /* Owned by the GdkDragContext */
  GdkWindow *dnd_owner;
} _GdkWaylandSelection;

typedef struct {
  _GdkDisplay parent_instance;
  GdkScreen *screen;

  /* Startup notification */
  gchar *startup_notification_id;

  /* Most recent serial */
  guint32 serial;

  /* Wayland fields below */
  struct wl_display *wl_display;
  struct wl_registry *wl_registry;
  struct wl_compositor *compositor;
  struct wl_shm *shm;
  struct zxdg_shell_v6 *xdg_shell;
  struct gtk_shell1 *gtk_shell;
  struct wl_input_device *input_device;
  struct wl_data_device_manager *data_device_manager;
  struct wl_subcompositor *subcompositor;
  struct zwp_pointer_gestures_v1 *pointer_gestures;
  struct gtk_primary_selection_device_manager *primary_selection_manager;
  struct zwp_tablet_manager_v2 *tablet_manager;
  struct zxdg_exporter_v1 *xdg_exporter;
  struct zxdg_importer_v1 *xdg_importer;
  struct zwp_keyboard_shortcuts_inhibit_manager_v1 *keyboard_shortcuts_inhibit;

  GList *async_roundtrips;

  /* Keep track of the ID's of the known globals and their corresponding
   * names. This way we can check whether an interface is known, and
   * remove globals given its ID. This table is not expected to be very
   * large, meaning the lookup by interface name time is insignificant. */
  GHashTable *known_globals;
  GList *on_has_globals_closures;

  /* Keep a list of orphaned dialogs (i.e. without parent) */
  GList *orphan_dialogs;

  GList *current_popups;

  struct wl_cursor_theme *scaled_cursor_themes[GDK_WAYLAND_THEME_SCALES_COUNT];
  gchar *cursor_theme_name;
  int cursor_theme_size;
  GHashTable *cursor_cache;

  GSource *event_source;

  int compositor_version;
  int seat_version;
  int data_device_manager_version;
  int gtk_shell_version;

  struct xkb_context *xkb_context;

  _GdkWaylandSelection *selection;

  GPtrArray *monitors;

  gint64 last_bell_time_ms;

  /* egl info */
  EGLDisplay egl_display;
  int egl_major_version;
  int egl_minor_version;

  guint have_egl : 1;
  guint have_egl_khr_create_context : 1;
  guint have_egl_buffer_age : 1;
  guint have_egl_swap_buffers_with_damage : 1;
  guint have_egl_surfaceless_context : 1;
  EGLint egl_min_swap_interval;
} _GdkWaylandDisplay;

typedef struct {
    GObject parent;
} _GdkWindowImpl;

typedef enum _PositionMethod {
  POSITION_METHOD_NONE,
  POSITION_METHOD_MOVE_RESIZE,
  POSITION_METHOD_MOVE_TO_RECT
} PositionMethod;

typedef struct {
    _GdkWindowImpl parent_instance;

  GdkWindow *wrapper;

  struct {
    /* The wl_outputs that this window currently touches */
    GSList               *outputs;

    struct wl_surface    *wl_surface;
    struct zxdg_surface_v6 *xdg_surface;
    struct zxdg_toplevel_v6 *xdg_toplevel;
    struct zxdg_popup_v6 *xdg_popup;
    struct gtk_surface1  *gtk_surface;
    struct wl_subsurface *wl_subsurface;
    struct wl_egl_window *egl_window;
    struct wl_egl_window *dummy_egl_window;
    struct zxdg_exported_v1 *xdg_exported;
  } display_server;

  EGLSurface egl_surface;
  EGLSurface dummy_egl_surface;

  unsigned int initial_configure_received : 1;
  unsigned int mapped : 1;
  unsigned int use_custom_surface : 1;
  unsigned int pending_buffer_attached : 1;
  unsigned int pending_commit : 1;
  unsigned int awaiting_frame : 1;
  GdkWindowTypeHint hint;
  GdkWindow *transient_for;
  GdkWindow *popup_parent;
  PositionMethod position_method;

  cairo_surface_t *staging_cairo_surface;
  cairo_surface_t *committed_cairo_surface;
  cairo_surface_t *backfill_cairo_surface;

  int pending_buffer_offset_x;
  int pending_buffer_offset_y;

  gchar *title;

  struct {
    gboolean was_set;

    gchar *application_id;
    gchar *app_menu_path;
    gchar *menubar_path;
    gchar *window_object_path;
    gchar *application_object_path;
    gchar *unique_bus_name;
  } application;

  GdkGeometry geometry_hints;
  GdkWindowHints geometry_mask;

  GdkSeat *grab_input_seat;

  gint64 pending_frame_counter;
  guint32 scale;

  int margin_left;
  int margin_right;
  int margin_top;
  int margin_bottom;
  gboolean margin_dirty;

  int initial_fullscreen_monitor;

  cairo_region_t *opaque_region;
  gboolean opaque_region_dirty;

  cairo_region_t *input_region;
  gboolean input_region_dirty;

  cairo_region_t *staged_updates_region;

  int saved_width;
  int saved_height;

  gulong parent_surface_committed_handler;

  struct {
    GdkRectangle rect;
    GdkGravity rect_anchor;
    GdkGravity window_anchor;
    GdkAnchorHints anchor_hints;
    gint rect_anchor_dx;
    gint rect_anchor_dy;
  } pending_move_to_rect;

  struct {
    int width;
    int height;
    GdkWindowState state;
  } pending;

  struct {
    char *handle;
    int export_count;
    GList *closures;
    guint idle_source_id;
  } exported;

  struct zxdg_imported_v1 *imported_transient_for;
  GHashTable *shortcuts_inhibitors;
} _GdkWindowImplWayland;

static struct wl_egl_window* gdk_wayland_window_get_wl_egl_window (_GdkWindow *window) {
    _GdkWindowImplWayland* impl = (_GdkWindowImplWayland*) (window->impl);

    if (impl->display_server.egl_window == NULL) {
        _GdkWindow* wrp = impl->wrapper;
        impl->display_server.egl_window = wl_egl_window_create (impl->display_server.wl_surface, wrp->width * impl->scale, wrp->height * impl->scale);
        wl_surface_set_buffer_scale (impl->display_server.wl_surface, impl->scale);
    }

    return impl->display_server.egl_window;
}

EGLSurface gdk_wayland_window_get_egl_surface (_GdkWindow *window, EGLConfig  config);

void _gdk_wayland_window_invalidate_for_new_frame (_GdkWindow* window, cairo_region_t* update_area);

GdkGLContext* gdk_wayland_window_create_gl_context (GdkWindow* window, gboolean attached, GdkGLContext* share, GError** error);


#endif // GLAREALINUX_H_INCLUDED
