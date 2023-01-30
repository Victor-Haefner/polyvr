#include "glareaCommon.h"
#include "glareaWayland.h"

void initGLFunctions() {}


EGLSurface gdk_wayland_window_get_egl_surface (_GdkWindow *window, EGLConfig  config) {
	GdkDisplay* display = gdk_window_get_display((GdkWindow*)window);
    _GdkWaylandDisplay* display_wayland = (_GdkWaylandDisplay*) (display);

    g_return_val_if_fail (GDK_IS_WAYLAND_WINDOW (window), NULL);

    _GdkWindowImplWayland* impl = (_GdkWindowImplWayland*) (window->impl);

    if (impl->egl_surface == NULL) {
        struct wl_egl_window* egl_window = gdk_wayland_window_get_wl_egl_window (window);
        impl->egl_surface = eglCreateWindowSurface (display_wayland->egl_display, config, egl_window, NULL);
    }

    return impl->egl_surface;
}

void _gdk_wayland_window_invalidate_for_new_frame (_GdkWindow* window, cairo_region_t* update_area) {
	cairo_rectangle_int_t window_rect;
	GdkDisplay* display = gdk_window_get_display((GdkWindow*)window);
	_GdkWaylandDisplay *display_wayland = (_GdkWaylandDisplay*) (display);

	gboolean invalidate_all;

	if (window->gl_paint_context == NULL) return;

	_GdkWaylandGLContext* context_wayland = (_GdkWaylandGLContext*) (window->gl_paint_context);

	int buffer_age = 0;

	EGLSurface egl_surface = gdk_wayland_window_get_egl_surface (window->impl_window, context_wayland->egl_config);

	if (display_wayland->have_egl_buffer_age) {
		gdk_gl_context_make_current (window->gl_paint_context);
		eglQuerySurface (display_wayland->egl_display, egl_surface, EGL_BUFFER_AGE_EXT, &buffer_age);
	}

	invalidate_all = FALSE;
	if (buffer_age == 0 || buffer_age >= 4) invalidate_all = TRUE;
	else {
		if (buffer_age >= 2) {
			if (window->old_updated_area[0]) cairo_region_union (update_area, window->old_updated_area[0]);
			else invalidate_all = TRUE;
		}

		if (buffer_age >= 3) {
			if (window->old_updated_area[1]) cairo_region_union (update_area, window->old_updated_area[1]);
			else invalidate_all = TRUE;
		}
	}

    /*if (buffer_age >= 4) invalidate_all = TRUE; // TODO: check the behavior above and maybe use this one instead!
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
    }*/

	if (invalidate_all || *getGlobalInvalidate()) {
		window_rect.x = 0;
		window_rect.y = 0;
		window_rect.width = gdk_window_get_width (window);
		window_rect.height = gdk_window_get_height (window);

		/* If nothing else is known, repaint everything so that the back
		* buffer is fully up-to-date for the swapbuffer */
		cairo_region_union_rectangle (update_area, &window_rect);
        *getGlobalInvalidate() = FALSE;
	}
}

static EGLDisplay
gdk_wayland_get_display (_GdkWaylandDisplay *display_wayland)
{
  EGLDisplay dpy = NULL;

  if (epoxy_has_egl_extension (NULL, "EGL_KHR_platform_base"))
    {
      PFNEGLGETPLATFORMDISPLAYPROC getPlatformDisplay =
	(void *) eglGetProcAddress ("eglGetPlatformDisplay");

      if (getPlatformDisplay)
	dpy = getPlatformDisplay (EGL_PLATFORM_WAYLAND_EXT,
				  display_wayland->wl_display,
				  NULL);
      if (dpy)
	return dpy;
    }

  if (epoxy_has_egl_extension (NULL, "EGL_EXT_platform_base"))
    {
      PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
	(void *) eglGetProcAddress ("eglGetPlatformDisplayEXT");

      if (getPlatformDisplay)
	dpy = getPlatformDisplay (EGL_PLATFORM_WAYLAND_EXT,
				  display_wayland->wl_display,
				  NULL);
      if (dpy)
	return dpy;
    }

  return eglGetDisplay ((EGLNativeDisplayType) display_wayland->wl_display);
}

gboolean
gdk_wayland_display_init_gl (GdkDisplay *display)
{
  _GdkWaylandDisplay *display_wayland = (_GdkWaylandDisplay*) (display);
  EGLint major, minor;
  EGLDisplay dpy;

  if (display_wayland->have_egl)
    return TRUE;

  dpy = gdk_wayland_get_display (display_wayland);

  if (dpy == NULL)
    return FALSE;

  if (!eglInitialize (dpy, &major, &minor))
    return FALSE;

  if (!eglBindAPI (EGL_OPENGL_API))
    return FALSE;

  display_wayland->egl_display = dpy;
  display_wayland->egl_major_version = major;
  display_wayland->egl_minor_version = minor;

  display_wayland->have_egl = TRUE;

  display_wayland->have_egl_khr_create_context =
    epoxy_has_egl_extension (dpy, "EGL_KHR_create_context");

  display_wayland->have_egl_buffer_age =
    epoxy_has_egl_extension (dpy, "EGL_EXT_buffer_age");

  display_wayland->have_egl_swap_buffers_with_damage =
    epoxy_has_egl_extension (dpy, "EGL_EXT_swap_buffers_with_damage");

  display_wayland->have_egl_surfaceless_context =
    epoxy_has_egl_extension (dpy, "EGL_KHR_surfaceless_context");
  return TRUE;
}

#define MAX_EGL_ATTRS   30

static gboolean
find_eglconfig_for_window (GdkWindow  *window, EGLConfig  *egl_config_out, EGLint     *min_swap_interval_out, GError    **error) {
  GdkDisplay *display = gdk_window_get_display (window);
  _GdkWaylandDisplay *display_wayland = (_GdkWaylandDisplay*) (display);
  GdkVisual *visual = gdk_window_get_visual (window);
  EGLint count;
  EGLConfig *configs, chosen_config;
  gboolean use_rgba;

    static int attrs[] = { // those do not matter as we only want to find the config with ID xvisual_id
      EGL_SURFACE_TYPE   , EGL_WINDOW_BIT,
      EGL_COLOR_BUFFER_TYPE     , EGL_RGB_BUFFER,
      EGL_RED_SIZE        , 1,
      EGL_GREEN_SIZE      , 1,
      EGL_BLUE_SIZE       , 1,
      EGL_ALPHA_SIZE      , EGL_DONT_CARE,
      EGL_NONE
    };


  /*int i = 0;

  EGLint attrs[MAX_EGL_ATTRS];
  attrs[i++] = EGL_SURFACE_TYPE;
  attrs[i++] = EGL_WINDOW_BIT;
  attrs[i++] = EGL_COLOR_BUFFER_TYPE;
  attrs[i++] = EGL_RGB_BUFFER;
  attrs[i++] = EGL_RED_SIZE;
  attrs[i++] = 1;
  attrs[i++] = EGL_GREEN_SIZE;
  attrs[i++] = 1;
  attrs[i++] = EGL_BLUE_SIZE;
  attrs[i++] = 1;

  use_rgba = (visual == gdk_screen_get_rgba_visual (gdk_display_get_default_screen (display)));

  if (use_rgba)
    {
      attrs[i++] = EGL_ALPHA_SIZE;
      attrs[i++] = 1;
    }
  else
    {
      attrs[i++] = EGL_ALPHA_SIZE;
      attrs[i++] = 0;
    }

  attrs[i++] = EGL_NONE;
  g_assert (i < MAX_EGL_ATTRS);*/

  if (!eglChooseConfig (display_wayland->egl_display, attrs, NULL, 0, &count) || count < 1)
    {
      printf("1 - No available configurations for the given pixel format");
      return FALSE;
    }

  configs = g_new (EGLConfig, count);

  if (!eglChooseConfig (display_wayland->egl_display, attrs, configs, count, &count) || count < 1)
    {
      printf("2 - No available configurations for the given pixel format");
      return FALSE;
    }

  /* Pick first valid configuration i guess? */
  chosen_config = configs[0];

  if (!eglGetConfigAttrib (display_wayland->egl_display, chosen_config,
                           EGL_MIN_SWAP_INTERVAL, min_swap_interval_out))
    {
      g_set_error_literal (error, GDK_GL_ERROR,
                           GDK_GL_ERROR_NOT_AVAILABLE,
                           "Could not retrieve the minimum swap interval");
      g_free (configs);
      return FALSE;
    }

  if (egl_config_out != NULL)
    *egl_config_out = chosen_config;

  g_free (configs);

  return TRUE;
}

GdkGLContext* gdk_wayland_window_create_gl_context (GdkWindow* window, gboolean attached, GdkGLContext* share, GError** error) {
  GdkDisplay *display = gdk_window_get_display (window);
  _GdkWaylandDisplay *display_wayland = (_GdkWaylandDisplay*) (display);
  _GdkWaylandGLContext *context;
  EGLConfig config;

  if (!gdk_wayland_display_init_gl (display)) {
      printf("No GL implementation is available\n");
      return NULL;
    }

  if (!display_wayland->have_egl_khr_create_context) {
      printf("Core GL is not available on EGL implementation\n");
      return NULL;
    }

  if (!find_eglconfig_for_window (window, &config, &display_wayland->egl_min_swap_interval, error)) return NULL;

  context = g_object_new (GDK_TYPE_WAYLAND_GL_CONTEXT, "display", display, "window", window, "shared-context", share, NULL);
  context->egl_config = config;
  context->is_attached = attached;
  return GDK_GL_CONTEXT (context);
}
