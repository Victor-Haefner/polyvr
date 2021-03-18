#ifndef GLAREACOMMON_H_INCLUDED
#define GLAREACOMMON_H_INCLUDED

#include <gtk/gtkversion.h>

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

#if GTK_CHECK_VERSION(3,24,11)
	guint synthesized_crossing_event_id;
#endif

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


#endif // GLAREACOMMON_H_INCLUDED
