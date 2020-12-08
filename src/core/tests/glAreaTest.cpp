

#include <windows.h>
#include <iostream>
#include <GL/gl.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#define GOBJECT_COMPILATION
#include <gobject/gtype.h>
//#include <glib-object.h>

using namespace std;


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

static inline gpointer gdk_gl_context_get_instance_private(GdkGLContext* context) {
	gpointer klass = g_type_class_ref(GDK_TYPE_GL_CONTEXT);
	gint privOffset = g_type_class_get_instance_private_offset(klass);
	return (G_STRUCT_MEMBER_P(context, privOffset));
}

GdkGLContext* on_create_context(GtkGLArea* area, gpointer user_data) {
	//GtkVRAreaPrivate* priv = gtk_gl_area_get_instance_private(area);
	GtkWidget* widget = GTK_WIDGET(area);
	GError* error = NULL;

	//g_setenv("GDK_GL_LEGACY", "TRUE", true);

	GdkGLContext* context = gdk_window_create_gl_context(gtk_widget_get_window(widget), &error);
	if (error != NULL) {
		gtk_gl_area_set_error(area, error);
		g_clear_object(&context);
		g_clear_error(&error);
		return NULL;
	}

	//GdkGLContextPrivate* cData = (GdkGLContextPrivate*)context;

	GdkGLContextPrivate* cData = (GdkGLContextPrivate*)gdk_gl_context_get_instance_private(context);

	cout << "create context: " << context << endl;
	gdk_gl_context_set_use_es(context, false);
	//gdk_gl_context_set_required_version(context, 1, 2);
	gdk_gl_context_set_forward_compatible(context, false);
	cData->is_legacy = 1;
	cData->major = 3;
	cData->minor = 0;
	//gdk_gl_context_set_is_legacy(context, true);

	cout << " is_legacy: " << gdk_gl_context_is_legacy(context) << endl;
	cout << " FUUUCK " << cData->major << " " << cData->minor << " " << cData->gl_version << endl;

	gdk_gl_context_realize(context, &error);
	cData->is_legacy = 1;

	if (error != NULL) {
		gtk_gl_area_set_error(area, error);
		g_clear_object(&context);
		g_clear_error(&error);
		return NULL;
	}

	return context;
}

static gboolean on_render(GtkGLArea* glarea, GdkGLContext* context) {
	glClearColor(1.0, 0.2, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBegin(GL_TRIANGLES);
	glColor3f(1, 0, 0); // red
	glVertex2f(-0.8, -0.8);
	glColor3f(0, 1, 0); // green
	glVertex2f(0.8, -0.8);
	glColor3f(0, 0, 1); // blue
	glVertex2f(0, 0.9);
	glEnd();

	return TRUE;
}

static void on_realize(GtkGLArea* glarea) {
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

	cout << "realized context: " << glcontext << endl;

	// Connect update signal:
	g_signal_connect_swapped
	(frame_clock
		, "update"
		, G_CALLBACK(gtk_gl_area_queue_render)
		, glarea
	);

	// Start updating:
	gdk_frame_clock_begin_updating(frame_clock);



	cout << "gdk_gl_context_is_legacy: " << gdk_gl_context_is_legacy(glcontext) << endl;
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
	g_signal_connect(glarea, "create-context", G_CALLBACK(on_create_context), NULL);

	gtk_widget_show_all(window);

	gtk_main();
	return true;
}

int main(int argc, char** argv) {
	return gui_init(&argc, &argv) && gui_run() ? 0 : 1;
}