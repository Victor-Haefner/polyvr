
#define GDK_COMPILATION

#include "glareaCommon.h"

//#define WAYLAND

#ifdef WAYLAND
#include "glareaWayland.h"
#else
#include "glareaX11.h"
#endif

#include "glgdk.h"

#include <GL/gl.h>
#include <GL/glx.h>

void replace_gl_visuals() {
    GdkDisplay* display = gdk_display_get_default();
    GdkScreen* screen = gdk_display_get_default_screen(display);
#ifdef WAYLAND
    //glgdk_wayland_screen_update_visuals_for_gl((_GdkScreen*)screen);
#else
    glgdk_x11_screen_update_visuals_for_gl((_GdkScreen*)screen);
#endif
}
