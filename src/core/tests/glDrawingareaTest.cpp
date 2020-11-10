

#include <GL/gl.h>
#include <gtk/gtk.h>
#include "core/gui/gtkglext/gtk/gtkgl.h"

static void on_realize(GtkDrawingArea* a, void* d) {
    gtk_widget_begin_gl(GTK_WIDGET(a));

	// Print version info:
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

    gtk_widget_end_gl(GTK_WIDGET(a), false);
}

static bool on_render(GtkDrawingArea* a, void* event, void* d) {
    gtk_widget_begin_gl(GTK_WIDGET(a));
	glClearColor(1.0, 0.2, 1.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    gtk_widget_end_gl(GTK_WIDGET(a), true);
	return true;
}

bool gui_init(int* argc, char*** argv) {
	gtk_init(argc, argv);
	gtk_gl_init(argc, argv);
	return true;
}

bool gui_run(void) {
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget* glarea = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), glarea);

    auto mode = (GdkGLConfigMode)(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH | GDK_GL_MODE_STENCIL | GDK_GL_MODE_MULTISAMPLE);
    GdkGLConfig* glConfigMode = gdk_gl_config_new_by_mode(mode, 4);
    gtk_widget_set_gl_capability(glarea,glConfigMode,NULL,true,GDK_GL_RGBA_TYPE);

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(glarea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glarea, "draw", G_CALLBACK(on_render), NULL);

	gtk_widget_show_all(window);

	gtk_main();
	return true;
}

int main(int argc, char** argv) {
	return gui_init(&argc, &argv) && gui_run() ? 0 : 1;
}

