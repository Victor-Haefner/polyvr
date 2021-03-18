#ifndef GLAREAWIN_H_INCLUDED
#define GLAREAWIN_H_INCLUDED


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


#endif // GLAREAWIN_H_INCLUDED
