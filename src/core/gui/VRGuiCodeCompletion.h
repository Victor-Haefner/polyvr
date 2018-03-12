#ifndef VRGUICODECOMPLETION_H_INCLUDED
#define VRGUICODECOMPLETION_H_INCLUDED

#include <gtk/gtk.h>

struct VRGuiCodeCompletionPrivate;

struct VRGuiCodeCompletion {
	GObject parent;
	VRGuiCodeCompletionPrivate* priv = 0;
};

struct VRGuiCodeCompletionClass {
	GObjectClass parent_class;
};

VRGuiCodeCompletion* VRGuiCodeCompletionNew();

#endif // VRGUICODECOMPLETION_H_INCLUDED
