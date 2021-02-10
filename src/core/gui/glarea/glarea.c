#define GTK_COMPILATION
#define GDK_COMPILATION

#ifdef _WIN32
#include <gdk/win32/gdkwin32glcontext.h>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <epoxy/wgl.h>
#else
#include <gdk/x11/gdkx11glcontext.h>
#endif

#include "glarea.h"
#include <gtk/gtkstylecontext.h>
#include <gtk/gtkrender.h>
#include <gobject/gtype.h>
#include <gobject/gvalue.h>
#include <gobject/gsignal.h>

gboolean global_invalidate = TRUE;

typedef enum {
    GDK_RENDERING_MODE_SIMILAR = 0,
    GDK_RENDERING_MODE_IMAGE,
    GDK_RENDERING_MODE_RECORDING
} GdkRenderingMode;

typedef struct {
    GObject parent_instance;

    void* impl; /* window-system-specific delegate object */  // GdkWindowImpl*

    GdkWindow* parent;
    GdkWindow* transient_for;
    GdkVisual* visual;

    gpointer user_data;

    gint x;
    gint y;

    GdkEventMask event_mask;
    guint8 window_type;

    guint8 depth;
    guint8 resize_count;

    gint8 toplevel_window_type;

    GList* filters;
    GList* children;
    GList children_list_node;
    GList* native_children;


    cairo_pattern_t* background;

    struct {
        cairo_surface_t* surface;

        cairo_region_t* region;
        cairo_region_t* flushed_region;
        cairo_region_t* need_blend_region;

        gboolean surface_needs_composite;
        gboolean use_gl;
    } current_paint;
    GdkGLContext* gl_paint_context;

    cairo_region_t* update_area;
    guint update_freeze_count;
    /* This is the update_area that was in effect when the current expose
       started. It may be smaller than the expose area if we'e painting
       more than we have to, but it represents the "true" damage. */
    cairo_region_t* active_update_area;
    /* We store the old expose areas to support buffer-age optimizations */
    cairo_region_t* old_updated_area[2];

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
    void* impl_window; // GdkWindow*

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
    cairo_region_t* clip_region;

    GdkCursor* cursor;
    GHashTable* device_cursor;

    cairo_region_t* shape;
    cairo_region_t* input_shape;

    GList* devices_inside;
    GHashTable* device_events;

    GHashTable* source_event_masks;
    gulong device_added_handler_id;
    gulong device_changed_handler_id;

    GdkFrameClock* frame_clock; /* NULL to use from parent or default */
    GdkWindowInvalidateHandlerFunc invalidate_handler;

    GdkDrawingContext* drawing_context;

    cairo_region_t* opaque_region;
} _GdkWindow;

typedef struct {
    GObjectClass parent_class;

    cairo_surface_t*
        (*ref_cairo_surface)    (GdkWindow* window);
    cairo_surface_t*
        (*create_similar_image_surface) (GdkWindow* window,
            cairo_format_t  format,
            int             width,
            int             height);

    void         (*show)                 (GdkWindow* window,
        gboolean         already_mapped);
    void         (*hide)                 (GdkWindow* window);
    void         (*withdraw)             (GdkWindow* window);
    void         (*raise)                (GdkWindow* window);
    void         (*lower)                (GdkWindow* window);
    void         (*restack_under)        (GdkWindow* window,
        GList* native_siblings);
    void         (*restack_toplevel)     (GdkWindow* window,
        GdkWindow* sibling,
        gboolean        above);

    void         (*move_resize)          (GdkWindow* window,
        gboolean         with_move,
        gint             x,
        gint             y,
        gint             width,
        gint             height);
    void         (*move_to_rect)         (GdkWindow* window,
        const GdkRectangle* rect,
        GdkGravity       rect_anchor,
        GdkGravity       window_anchor,
        GdkAnchorHints   anchor_hints,
        gint             rect_anchor_dx,
        gint             rect_anchor_dy);
    void         (*set_background)       (GdkWindow* window,
        cairo_pattern_t* pattern);

    GdkEventMask(*get_events)           (GdkWindow* window);
    void         (*set_events)           (GdkWindow* window,
        GdkEventMask     event_mask);

    gboolean(*reparent)             (GdkWindow* window,
        GdkWindow* new_parent,
        gint             x,
        gint             y);

    void         (*set_device_cursor)    (GdkWindow* window,
        GdkDevice* device,
        GdkCursor* cursor);

    void         (*get_geometry)         (GdkWindow* window,
        gint* x,
        gint* y,
        gint* width,
        gint* height);
    void         (*get_root_coords)      (GdkWindow* window,
        gint             x,
        gint             y,
        gint* root_x,
        gint* root_y);
    gboolean(*get_device_state)     (GdkWindow* window,
        GdkDevice* device,
        gdouble* x,
        gdouble* y,
        GdkModifierType* mask);
    gboolean(*begin_paint)           (GdkWindow* window);
    void        (*end_paint)             (GdkWindow* window);

    cairo_region_t* (*get_shape)        (GdkWindow* window);
    cairo_region_t* (*get_input_shape)  (GdkWindow* window);
    void         (*shape_combine_region) (GdkWindow* window,
        const cairo_region_t* shape_region,
        gint             offset_x,
        gint             offset_y);
    void         (*input_shape_combine_region) (GdkWindow* window,
        const cairo_region_t* shape_region,
        gint             offset_x,
        gint             offset_y);

    /* Called before processing updates for a window. This gives the windowing
     * layer a chance to save the region for later use in avoiding duplicate
     * exposes.
     */
    void     (*queue_antiexpose)     (GdkWindow* window,
        cairo_region_t* update_area);

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
    void         (*destroy)              (GdkWindow* window,
        gboolean         recursing,
        gboolean         foreign_destroy);


    /* Called when gdk_window_destroy() is called on a foreign window
     * or an ancestor of the foreign window. It should generally reparent
     * the window out of it's current heirarchy, hide it, and then
     * send a message to the owner requesting that the window be destroyed.
     */
    void         (*destroy_foreign)       (GdkWindow* window);

    /* optional */
    gboolean(*beep)                 (GdkWindow* window);

    void         (*focus)                (GdkWindow* window,
        guint32          timestamp);
    void         (*set_type_hint)        (GdkWindow* window,
        GdkWindowTypeHint hint);
    GdkWindowTypeHint(*get_type_hint)   (GdkWindow* window);
    void         (*set_modal_hint)       (GdkWindow* window,
        gboolean   modal);
    void         (*set_skip_taskbar_hint) (GdkWindow* window,
        gboolean   skips_taskbar);
    void         (*set_skip_pager_hint)  (GdkWindow* window,
        gboolean   skips_pager);
    void         (*set_urgency_hint)     (GdkWindow* window,
        gboolean   urgent);
    void         (*set_geometry_hints)   (GdkWindow* window,
        const GdkGeometry* geometry,
        GdkWindowHints     geom_mask);
    void         (*set_title)            (GdkWindow* window,
        const gchar* title);
    void         (*set_role)             (GdkWindow* window,
        const gchar* role);
    void         (*set_startup_id)       (GdkWindow* window,
        const gchar* startup_id);
    void         (*set_transient_for)    (GdkWindow* window,
        GdkWindow* parent);
    void         (*get_frame_extents)    (GdkWindow* window,
        GdkRectangle* rect);
    void         (*set_override_redirect) (GdkWindow* window,
        gboolean override_redirect);
    void         (*set_accept_focus)     (GdkWindow* window,
        gboolean accept_focus);
    void         (*set_focus_on_map)     (GdkWindow* window,
        gboolean focus_on_map);
    void         (*set_icon_list)        (GdkWindow* window,
        GList* pixbufs);
    void         (*set_icon_name)        (GdkWindow* window,
        const gchar* name);
    void         (*iconify)              (GdkWindow* window);
    void         (*deiconify)            (GdkWindow* window);
    void         (*stick)                (GdkWindow* window);
    void         (*unstick)              (GdkWindow* window);
    void         (*maximize)             (GdkWindow* window);
    void         (*unmaximize)           (GdkWindow* window);
    void         (*fullscreen)           (GdkWindow* window);
    void         (*fullscreen_on_monitor) (GdkWindow* window, gint monitor);
    void         (*apply_fullscreen_mode) (GdkWindow* window);
    void         (*unfullscreen)         (GdkWindow* window);
    void         (*set_keep_above)       (GdkWindow* window,
        gboolean   setting);
    void         (*set_keep_below)       (GdkWindow* window,
        gboolean   setting);
    GdkWindow* (*get_group)            (GdkWindow* window);
    void         (*set_group)            (GdkWindow* window,
        GdkWindow* leader);
    void         (*set_decorations)      (GdkWindow* window,
        GdkWMDecoration decorations);
    gboolean(*get_decorations)      (GdkWindow* window,
        GdkWMDecoration* decorations);
    void         (*set_functions)        (GdkWindow* window,
        GdkWMFunction functions);
    void         (*begin_resize_drag)    (GdkWindow* window,
        GdkWindowEdge  edge,
        GdkDevice* device,
        gint           button,
        gint           root_x,
        gint           root_y,
        guint32        timestamp);
    void         (*begin_move_drag)      (GdkWindow* window,
        GdkDevice* device,
        gint       button,
        gint       root_x,
        gint       root_y,
        guint32    timestamp);
    void         (*enable_synchronized_configure) (GdkWindow* window);
    void         (*configure_finished)   (GdkWindow* window);
    void         (*set_opacity)          (GdkWindow* window,
        gdouble    opacity);
    void         (*set_composited)       (GdkWindow* window,
        gboolean   composited);
    void         (*destroy_notify)       (GdkWindow* window);
    GdkDragProtocol(*get_drag_protocol) (GdkWindow* window,
        GdkWindow** target);
    void         (*register_dnd)         (GdkWindow* window);
    GdkDragContext* (*drag_begin)        (GdkWindow* window,
        GdkDevice* device,
        GList* targets,
        gint       x_root,
        gint       y_root);

    void         (*process_updates_recurse) (GdkWindow* window,
        cairo_region_t* region);

    void         (*sync_rendering)          (GdkWindow* window);
    gboolean(*simulate_key)            (GdkWindow* window,
        gint            x,
        gint            y,
        guint           keyval,
        GdkModifierType modifiers,
        GdkEventType    event_type);
    gboolean(*simulate_button)         (GdkWindow* window,
        gint            x,
        gint            y,
        guint           button,
        GdkModifierType modifiers,
        GdkEventType    event_type);

    gboolean(*get_property)            (GdkWindow* window,
        GdkAtom         property,
        GdkAtom         type,
        gulong          offset,
        gulong          length,
        gint            pdelete,
        GdkAtom* actual_type,
        gint* actual_format,
        gint* actual_length,
        guchar** data);
    void         (*change_property)         (GdkWindow* window,
        GdkAtom         property,
        GdkAtom         type,
        gint            format,
        GdkPropMode     mode,
        const guchar* data,
        gint            n_elements);
    void         (*delete_property)         (GdkWindow* window,
        GdkAtom         property);

    gint(*get_scale_factor)       (GdkWindow* window);
    void         (*get_unscaled_size)      (GdkWindow* window,
        int* unscaled_width,
        int* unscaled_height);

    void         (*set_opaque_region)      (GdkWindow* window,
        cairo_region_t* region);
    void         (*set_shadow_width)       (GdkWindow* window,
        gint            left,
        gint            right,
        gint            top,
        gint            bottom);
    gboolean(*show_window_menu)       (GdkWindow* window,
        GdkEvent* event);
    GdkGLContext* (*create_gl_context)      (GdkWindow* window,
        gboolean        attached,
        GdkGLContext* share,
        GError** error);
    gboolean(*realize_gl_context)     (GdkWindow* window,
        GdkGLContext* context,
        GError** error);
    void         (*invalidate_for_new_frame)(_GdkWindow* window,
        cairo_region_t* update_area);

    GdkDrawingContext* (*create_draw_context)  (GdkWindow* window,
        const cairo_region_t* region);
    void               (*destroy_draw_context) (GdkWindow* window,
        GdkDrawingContext* context);
} _GdkWindowImplClass;

typedef struct {
    GObject parent_instance;

    cairo_font_options_t* font_options;
    gdouble resolution; /* pixels/points scale factor for fonts */
    guint resolution_set : 1; /* resolution set through public API */
    guint closed : 1;
} _GdkScreen;

typedef struct {
    GObject parent_instance;

    GList* queued_events;
    GList* queued_tail;

    /* Information for determining if the latest button click
     * is part of a double-click or triple-click
     */
    GHashTable* multiple_click_info;

    guint event_pause_count;       /* How many times events are blocked */

    guint closed : 1;  /* Whether this display has been closed */

    GArray* touch_implicit_grabs;
    GHashTable* device_grabs;
    GHashTable* motion_hint_info;
    GdkDeviceManager* device_manager;
    GList* input_devices; /* Deprecated, only used to keep gdk_display_list_devices working */

    GHashTable* pointers_info;  /* GdkPointerWindowInfo for each device */
    guint32 last_event_time;    /* Last reported event time from server */

    guint double_click_time;  /* Maximum time between clicks in msecs */
    guint double_click_distance;   /* Maximum distance between clicks in pixels */

    guint has_gl_extension_texture_non_power_of_two : 1;
    guint has_gl_extension_texture_rectangle : 1;

    guint debug_updates : 1;
    guint debug_updates_set : 1;

    GdkRenderingMode rendering_mode;

    GList* seats;
} _GdkDisplay;

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

    GdkGLContextProgram* current_program;

    guint is_legacy : 1;
    guint use_es : 1;
} GdkGLContextPaintData;

typedef struct {
    GdkDisplay* display;
    GdkWindow* window;
    GdkGLContext* shared_context;

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

    GdkScreen* screen;
} _GdkVisual;

static inline GdkGLContextPrivate* gdk_gl_context_get_instance_private(GdkGLContext* context) {
    gpointer klass = g_type_class_ref(GDK_TYPE_GL_CONTEXT);
    gint privOffset = g_type_class_get_instance_private_offset(klass);
    return (G_STRUCT_MEMBER_P(context, privOffset));
}

_GdkWindowImplClass* getGdkWindowImplClass() {
#ifndef _WIN32
    GType t = g_type_from_name("GdkWindowImplX11");
#else
    GType t = g_type_from_name("GdkWindowImplWin32");
#endif
    return g_type_class_ref(t);
}

typedef struct {
    GObjectClass parent_class;

    gboolean(*realize) (GdkGLContext* context,
        GError** error);

    void (*end_frame)    (GdkGLContext* context,
        cairo_region_t* painted,
        cairo_region_t* damage);
    gboolean(*texture_from_surface) (GdkGLContext* context,
        cairo_surface_t* surface,
        cairo_region_t* region);
} _GdkGLContextClass;

typedef struct {
    _GdkGLContextClass parent_class;
} _GdkWin32GLContextClass;

#ifndef _WIN32
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

#else

typedef enum _GdkWin32ProcessDpiAwareness {
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} GdkWin32ProcessDpiAwareness;

typedef enum _GdkWin32MonitorDpiType
{
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} GdkWin32MonitorDpiType;

typedef HRESULT(WINAPI* funcSetProcessDpiAwareness) (GdkWin32ProcessDpiAwareness value);
typedef HRESULT(WINAPI* funcGetProcessDpiAwareness) (HANDLE                       handle,
    GdkWin32ProcessDpiAwareness* awareness);
typedef HRESULT(WINAPI* funcGetDpiForMonitor)       (HMONITOR                monitor,
    GdkWin32MonitorDpiType  dpi_type,
    UINT* dpi_x,
    UINT* dpi_y);

typedef struct _GdkWin32ShcoreFuncs
{
    HMODULE hshcore;
    funcSetProcessDpiAwareness setDpiAwareFunc;
    funcGetProcessDpiAwareness getDpiAwareFunc;
    funcGetDpiForMonitor getDpiForMonitorFunc;
} GdkWin32ShcoreFuncs;

typedef BOOL(WINAPI* funcSetProcessDPIAware) (void);
typedef BOOL(WINAPI* funcIsProcessDPIAware)  (void);

typedef struct _GdkWin32User32DPIFuncs
{
    funcSetProcessDPIAware setDpiAwareFunc;
    funcIsProcessDPIAware isDpiAwareFunc;
} GdkWin32User32DPIFuncs;

typedef struct Win32CursorTheme {
    GHashTable* named_cursors;
} _Win32CursorTheme;

typedef struct {
    _GdkDisplay display;

    GdkScreen* screen;

    _Win32CursorTheme* cursor_theme;
    gchar* cursor_theme_name;
    int cursor_theme_size;
    GHashTable* cursor_cache;

    HWND hwnd;
    HWND clipboard_hwnd;

    /* WGL/OpenGL Items */
    guint have_wgl : 1;
    guint gl_version;
    HDC gl_hdc;
    HWND gl_hwnd;

    GPtrArray* monitors;

    guint hasWglARBCreateContext : 1;
    guint hasWglEXTSwapControl : 1;
    guint hasWglOMLSyncControl : 1;
    guint hasWglARBPixelFormat : 1;
    guint hasWglARBmultisample : 1;

    /* HiDPI Items */
    guint have_at_least_win81 : 1;
    GdkWin32ProcessDpiAwareness dpi_aware_type;
    guint has_fixed_scale : 1;
    guint window_scale;

    GdkWin32ShcoreFuncs shcore_funcs;
    GdkWin32User32DPIFuncs user32_dpi_funcs;
} _GdkWin32Display;

typedef struct
{
    GObject parent_instance;

    /* WGL Context Items */
    HGLRC hglrc;
    HDC gl_hdc;
    guint need_alpha_bits : 1;

    /* other items */
    guint is_attached : 1;
    guint do_frame_sync : 1;
    guint do_blit_swap : 1;
} _GdkWin32GLContext;

typedef struct {
    ATOM wc_atom;
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
    gboolean inited;
} GdkWGLDummy;

typedef struct {
    gdouble x;
    gdouble y;
    gdouble width;
    gdouble height;
} _GdkRectangleDouble;

typedef enum {
    GDK_WIN32_AEROSNAP_STATE_UNDETERMINED = 0,
    GDK_WIN32_AEROSNAP_STATE_HALFLEFT,
    GDK_WIN32_AEROSNAP_STATE_HALFRIGHT,
    GDK_WIN32_AEROSNAP_STATE_FULLUP,
    /* Maximize state is only used by edge-snap */
    GDK_WIN32_AEROSNAP_STATE_MAXIMIZE
} _GdkWin32AeroSnapState;

typedef enum {
    GDK_WIN32_DRAGOP_NONE = 0,
    GDK_WIN32_DRAGOP_RESIZE,
    GDK_WIN32_DRAGOP_MOVE,
    GDK_WIN32_DRAGOP_COUNT
} _GdkW32WindowDragOp;

typedef struct {
    /* The window that is being moved/resized */
    GdkWindow* window;

    /* The kind of drag-operation going on. */
    _GdkW32WindowDragOp op;

    /* The edge that was grabbed for resizing. Not used for moving. */
    GdkWindowEdge      edge;

    /* The device used to initiate the op.
     * We grab it at the beginning and ungrab it at the end.
     */
    GdkDevice* device;

    /* The button pressed down to initiate the op.
     * The op will be canceled only when *this* button
     * is released.
     */
    gint               button;

    /* Initial cursor position when the operation began.
     * Current cursor position is subtracted from it to find how far
     * to move window border(s).
     */
    gint               start_root_x;
    gint               start_root_y;

    /* Initial window rectangle (position and size).
     * The window is resized/moved relative to this (see start_root_*).
     */
    RECT               start_rect;

    /* Not used */
    guint32            timestamp;

    /* TRUE if during the next redraw we should call SetWindowPos() to push
     * the window size and poistion to the native window.
     */
    gboolean           native_move_resize_pending;

    /* The cursor we should use while the operation is running. */
    GdkCursor* cursor;

    /* This window looks like an outline and is drawn under the window
     * that is being dragged. It indicates the shape the dragged window
     * will take if released at a particular point.
     * Indicator window size always matches the target indicator shape,
     * the the actual indicator drawn on it might not, depending on
     * how much time elapsed since the animation started.
     */
    HWND               shape_indicator;

    /* Used to draw the indicator */
    cairo_surface_t* indicator_surface;
    gint               indicator_surface_width;
    gint               indicator_surface_height;

    /* Size/position of shape_indicator */
    GdkRectangle       indicator_window_rect;

    /* Indicator will animate to occupy this rectangle */
    GdkRectangle       indicator_target;

    /* Indicator will start animating from this rectangle */
    GdkRectangle       indicator_start;

    /* Timestamp of the animation start */
    gint64             indicator_start_time;

    /* Timer that drives the animation */
    guint              timer;

    /* A special timestamp, if we want to draw not how
     * the animation should look *now*, but how it should
     * look at arbitrary moment of time.
     * Set to 0 to tell GDK to use current time.
     */
    gint64             draw_timestamp;

    /* Indicates that a transformation was revealed:
     *
     * For drag-resize: If it's FALSE,
     * then the pointer have not yet hit a trigger that triggers fullup.
     * If TRUE, then the pointer did hit a trigger that triggers fullup
     * at some point during this drag op.
     * This is used to prevent drag-resize from triggering
     * a transformation when first approaching a trigger of the work area -
     * one must drag it all the way to the very trigger to trigger; afterwards
     * a transformation will start triggering at some distance from the trigger
     * for as long as the op is still running. This is how AeroSnap works.
     *
     * For drag-move: If it's FALSE,
     * then the pointer have not yet hit a trigger, even if it is
     * already within a edge region.
     * If it's TRUE, then the pointer did hit a trigger within an
     * edge region, and have not yet left an edge region
     * (passing from one edge region into another doesn't count).
     */
    gboolean           revealed;

    /* Arrays of GdkRectangle pairs, describing the areas of the virtual
     * desktop that trigger various AeroSnap window transofrmations
     * Coordinates are GDK screen coordinates.
     */
    GArray* halfleft_regions;
    GArray* halfright_regions;
    GArray* maximize_regions;
    GArray* fullup_regions;

    /* Current pointer position will result in this kind of snapping,
     * if the drag op is finished.
     */
    _GdkWin32AeroSnapState current_snap;
} _GdkW32DragMoveResizeContext;

typedef struct {
    GObject parent;
} _GdkWindowImpl;

typedef struct {
    _GdkWindowImpl parent_instance;

    GdkWindow* wrapper;
    HANDLE handle;

    gint8 toplevel_window_type;

    GdkCursor* cursor;
    HICON   hicon_big;
    HICON   hicon_small;

    /* When VK_PACKET sends us a leading surrogate, it's stashed here.
     * Later, when another VK_PACKET sends a tailing surrogate, we make up
     * a full unicode character from them, or discard the leading surrogate,
     * if the next key is not a tailing surrogate.
     */
    wchar_t leading_surrogate_keydown;
    wchar_t leading_surrogate_keyup;

    /* Window size hints */
    gint hint_flags;
    GdkGeometry hints;

    GdkEventMask native_event_mask;

    GdkWindowTypeHint type_hint;

    GdkWindow* transient_owner;
    GSList* transient_children;
    gint       num_transients;
    gboolean   changing_state;

    gint initial_x;
    gint initial_y;

    /* left/right/top/bottom width of the shadow/resize-grip around the window */
    RECT margins;

    /* left+right and top+bottom from @margins */
    gint margins_x;
    gint margins_y;

    /* Set to TRUE when GTK tells us that margins are 0 everywhere.
     * We don't actually set margins to 0, we just set this bit.
     */
    guint zero_margins : 1;
    guint no_bg : 1;
    guint inhibit_configure : 1;
    guint override_redirect : 1;

    /* Set to TRUE if window is using true layered mode adjustments
     * via UpdateLayeredWindow().
     * Layered windows that get SetLayeredWindowAttributes() called
     * on them are not true layered windows.
     */
    guint layered : 1;

    /* If TRUE, the @temp_styles is set to the styles that were temporarily
     * added to this window.
     */
    guint have_temp_styles : 1;

    /* If TRUE, the window is in the process of being maximized.
     * This is set by WM_SYSCOMMAND and by gdk_win32_window_maximize (),
     * and is unset when WM_WINDOWPOSCHANGING is handled.
     */
    guint maximizing : 1;

    /* GDK does not keep window contents around, it just draws new
     * stuff over the window where changes occurred.
     * cache_surface retains old window contents, because
     * UpdateLayeredWindow() doesn't do partial redraws.
     */
    cairo_surface_t* cache_surface;
    cairo_surface_t* cairo_surface;

    /* Unlike window-backed surfaces, DIB-backed surface
     * does not provide a way to query its size,
     * so we have to remember it ourselves.
     */
    gint             dib_width;
    gint             dib_height;

    /* If the client wants uniformly-transparent window,
     * we remember the opacity value here and apply it
     * during UpdateLayredWindow() call, for layered windows.
     */
    gdouble          layered_opacity;

    HDC              hdc;
    int              hdc_count;
    HBITMAP          saved_dc_bitmap; /* Original bitmap for dc */

    _GdkW32DragMoveResizeContext drag_move_resize_context;

    /* Remembers where the window was snapped.
     * Some snap operations change their meaning if
     * the window is already snapped.
     */
    _GdkWin32AeroSnapState snap_state;

    /* Remembers window position before it was snapped.
     * This is used to unsnap it.
     * Position and size are percentages of the workarea
     * of the monitor on which the window was before it was snapped.
     */
    _GdkRectangleDouble* snap_stash;

    /* Also remember the same position, but in absolute form. */
    GdkRectangle* snap_stash_int;

    /* Decorations set by gdk_window_set_decorations() or NULL if unset */
    GdkWMDecoration* decorations;

    /* No. of windows to force layered windows off */
    guint suppress_layered;

    /* Temporary styles that this window got for the purpose of
     * handling WM_SYSMENU.
     * They are removed at the first opportunity (usually WM_INITMENU).
     */
    LONG_PTR temp_styles;

    /* scale of window on HiDPI */
    gint window_scale;
    gint unscaled_width;
    gint unscaled_height;
} _GdkWindowImplWin32;

static gboolean
_set_pixformat_for_hdc(HDC hdc, gint* best_idx, _GdkWin32Display* display) {
    PIXELFORMATDESCRIPTOR pfd;
    gboolean set_pixel_format_result = FALSE;

    /* one is only allowed to call SetPixelFormat(), and so ChoosePixelFormat()
     * one single time per window HDC
     */
    *best_idx = vr_get_wgl_pfd(hdc, &pfd, display); // vr_get_wgl_pfd
    if (*best_idx != 0) set_pixel_format_result = SetPixelFormat(hdc, *best_idx, &pfd);

    /* ChoosePixelFormat() or SetPixelFormat() failed, bail out */
    if (*best_idx == 0 || !set_pixel_format_result) return FALSE;
    return TRUE;
}

static HGLRC vr_create_gl_context_with_attribs(HDC hdc, HGLRC hglrc_base, int flags, int major, int minor) {
    HGLRC hglrc;
    _GdkWin32GLContext* context_win32;

    //major = 2;// 3;
    //minor = 1;// 0;

    printf("vr_create_gl_context_with_attribs %i %i\n", major, minor);

    int attribs[] = {
      WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
      WGL_CONTEXT_MAJOR_VERSION_ARB, major,
      WGL_CONTEXT_MINOR_VERSION_ARB, minor,
      WGL_CONTEXT_FLAGS_ARB,         flags,
      0
    };

    hglrc = wglCreateContextAttribsARB(hdc, NULL, attribs);
    return hglrc;
}

static gboolean _ensure_legacy_gl_context(HDC hdc, HGLRC hglrc_legacy, GdkGLContext* share) {
    _GdkWin32GLContext* context_win32;

    if (!wglMakeCurrent(hdc, hglrc_legacy)) return FALSE;

    if (share != NULL) {
        context_win32 = GDK_WIN32_GL_CONTEXT(share);
        return wglShareLists(hglrc_legacy, context_win32->hglrc);
    }

    return TRUE;
}

static HGLRC
_create_gl_context(HDC           hdc,
    GdkGLContext* share,
    int           flags,
    int           major,
    int           minor,
    gboolean* is_legacy,
    gboolean      hasWglARBCreateContext)
{
    /* We need a legacy context for *all* cases */
    HGLRC hglrc_base = wglCreateContext(hdc);
    gboolean success = TRUE;

    /* if we have no wglCreateContextAttribsARB(), return the legacy context when all is set */
    if (*is_legacy && !hasWglARBCreateContext)
    {
        if (_ensure_legacy_gl_context(hdc, hglrc_base, share)) return hglrc_base;
        success = FALSE;
        goto gl_fail;
    }
    else
    {
        HGLRC hglrc;

        if (!wglMakeCurrent(hdc, hglrc_base)) {
            success = FALSE;
            goto gl_fail;
        }

        hglrc = vr_create_gl_context_with_attribs(hdc, hglrc_base, flags, major, minor);

        /* return the legacy context we have if it could be setup properly, in case the 3.0+ context creation failed */
        if (hglrc == NULL)
        {
            if (!(*is_legacy))
            {
                /* If we aren't using a legacy context in the beginning, try again with a compatibility profile 3.0 context */
                hglrc = vr_create_gl_context_with_attribs(hdc, hglrc_base, flags, 3, 0);

                *is_legacy = TRUE;
            }

            if (hglrc == NULL)
            {
                if (!_ensure_legacy_gl_context(hdc, hglrc_base, share))
                    success = FALSE;
            }
        }

    gl_fail:
        if (!success || hglrc != NULL)
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(hglrc_base);
        }

        if (!success)
            return NULL;

        if (hglrc != NULL)
            return hglrc;

        return hglrc_base;
    }
}

#define GDK_WINDOW_DESTROYED(d) (((_GdkWindow*)(d))->destroyed)

#define GDK_WINDOW_TYPE(d) ((((_GdkWindow *)(d)))->window_type)

#define WINDOW_IS_TOPLEVEL(window)		   \
  (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD && \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_FOREIGN && \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_OFFSCREEN)

gboolean
_gdk_win32_window_lacks_wm_decorations(_GdkWindow* window)
{
    _GdkWindowImplWin32* impl;
    LONG style;
    gboolean has_any_decorations;

    if (GDK_WINDOW_DESTROYED(window))
        return FALSE;

    /* only toplevels can be layered */
    if (!WINDOW_IS_TOPLEVEL(window))
        return FALSE;

    impl = (_GdkWindowImplWin32*)(window->impl);

    /* This is because GTK calls gdk_window_set_decorations (window, 0),
     * even though GdkWMDecoration docs indicate that 0 does NOT mean
     * "no decorations".
     */
    if (impl->decorations &&
        *impl->decorations == 0)
        return TRUE;

    if (gdk_win32_window_get_handle(window) == 0)
        return FALSE;

    style = GetWindowLong(gdk_win32_window_get_handle(window), GWL_STYLE);

    if (style == 0)
    {
        DWORD w32_error = GetLastError();
        return FALSE;
    }

    /* Keep this in sync with _gdk_win32_window_update_style_bits() */
    /* We don't check what get_effective_window_decorations()
     * has to say, because it gives suggestions based on
     * various hints, while we want *actual* decorations,
     * or their absence.
     */
    has_any_decorations = FALSE;

    if (style & (WS_BORDER | WS_THICKFRAME | WS_CAPTION |
        WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX))
        has_any_decorations = TRUE;

    return !has_any_decorations;
}

static gboolean get_effective_window_decorations(_GdkWindow* window, GdkWMDecoration* decoration) {
    _GdkWindowImplWin32* impl;

    impl = (_GdkWindowImplWin32*)window->impl;

    if (gdk_window_get_decorations(window, decoration))
        return TRUE;

    if (window->window_type != GDK_WINDOW_TOPLEVEL)
    {
        return FALSE;
    }

    if ((impl->hint_flags & GDK_HINT_MIN_SIZE) &&
        (impl->hint_flags & GDK_HINT_MAX_SIZE) &&
        impl->hints.min_width == impl->hints.max_width &&
        impl->hints.min_height == impl->hints.max_height)
    {
        *decoration = GDK_DECOR_ALL | GDK_DECOR_RESIZEH | GDK_DECOR_MAXIMIZE;

        if (impl->type_hint == GDK_WINDOW_TYPE_HINT_DIALOG ||
            impl->type_hint == GDK_WINDOW_TYPE_HINT_MENU ||
            impl->type_hint == GDK_WINDOW_TYPE_HINT_TOOLBAR)
        {
            *decoration |= GDK_DECOR_MINIMIZE;
        }
        else if (impl->type_hint == GDK_WINDOW_TYPE_HINT_SPLASHSCREEN)
        {
            *decoration |= GDK_DECOR_MENU | GDK_DECOR_MINIMIZE;
        }

        return TRUE;
    }
    else if (impl->hint_flags & GDK_HINT_MAX_SIZE)
    {
        *decoration = GDK_DECOR_ALL | GDK_DECOR_MAXIMIZE;
        if (impl->type_hint == GDK_WINDOW_TYPE_HINT_DIALOG ||
            impl->type_hint == GDK_WINDOW_TYPE_HINT_MENU ||
            impl->type_hint == GDK_WINDOW_TYPE_HINT_TOOLBAR)
        {
            *decoration |= GDK_DECOR_MINIMIZE;
        }

        return TRUE;
    }
    else
    {
        switch (impl->type_hint)
        {
        case GDK_WINDOW_TYPE_HINT_DIALOG:
            *decoration = (GDK_DECOR_ALL | GDK_DECOR_MINIMIZE | GDK_DECOR_MAXIMIZE);
            return TRUE;

        case GDK_WINDOW_TYPE_HINT_MENU:
            *decoration = (GDK_DECOR_ALL | GDK_DECOR_RESIZEH | GDK_DECOR_MINIMIZE | GDK_DECOR_MAXIMIZE);
            return TRUE;

        case GDK_WINDOW_TYPE_HINT_TOOLBAR:
        case GDK_WINDOW_TYPE_HINT_UTILITY:
            gdk_window_set_skip_taskbar_hint(window, TRUE);
            gdk_window_set_skip_pager_hint(window, TRUE);
            *decoration = (GDK_DECOR_ALL | GDK_DECOR_MINIMIZE | GDK_DECOR_MAXIMIZE);
            return TRUE;

        case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
            *decoration = (GDK_DECOR_ALL | GDK_DECOR_RESIZEH | GDK_DECOR_MENU |
                GDK_DECOR_MINIMIZE | GDK_DECOR_MAXIMIZE);
            return TRUE;

        case GDK_WINDOW_TYPE_HINT_DOCK:
            return FALSE;

        case GDK_WINDOW_TYPE_HINT_DESKTOP:
            return FALSE;

        default:
            /* Fall thru */
        case GDK_WINDOW_TYPE_HINT_NORMAL:
            *decoration = GDK_DECOR_ALL;
            return TRUE;
        }
    }

    return FALSE;
}

static void update_single_bit(LONG* style, gboolean all, int gdk_bit, int style_bit) {
    /* all controls the interpretation of gdk_bit -- if all is TRUE,
     * gdk_bit indicates whether style_bit is off; if all is FALSE, gdk
     * bit indicate whether style_bit is on
     */
    if ((!all && gdk_bit) || (all && !gdk_bit)) *style |= style_bit;
    else *style &= ~style_bit;
}

#define SWP_NOZORDER_SPECIFIED HWND_TOP

void _gdk_win32_window_update_style_bits(_GdkWindow* window) {
    _GdkWindowImplWin32* impl = (_GdkWindowImplWin32*)window->impl;
    GdkWMDecoration decorations;
    LONG old_style, new_style, old_exstyle, new_exstyle;
    gboolean all;
    RECT rect, before, after;
    gboolean was_topmost;
    gboolean will_be_topmost;
    HWND insert_after;
    UINT flags;

    if (window->state & GDK_WINDOW_STATE_FULLSCREEN)
        return;

    old_style = GetWindowLong(gdk_win32_window_get_handle(window), GWL_STYLE);
    old_exstyle = GetWindowLong(gdk_win32_window_get_handle(window), GWL_EXSTYLE);

    GetClientRect(gdk_win32_window_get_handle(window), &before);
    after = before;
    AdjustWindowRectEx(&before, old_style, FALSE, old_exstyle);

    was_topmost = (old_exstyle & WS_EX_TOPMOST) ? TRUE : FALSE;
    will_be_topmost = was_topmost;

    old_exstyle &= ~WS_EX_TOPMOST;

    new_style = old_style;
    new_exstyle = old_exstyle;

    if (window->window_type == GDK_WINDOW_TEMP)
    {
        new_exstyle |= WS_EX_TOOLWINDOW;
        will_be_topmost = TRUE;
    }
    else if (impl->type_hint == GDK_WINDOW_TYPE_HINT_UTILITY)
    {
        new_exstyle |= WS_EX_TOOLWINDOW;
    }
    else
    {
        new_exstyle &= ~WS_EX_TOOLWINDOW;
    }

    /* We can get away with using layered windows
     * only when no decorations are needed. It can mean
     * CSD or borderless non-CSD windows (tooltips?).
     *
     * If this window cannot use layered windows, disable it always.
     * This currently applies to windows using OpenGL, which
     * does not work with layered windows.
     */
    if (impl->suppress_layered == 0)
    {
        if (_gdk_win32_window_lacks_wm_decorations(window))
            impl->layered = g_strcmp0(g_getenv("GDK_WIN32_LAYERED"), "0") != 0;
    }
    else
        impl->layered = FALSE;

    if (impl->layered)
        new_exstyle |= WS_EX_LAYERED;
    else
        new_exstyle &= ~WS_EX_LAYERED;

    if (get_effective_window_decorations(window, &decorations))
    {
        all = (decorations & GDK_DECOR_ALL);
        /* Keep this in sync with the test in _gdk_win32_window_lacks_wm_decorations() */
        update_single_bit(&new_style, all, decorations & GDK_DECOR_BORDER, WS_BORDER);
        update_single_bit(&new_style, all, decorations & GDK_DECOR_RESIZEH, WS_THICKFRAME);
        update_single_bit(&new_style, all, decorations & GDK_DECOR_TITLE, WS_CAPTION);
        update_single_bit(&new_style, all, decorations & GDK_DECOR_MENU, WS_SYSMENU);
        update_single_bit(&new_style, all, decorations & GDK_DECOR_MINIMIZE, WS_MINIMIZEBOX);
        update_single_bit(&new_style, all, decorations & GDK_DECOR_MAXIMIZE, WS_MAXIMIZEBOX);
    }

    if (old_style == new_style && old_exstyle == new_exstyle)
    {
        return;
    }

    if (old_style != new_style)
    {

        SetWindowLong(gdk_win32_window_get_handle(window), GWL_STYLE, new_style);
    }

    if (old_exstyle != new_exstyle)
    {
        SetWindowLong(gdk_win32_window_get_handle(window), GWL_EXSTYLE, new_exstyle);
    }

    AdjustWindowRectEx(&after, new_style, FALSE, new_exstyle);

    GetWindowRect(gdk_win32_window_get_handle(window), &rect);
    rect.left += after.left - before.left;
    rect.top += after.top - before.top;
    rect.right += after.right - before.right;
    rect.bottom += after.bottom - before.bottom;

    flags = SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOREPOSITION;

    if (will_be_topmost && !was_topmost)
    {
        insert_after = HWND_TOPMOST;
    }
    else if (was_topmost && !will_be_topmost)
    {
        insert_after = HWND_NOTOPMOST;
    }
    else
    {
        flags |= SWP_NOZORDER;
        insert_after = SWP_NOZORDER_SPECIFIED;
    }

    SetWindowPos(gdk_win32_window_get_handle(window), insert_after,
        rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top,
        flags);
}

void gdk_gl_context_set_is_legacy(GdkGLContext* context, gboolean is_legacy) {
    GdkGLContextPrivate* priv = gdk_gl_context_get_instance_private(context);
    priv->is_legacy = !!is_legacy;
}

gboolean vr_gdk_win32_gl_context_realize(GdkGLContext* context, GError** error) {
    GdkGLContext* share = gdk_gl_context_get_shared_context(context);
    _GdkWin32GLContext* context_win32 = GDK_WIN32_GL_CONTEXT(context);

    /* These are the real WGL context items that we will want to use later */
    HGLRC hglrc;
    gint pixel_format;
    gboolean debug_bit, compat_bit, legacy_bit;

    /* request flags and specific versions for core (3.2+) WGL context */
    gint flags = 0;
    gint glver_major = 0;
    gint glver_minor = 0;

    _GdkWindow* window = gdk_gl_context_get_window(context);
    _GdkWindowImplWin32* impl = (_GdkWindowImplWin32*)window->impl;
    _GdkWin32Display* win32_display = (_GdkWin32Display*)gdk_window_get_display(window);

    if (!_set_pixformat_for_hdc(context_win32->gl_hdc, &pixel_format, win32_display)) {
        g_set_error_literal(error, GDK_GL_ERROR, GDK_GL_ERROR_UNSUPPORTED_FORMAT, "No available configurations for the given pixel format");
        return FALSE;
    }

    gdk_gl_context_get_required_version(context, &glver_major, &glver_minor);
    debug_bit = gdk_gl_context_get_debug_enabled(context);
    compat_bit = gdk_gl_context_get_forward_compatible(context);

    /* if there isn't wglCreateContextAttribsARB(), or if GDK_GL_LEGACY is set, we default to a legacy context */
    legacy_bit = !win32_display->hasWglARBCreateContext || g_getenv("GDK_GL_LEGACY") != NULL;

    /*
     * A legacy context cannot be shared with core profile ones, so this means we
     * must stick to a legacy context if the shared context is a legacy context
     */
    if (share != NULL && gdk_gl_context_is_legacy(share)) legacy_bit = TRUE;

    if (debug_bit) flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
    if (compat_bit) flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

    printf(" - - - vr_gdk_win32_gl_context_realize %i %i %i %i %i\n", legacy_bit, debug_bit, compat_bit, glver_major, glver_minor);

    hglrc = _create_gl_context(context_win32->gl_hdc, share, flags, glver_major, glver_minor, &legacy_bit, win32_display->hasWglARBCreateContext);

    if (hglrc == NULL) {
        g_set_error_literal(error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "Unable to create a GL context");
        return FALSE;
    }

    context_win32->hglrc = hglrc;

    /* OpenGL does not work with WS_EX_LAYERED enabled, so we need to
     * disable WS_EX_LAYERED when we acquire a valid HGLRC
     */
    impl->suppress_layered++;

    /* if this is the first time a GL context is acquired for the window,
     * disable layered windows by triggering update_style_bits()
     */
    if (impl->suppress_layered == 1) _gdk_win32_window_update_style_bits(window);

    /* Ensure that any other context is created with a legacy bit set */
    gdk_gl_context_set_is_legacy(context, legacy_bit);
    return TRUE;
}

void disableBlur(_GdkWindow* window) {
    _GdkWindowImplWin32* impl = (_GdkWindowImplWin32*)(window->impl);
    HWND thiswindow = impl->handle;
    DWM_BLURBEHIND blur_behind;
    memset(&blur_behind, 0, sizeof(blur_behind));
    blur_behind.dwFlags = DWM_BB_ENABLE;
    blur_behind.fEnable = FALSE;
    DwmEnableBlurBehindWindow(thiswindow, &blur_behind);
}

void override_win32_gl_context_realize() {
    GType glc_type = gdk_win32_gl_context_get_type();
    _GdkGLContextClass* gklass = (_GdkGLContextClass*)g_type_class_ref(glc_type);
    gklass->realize = vr_gdk_win32_gl_context_realize;
    g_type_class_unref(gklass);
}

gboolean
gdk_gl_context_has_framebuffer_blit(GdkGLContext* context)
{
    GdkGLContextPrivate* priv = gdk_gl_context_get_instance_private(context);

    return priv->has_gl_framebuffer_blit;
}

void analyseCairoRegion(cairo_region_t* r, cairo_region_overlap_t winOverlap) {
    int N = cairo_region_num_rectangles(r);
    printf("analyse cairo region with %i rectangles, window overlap: %i (in, out, part)\n", N, winOverlap);
    cairo_rectangle_int_t rec;
    for (int i=0; i<N; i++) {
        cairo_region_get_rectangle(r, i, &rec);
        printf(" rectangle %i %i %i %i\n", rec.x, rec.y, rec.width, rec.height);
    }
}

void _gdk_win32_window_invalidate_for_new_frame(_GdkWindow* window, cairo_region_t* update_area) {
    if (window->gl_paint_context == NULL) return; // Minimal update is ok if we're not drawing with gl

    _GdkWin32GLContext* context_win32 = GDK_WIN32_GL_CONTEXT(window->gl_paint_context);
    context_win32->do_blit_swap = FALSE;

    //printf("gl needs alpha %i\n", context_win32->need_alpha_bits);

    cairo_rectangle_int_t whole_window = { 0, 0, gdk_window_get_width(window), gdk_window_get_height(window) };
    /*gboolean invalidate_all = FALSE;
    cairo_region_overlap_t overlap = cairo_region_contains_rectangle(update_area, &whole_window);
    if (overlap == CAIRO_REGION_OVERLAP_IN) invalidate_all = TRUE;*/

    //analyseCairoRegion(update_area, overlap);

    if (global_invalidate) { // when resizing
        cairo_region_union_rectangle(update_area, &whole_window);
        global_invalidate = FALSE;
    }

    static cairo_rectangle_int_t lastRegion = { 0, 0, 0, 0 }; // to avoid flickering just remeber and update last region too..
    cairo_rectangle_int_t extends;
    cairo_region_get_extents(update_area, &extends);
    cairo_region_union_rectangle(update_area, &lastRegion);
    lastRegion = extends;
}

/*static void _gdk_win32_window_set_opacity(_GdkWindow* window, gdouble opacity) {
    printf(" +-+-+-++-- _gdk_win32_window_set_opacity %d\n", opacity);

    LONG exstyle;
    typedef BOOL(WINAPI* PFN_SetLayeredWindowAttributes) (HWND, COLORREF, BYTE, DWORD);
    PFN_SetLayeredWindowAttributes setLayeredWindowAttributes = NULL;

    g_return_if_fail(GDK_IS_WINDOW(window));

    if (!WINDOW_IS_TOPLEVEL(window) || GDK_WINDOW_DESTROYED(window))
        return;

    if (opacity < 0)
        opacity = 0;
    else if (opacity > 1)
        opacity = 1;

    _GdkWindowImplWin32* impl = (_GdkWindowImplWin32*)window->impl;

    impl->layered_opacity = opacity;

    if (impl->layered) return;

    exstyle = GetWindowLong(gdk_win32_window_get_handle(window), GWL_EXSTYLE);

    if (!(exstyle & WS_EX_LAYERED)) SetWindowLong(gdk_win32_window_get_handle(window), GWL_EXSTYLE, exstyle | WS_EX_LAYERED);

    setLayeredWindowAttributes = (PFN_SetLayeredWindowAttributes)GetProcAddress(GetModuleHandle("user32.dll"), "SetLayeredWindowAttributes");

    if (setLayeredWindowAttributes) {
        setLayeredWindowAttributes(gdk_win32_window_get_handle(window),
            0,
            opacity * 0xff,
            LWA_ALPHA);
    }
}*/

#endif

void override_window_invalidate_for_new_frame(_GdkWindow* window) {
    _GdkWindowImplClass* impl_class = getGdkWindowImplClass();
#ifdef _WIN32
    impl_class->invalidate_for_new_frame = _gdk_win32_window_invalidate_for_new_frame;
    //impl_class->set_opacity = _gdk_win32_window_set_opacity;
#else
    impl_class->invalidate_for_new_frame = _gdk_x11_window_invalidate_for_new_frame;
#endif
    g_type_class_unref(impl_class);
}

typedef struct {
    GdkGLContext* context;
    GdkWindow *event_window;
    GError *error;

    GLClipping clipping;
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

#ifndef _WIN32
gint epoxy_glx_version(Display*, int);
guint epoxy_has_glx_extension(Display*, int, const char*);

gboolean gdk_x11_screen_init_gl (GdkScreen *screen) {
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

    printf("Error, find_fbconfig_for_visual failed? tried to find %i\n", xvisual_id);
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

#else

static void _get_dummy_window_hwnd(GdkWGLDummy* dummy) {
    WNDCLASSEX dummy_wc;

    memset(&dummy_wc, 0, sizeof(WNDCLASSEX));

    dummy_wc.cbSize = sizeof(WNDCLASSEX);
    dummy_wc.style = CS_OWNDC;
    dummy_wc.lpfnWndProc = (WNDPROC)DefWindowProc;
    dummy_wc.cbClsExtra = 0;
    dummy_wc.cbWndExtra = 0;
    dummy_wc.hInstance = GetModuleHandle(NULL);
    dummy_wc.hIcon = 0;
    dummy_wc.hCursor = NULL;
    dummy_wc.hbrBackground = 0;
    dummy_wc.lpszMenuName = 0;
    dummy_wc.lpszClassName = "dummy";
    dummy_wc.hIconSm = 0;

    dummy->wc_atom = RegisterClassEx(&dummy_wc);

    dummy->hwnd =
        CreateWindowEx(WS_EX_APPWINDOW,
            MAKEINTATOM(dummy->wc_atom),
            "",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0,
            0,
            0,
            0,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL);
}

static void _destroy_dummy_gl_context(GdkWGLDummy dummy) {
    if (dummy.hglrc != NULL) {
        if (wglGetCurrentContext() == dummy.hglrc) wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy.hglrc);
        dummy.hglrc = NULL;
    }
    if (dummy.hdc != NULL) {
        DeleteDC(dummy.hdc);
        dummy.hdc = NULL;
    }
    if (dummy.hwnd != NULL) {
        DestroyWindow(dummy.hwnd);
        dummy.hwnd = NULL;
    }
    if (dummy.wc_atom != 0) {
        UnregisterClass(MAKEINTATOM(dummy.wc_atom), GetModuleHandle(NULL));
        dummy.wc_atom = 0;
    }
    dummy.inited = FALSE;
}

#define PIXEL_ATTRIBUTES 19

static gint vr_get_dummy_wgl_pfd(HDC hdc, PIXELFORMATDESCRIPTOR* pfd) {
    printf("   get minimal GL Context\n");

    pfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->cColorBits = 32;// GetDeviceCaps(hdc, BITSPIXEL);
    pfd->cDepthBits = 24;
    pfd->cStencilBits = 8;
    pfd->dwLayerMask = PFD_MAIN_PLANE;
    gint best_pf = ChoosePixelFormat(hdc, pfd);

    printf("    chose minimal pixel format: %i\n", best_pf);
    return best_pf;
}

static gint vr_get_wgl_pfd(HDC hdc, PIXELFORMATDESCRIPTOR* pfd, _GdkWin32Display* display) {
    printf("   vr_get_wgl_pfd, get complete GL Context\n");
    gint best_pf = 0;

    pfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);

    if (display->hasWglARBPixelFormat) {
        GdkWGLDummy dummy;
        UINT num_formats;
        gint colorbits = GetDeviceCaps(hdc, BITSPIXEL);

        printf("     device colorbits: %i\n", colorbits);

        // acquire and cache dummy Window (HWND & HDC) and dummy GL Context, we need it for wglChoosePixelFormatARB()
        memset(&dummy, 0, sizeof(GdkWGLDummy));
        best_pf = _gdk_init_dummy_context(&dummy);
        if (best_pf == 0 || !wglMakeCurrent(dummy.hdc, dummy.hglrc)) return 0;

        int pixelAttribsFull[] = {
            WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
            WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,         32,
            //WGL_ALPHA_BITS_ARB,         8, // important, if set to 0 the window might get translucent on some systems
            WGL_DEPTH_BITS_ARB,         24,
            WGL_STENCIL_BITS_ARB,       8,
            WGL_SAMPLE_BUFFERS_ARB,     1,
            WGL_SAMPLES_ARB,            8,
            0
        };

        printf(" --- === --- pixelAttribsFull %i %i\n", colorbits, 0);

        int pixelAttribsMin[] = {
            WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
            WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,         32,
            //WGL_ALPHA_BITS_ARB,         8, // important, if set to 0 the window might get translucent on some systems
            WGL_DEPTH_BITS_ARB,         24,
            WGL_STENCIL_BITS_ARB,       8,
            0
        };

        bool validPF = wglChoosePixelFormatARB(hdc, pixelAttribsFull, NULL, 1, &best_pf, &num_formats);
        if (!validPF) {
            printf(" --= WARNING! full pixel attribs failed, trying without multisampling =--\n");
            validPF = wglChoosePixelFormatARB(hdc, pixelAttribsMin, NULL, 1, &best_pf, &num_formats);
        }

        if (!validPF) {
            printf(" --= WARNING! no working pixel attribs, get fallback GL Context! ..good luck! =--\n");
            best_pf = vr_get_dummy_wgl_pfd(hdc, pfd);
        }

        wglMakeCurrent(NULL, NULL);
        _destroy_dummy_gl_context(dummy);
    } else {
        printf(" --= WARNING! on ARB, get fallback GL Context! ..good luck! =--\n");
        best_pf = vr_get_dummy_wgl_pfd(hdc, pfd);
    }

    printf("   chose pixel format: %i\n", best_pf);
    return best_pf;
}

static gint _gdk_init_dummy_context(GdkWGLDummy* dummy) {
    PIXELFORMATDESCRIPTOR pfd;
    gboolean set_pixel_format_result = FALSE;
    gint best_idx = 0;

    _get_dummy_window_hwnd(dummy);

    dummy->hdc = GetDC(dummy->hwnd);
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    best_idx = vr_get_dummy_wgl_pfd(dummy->hdc, &pfd);

    if (best_idx != 0) set_pixel_format_result = SetPixelFormat(dummy->hdc, best_idx, &pfd);
    if (best_idx == 0 || !set_pixel_format_result) return 0;

    dummy->hglrc = wglCreateContext(dummy->hdc);
    if (dummy->hglrc == NULL) return 0;

    dummy->inited = TRUE;

    return best_idx;
}

gboolean _gdk_win32_display_init_gl(GdkDisplay* display) {
    _GdkWin32Display* display_win32 = (_GdkWin32Display*)display;
    gint best_idx = 0;
    GdkWGLDummy dummy;

    if (display_win32->have_wgl) return TRUE;

    memset(&dummy, 0, sizeof(GdkWGLDummy));

    /* acquire and cache dummy Window (HWND & HDC) and
     * dummy GL Context, it is used to query functions
     * and used for other stuff as well
     */
    best_idx = _gdk_init_dummy_context(&dummy);

    if (best_idx == 0 || !wglMakeCurrent(dummy.hdc, dummy.hglrc)) return FALSE;

    display_win32->have_wgl = TRUE;
    display_win32->gl_version = epoxy_gl_version();

    display_win32->hasWglARBCreateContext = epoxy_has_wgl_extension(dummy.hdc, "WGL_ARB_create_context");
    display_win32->hasWglEXTSwapControl = epoxy_has_wgl_extension(dummy.hdc, "WGL_EXT_swap_control");
    display_win32->hasWglOMLSyncControl = epoxy_has_wgl_extension(dummy.hdc, "WGL_OML_sync_control");
    display_win32->hasWglARBPixelFormat = epoxy_has_wgl_extension(dummy.hdc, "WGL_ARB_pixel_format");
    display_win32->hasWglARBmultisample = epoxy_has_wgl_extension(dummy.hdc, "WGL_ARB_multisample");

    wglMakeCurrent(NULL, NULL);
    _destroy_dummy_gl_context(dummy);

    return TRUE;
}

GdkGLContext* win32_window_create_gl_context(GdkWindow* window, gboolean attached, GdkGLContext* share, gboolean full, GError** error) {
    GdkDisplay* display = gdk_window_get_display(window);
    _GdkWin32Display* display_win32 = (_GdkWin32Display*)display;
    _GdkWin32GLContext* context = NULL;
    GdkVisual* visual = gdk_window_get_visual(window);

    if (!_gdk_win32_display_init_gl(display)) {
        g_set_error_literal(error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "No GL implementation is available");
        return NULL;
    }

    HWND hwnd = gdk_win32_window_get_handle(window);
    HDC hdc   = GetDC(hwnd);

    display_win32->gl_hdc = hdc;
    display_win32->gl_hwnd = hwnd;

    context = g_object_new(GDK_TYPE_WIN32_GL_CONTEXT, "display", display, "window", window, "shared-context", share, NULL);
    context->gl_hdc = hdc;
    context->is_attached = attached;

    return GDK_GL_CONTEXT(context);
}


/*static void
gdk_window_begin_paint_internal(_GdkWindow* window, const cairo_region_t* region) {
    GdkRectangle clip_box;
    double sx, sy;
    gboolean needs_surface;
    cairo_content_t surface_content;

    if (GDK_WINDOW_DESTROYED(window) ||
        !gdk_window_has_impl(window))
        return;

    if (window->current_paint.surface != NULL)
    {
        g_warning("A paint operation on the window is alredy in progress. "
            "This is not allowed.");
        return;
    }

    _GdkWindowImplClass* impl_class = getGdkWindowImplClass();

    needs_surface = TRUE;
    if (impl_class->begin_paint)
        needs_surface = impl_class->begin_paint(window);

    window->current_paint.region = cairo_region_copy(region);
    cairo_region_intersect(window->current_paint.region, window->clip_region);
    cairo_region_get_extents(window->current_paint.region, &clip_box);

    window->current_paint.flushed_region = cairo_region_create();
    window->current_paint.need_blend_region = cairo_region_create();

    surface_content = gdk_window_get_content(window);

    _GdkWindow* iwindow = (_GdkWindow*)window->impl_window;
    window->current_paint.use_gl = iwindow->gl_paint_context != NULL;

    if (window->current_paint.use_gl)
    {
        GdkGLContext* context;

        int ww = gdk_window_get_width(window) * gdk_window_get_scale_factor(window);
        int wh = gdk_window_get_height(window) * gdk_window_get_scale_factor(window);

        context = gdk_window_get_paint_gl_context(window, NULL);
        if (context == NULL)
        {
            g_warning("gl rendering failed, context: %p", context);
            window->current_paint.use_gl = FALSE;
        }
        else
        {
            gdk_gl_context_make_current(context);
            // With gl we always need a surface to combine the gl drawing with the native drawing.
            needs_surface = TRUE;
            // Also, we need the surface to include alpha
            surface_content = CAIRO_CONTENT_COLOR_ALPHA;

            // Initial setup
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            glViewport(0, 0, ww, wh);
        }
    }

    if (needs_surface)
    {
        window->current_paint.surface = gdk_window_create_similar_surface(window,
            surface_content,
            MAX(clip_box.width, 1),
            MAX(clip_box.height, 1));
        sx = sy = 1;
        cairo_surface_get_device_scale(window->current_paint.surface, &sx, &sy);
        cairo_surface_set_device_offset(window->current_paint.surface, -clip_box.x * sx, -clip_box.y * sy);
        gdk_cairo_surface_mark_as_direct(window->current_paint.surface, window);

        window->current_paint.surface_needs_composite = TRUE;
    }
    else
    {
        window->current_paint.surface = gdk_window_ref_impl_surface(window);
        window->current_paint.surface_needs_composite = FALSE;
    }

    if (!cairo_region_is_empty(window->current_paint.region))
        gdk_window_clear_backing_region(window);
}

static void gdk_window_end_paint_internal(_GdkWindow* window) {
    _GdkWindow* composited;
    GdkRectangle clip_box = { 0, };
    cairo_t* cr;

    if (GDK_WINDOW_DESTROYED(window) ||
        !gdk_window_has_impl(window))
        return;

    if (window->current_paint.surface == NULL)
    {
        g_warning(G_STRLOC": no preceding call to gdk_window_begin_draw_frame(), see documentation");
        return;
    }

    _GdkWindowImplClass* impl_class = getGdkWindowImplClass();

    if (impl_class->end_paint)
        impl_class->end_paint(window);

    if (window->current_paint.surface_needs_composite)
    {
        cairo_surface_t* surface;

        cairo_region_get_extents(window->current_paint.region, &clip_box);

        if (window->current_paint.use_gl)
        {
            cairo_region_t* opaque_region = cairo_region_copy(window->current_paint.region);
            cairo_region_subtract(opaque_region, window->current_paint.flushed_region);
            cairo_region_subtract(opaque_region, window->current_paint.need_blend_region);

            gdk_gl_context_make_current(window->gl_paint_context);

            if (!cairo_region_is_empty(opaque_region))
                gdk_gl_texture_from_surface(window->current_paint.surface,
                    opaque_region);
            if (!cairo_region_is_empty(window->current_paint.need_blend_region))
            {
                glEnable(GL_BLEND);
                gdk_gl_texture_from_surface(window->current_paint.surface,
                    window->current_paint.need_blend_region);
                glDisable(GL_BLEND);
            }

            cairo_region_destroy(opaque_region);

            gdk_gl_context_end_frame(window->gl_paint_context,
                window->current_paint.region,
                window->active_update_area);
        }
        else
        {
            surface = gdk_window_ref_impl_surface(window);
            cr = cairo_create(surface);

            cairo_set_source_surface(cr, window->current_paint.surface, 0, 0);
            gdk_cairo_region(cr, window->current_paint.region);
            cairo_clip(cr);

            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_paint(cr);

            cairo_destroy(cr);

            cairo_surface_flush(surface);
            cairo_surface_destroy(surface);
        }
    }

    gdk_window_free_current_paint(window);

    // find a composited window in our hierarchy to signal its
    // parent to redraw, calculating the clip box as we go...
    // stop if parent becomes NULL since then we'd have nowhere
    // to draw (ie: 'composited' will always be non-NULL here).
    for (composited = window; composited->parent; composited = composited->parent) {
        clip_box.x += composited->x;
        clip_box.y += composited->y;

        _GdkWindow* parent = composited->parent;
        clip_box.width = MIN(clip_box.width, parent->width - clip_box.x);
        clip_box.height = MIN(clip_box.height, parent->height - clip_box.y);

        if (composited->composited) {
            gdk_window_invalidate_rect(GDK_WINDOW(composited->parent), &clip_box, FALSE);
            break;
        }
    }
}*/

#endif

GdkGLContext* gdk_window_get_paint_gl_context(_GdkWindow* window, GError** error) {
  GError *internal_error = NULL;

  _GdkWindow* iwindow = (_GdkWindow*)window->impl_window;
  if (iwindow->gl_paint_context == NULL) {
      _GdkWindowImplClass *impl_class = getGdkWindowImplClass();

      if (impl_class->create_gl_context == NULL) {
          g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE, "The current backend does not support OpenGL");
          return NULL;
        }
#ifdef _WIN32
      iwindow->gl_paint_context = win32_window_create_gl_context(iwindow, TRUE, NULL, FALSE, &internal_error);
#else
      iwindow->gl_paint_context = x11_window_create_gl_context ((GdkWindow*)iwindow, TRUE, NULL, FALSE, &internal_error);
#endif
    }

  if (internal_error != NULL) {
      g_propagate_error (error, internal_error);
      g_clear_object (&(iwindow->gl_paint_context));
      return NULL;
    }

  gdk_gl_context_realize (iwindow->gl_paint_context, &internal_error);
  if (internal_error != NULL) {
        printf("setting the gl_paint_context context failed!\n");
      g_propagate_error (error, internal_error);
      g_clear_object (&(iwindow->gl_paint_context));
      return NULL;
  }

  return iwindow->gl_paint_context;
}

GdkGLContext* _gdk_window_create_gl_context (_GdkWindow* window, GError** error) {
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  GdkGLContext* paint_context = gdk_window_get_paint_gl_context (window, error);
  if (paint_context == NULL) return NULL;

#ifdef _WIN32
  return win32_window_create_gl_context(window->impl_window, TRUE, NULL, TRUE, error);
#else
  return x11_window_create_gl_context(window->impl_window, TRUE, NULL, TRUE, error);
#endif
}

static GdkGLContext* gl_area_real_create_context(GLArea *area) {
    printf("gl_area_real_create_context\n");
    //GLAreaPrivate *priv = gl_area_get_instance_private (area);
    GtkWidget *widget = GTK_WIDGET (area);
    GError *error = NULL;
    GdkGLContext *context;

    override_window_invalidate_for_new_frame((_GdkWindow*)gtk_widget_get_window(widget));
/*#ifdef _WIN32 // in GuiManager
    override_win32_gl_context_realize();
#endif*/

    //context = gdk_window_create_gl_context (gtk_widget_get_window (widget), &error);
    context = _gdk_window_create_gl_context((_GdkWindow*)gtk_widget_get_window (widget), &error);
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

    return context;
}

static void gl_area_realize (GtkWidget *widget) {
    printf("gl_area_realize\n");
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

    //_GdkWindow* window = gtk_widget_get_window(widget);

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
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    priv->needs_resize = TRUE;
    //gtk_widget_queue_draw(GTK_WIDGET(area));
}

static void gl_area_unrealize (GtkWidget *widget) {
    GLArea *area = GL_AREA (widget);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);

    if (priv->context != NULL) {
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

cairo_region_t *
gdk_cairo_region_from_clip (cairo_t *cr)
{
  cairo_rectangle_list_t *rectangles;
  cairo_region_t *region;
  int i;

  rectangles = cairo_copy_clip_rectangle_list (cr);

  if (rectangles->status != CAIRO_STATUS_SUCCESS)
    return NULL;

  region = cairo_region_create ();
  for (i = 0; i < rectangles->num_rectangles; i++)
    {
      cairo_rectangle_int_t clip_rect;
      cairo_rectangle_t *rect;

      rect = &rectangles->rectangles[i];

      /* Here we assume clip rects are ints for direct targets, which
         is true for cairo */
      clip_rect.x = (int)rect->x;
      clip_rect.y = (int)rect->y;
      clip_rect.width = (int)rect->width;
      clip_rect.height = (int)rect->height;

      cairo_region_union_rectangle (region, &clip_rect);
    }

  cairo_rectangle_list_destroy (rectangles);

  return region;
}

void gdk_window_get_unscaled_size (_GdkWindow *window, int *unscaled_width, int *unscaled_height) {
    g_return_if_fail (GDK_IS_WINDOW (window));

    if (window->impl_window == window) {
        //impl_class = GDK_WINDOW_IMPL_GET_CLASS (window->impl);
        _GdkWindowImplClass* impl_class = getGdkWindowImplClass();
        impl_class->get_unscaled_size((GdkWindow*)window, unscaled_width, unscaled_height);
        return;
    }

    gint scale = gdk_window_get_scale_factor((GdkWindow*)window);
    if (unscaled_width) *unscaled_width = window->width * scale;
    if (unscaled_height) *unscaled_height = window->height * scale;
}



#define FLIP_Y(_y) (unscaled_window_height - (_y))

cairo_region_t* clip_region;
int window_scale;
int unscaled_window_width, unscaled_window_height, dx, dy;
cairo_rectangle_int_t clip_rect, dest;
_GdkWindow* impl_window;

void gl_area_trigger_resize(GLArea* area) {
    GLAreaPrivate* priv = gl_area_get_instance_private(area);
    priv->needs_resize = TRUE;
    global_invalidate = TRUE;
}

void glarea_render(GLArea* area) {
    GLAreaPrivate* priv = gl_area_get_instance_private (area);

    //if (priv->needs_resize) { // insufficient, when reducing the width, when getting small its not called anymore??
    if (priv->needs_resize || priv->clipping.w != dest.width || priv->clipping.h != dest.height) {
        priv->clipping.x = dest.x;
        priv->clipping.y = dest.y;
        priv->clipping.w = dest.width;
        priv->clipping.h = dest.height;
        priv->clipping.W = unscaled_window_width;
        priv->clipping.H = unscaled_window_height;
        g_signal_emit (area, area_signals[RESIZE], 0, 0, 0, NULL);
        priv->needs_resize = FALSE;
        priv->needs_render = TRUE;
    }

    if (priv->needs_render) {
        gboolean unused;
        g_signal_emit (area, area_signals[RENDER], 0, priv->context, &unused);
        priv->needs_render = FALSE;
    }
}

void cairo_draw(cairo_t* cr, GLArea* area, _GdkWindow* window, int scale, int x, int y, int width, int height) {
  if (glGetError() != GL_NO_ERROR) printf(" gl error on cairo_draw_begin beg\n");
  GLAreaPrivate* priv = gl_area_get_instance_private (area);
  impl_window = window->impl_window;
  window_scale = gdk_window_get_scale_factor ((GdkWindow*)impl_window);
  clip_region = gdk_cairo_region_from_clip (cr);
  gdk_gl_context_make_current(priv->context);
  if (glGetError() != GL_NO_ERROR) printf(" gl error on gdk_gl_context_make_current\n");
  cairo_matrix_t matrix;
  cairo_get_matrix (cr, &matrix);
  dx = matrix.x0;
  dy = matrix.y0;

  if (clip_region != NULL) {
      /* Translate to impl coords */
      cairo_region_translate(clip_region, dx, dy);
      gdk_window_get_unscaled_size (impl_window, &unscaled_window_width, &unscaled_window_height);

      for (int i = 0; i < cairo_region_num_rectangles (clip_region); i++) {
          cairo_region_get_rectangle (clip_region, i, &clip_rect);
          clip_rect.x *= window_scale;
          clip_rect.y *= window_scale;
          clip_rect.width *= window_scale;
          clip_rect.height *= window_scale;

          dest.x = dx * window_scale;
          dest.y = dy * window_scale;
          dest.width = width * window_scale / scale;
          dest.height = height * window_scale / scale;

          if (gdk_rectangle_intersect (&clip_rect, &dest, &dest)) {
              glarea_render(area);
              if (impl_window->current_paint.flushed_region) {
                  cairo_rectangle_int_t flushed_rect;

                  flushed_rect.x = dest.x / window_scale;
                  flushed_rect.y = dest.y / window_scale;
                  flushed_rect.width = (dest.x + dest.width + window_scale - 1) / window_scale - flushed_rect.x;
                  flushed_rect.height = (dest.y + dest.height + window_scale - 1) / window_scale - flushed_rect.y;

                  cairo_region_union_rectangle (impl_window->current_paint.flushed_region, &flushed_rect);
                  cairo_region_subtract_rectangle (impl_window->current_paint.need_blend_region, &flushed_rect);
                }
            }
        }
    }

    if (clip_region) cairo_region_destroy (clip_region);
}

static gboolean gl_area_draw(GtkWidget* widget, cairo_t* cr) {
    //SET_DEBUG_BREAK_POINT(B,1)
    if (glGetError() != GL_NO_ERROR) printf(" gl error on gl_area_draw beg\n");

    GLArea *area = GL_AREA(widget);
    //gtk_widget_set_double_buffered(widget, FALSE);
    GLAreaPrivate *priv = gl_area_get_instance_private (area);
    //gboolean unused;
    int w, h, scale;
    //GLenum status;

    if (priv->error != NULL) return FALSE;
    if (priv->context == NULL) return FALSE;

    gl_area_make_current (area);
    if (glGetError() != GL_NO_ERROR) printf(" gl error on gl_area_make_current\n");

    //glEnable (GL_DEPTH_TEST);

    scale = gtk_widget_get_scale_factor (widget);
    w = gtk_widget_get_allocated_width (widget) * scale;
    h = gtk_widget_get_allocated_height (widget) * scale;
    cairo_draw(cr, area, (_GdkWindow*)gtk_widget_get_window(widget), scale, 0, 0, w, h);

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
    priv->clipping.x = priv->clipping.y = 0;
    priv->clipping.w = priv->clipping.h = 100;
    priv->clipping.W = priv->clipping.H = 100;
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

GLClipping gl_area_get_clipping(GLArea* area) {
    GLAreaPrivate* priv = gl_area_get_instance_private (area);
    return priv->clipping;
}

#ifndef _WIN32
int glXSwapIntervalSGI(int);
#endif

void gl_area_set_vsync(GLArea* area, gboolean b) {
#ifndef _WIN32
    if (b) {
        int res = glXSwapIntervalSGI(1);
        printf(" gl_area_set_vsync %i\n", res);
    } else {
        int res = glXSwapIntervalSGI(0);
        printf(" gl_area_set_vsync %i\n", res);
    }
#endif
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
    if (glGetError() != GL_NO_ERROR) printf(" gl error on gdk_gl_context_make_current bevore\n");
    if (priv->context != NULL) gdk_gl_context_make_current (priv->context);
    if (glGetError() != GL_NO_ERROR) printf(" gl error on gdk_gl_context_make_current after\n");
}
