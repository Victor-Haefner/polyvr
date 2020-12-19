#ifndef __GL_GDK_H__
#define __GL_GDK_H__

#define GTK_COMPILATION

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

gboolean glgtk_init_check (int* argc, char*** argv);

void replace_gl_visuals();

G_END_DECLS

#endif /* __GL_GDK_H__ */
