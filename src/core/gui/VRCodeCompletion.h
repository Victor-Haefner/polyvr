#ifndef VRCODECOMPLETION_H_INCLUDED
#define VRCODECOMPLETION_H_INCLUDED

#include <gtksourceview/gtksourcecompletionprovider.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_SOURCE_COMPLETION_WORDS				(vr_code_completion_get_type ())
#define GTK_SOURCE_COMPLETION_WORDS(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SOURCE_COMPLETION_WORDS, VRCodeCompletion))
#define GTK_SOURCE_COMPLETION_WORDS_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SOURCE_COMPLETION_WORDS, VRCodeCompletion const))
#define GTK_SOURCE_COMPLETION_WORDS_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_SOURCE_COMPLETION_WORDS, VRCodeCompletionClass))
#define GTK_IS_SOURCE_COMPLETION_WORDS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_SOURCE_COMPLETION_WORDS))
#define GTK_IS_SOURCE_COMPLETION_WORDS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_SOURCE_COMPLETION_WORDS))
#define GTK_SOURCE_COMPLETION_WORDS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_SOURCE_COMPLETION_WORDS, VRCodeCompletionClass))

typedef struct _VRCodeCompletion		    VRCodeCompletion;
typedef struct _VRCodeCompletionClass		VRCodeCompletionClass;
typedef struct _VRCodeCompletionPrivate		VRCodeCompletionPrivate;

struct _VRCodeCompletion {
	GObject parent;
	VRCodeCompletionPrivate* priv;
};

struct _VRCodeCompletionClass {
	GObjectClass parent_class;
};

GType vr_code_completion_get_type (void) G_GNUC_CONST;
VRCodeCompletion* vr_code_completion_new();

G_END_DECLS

#endif // VRCODECOMPLETION_H_INCLUDED
