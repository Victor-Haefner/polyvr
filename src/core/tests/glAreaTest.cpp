

#include <windows.h>
#include <GL/gl.h>
#include <gtk/gtk.h>

static gboolean on_render(GtkGLArea* glarea, GdkGLContext* context) {
	glClearColor(1.0, 0.2, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return TRUE;
}

static void on_realize(GtkGLArea* glarea) {
	// Make current:
	gtk_gl_area_make_current(glarea);

	// Print version info:
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	// Enable depth buffer:
	gtk_gl_area_set_has_depth_buffer(glarea, TRUE);


	// Get frame clock:
	GdkGLContext* glcontext = gtk_gl_area_get_context(glarea);
	GdkWindow* glwindow = gdk_gl_context_get_window(glcontext);
	GdkFrameClock* frame_clock = gdk_window_get_frame_clock(glwindow);

	// Connect update signal:
	g_signal_connect_swapped
	(frame_clock
		, "update"
		, G_CALLBACK(gtk_gl_area_queue_render)
		, glarea
	);

	// Start updating:
	gdk_frame_clock_begin_updating(frame_clock);
}

bool gui_init(int* argc, char*** argv) {
	if (!gtk_init_check(argc, argv)) {
		fputs("Could not initialize GTK", stderr);
		return false;
	}
	return true;
}

bool gui_run(void) {
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget* glarea = gtk_gl_area_new();
	gtk_container_add(GTK_CONTAINER(window), glarea);

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(glarea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glarea, "render", G_CALLBACK(on_render), NULL);

	gtk_widget_show_all(window);

	gtk_main();
	return true;
}

int main(int argc, char** argv) {
	return gui_init(&argc, &argv) && gui_run() ? 0 : 1;
}